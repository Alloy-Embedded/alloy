#!/usr/bin/env python3
"""
Generate Platform GPIO Abstraction

This generator creates high-level GPIO abstractions in src/hal/platform/
that use the low-level vendor-specific register definitions.

Key features:
- Template-based with PORT_BASE and PIN_NUM parameters (matches manual design)
- Uses Result<T> for error handling
- Includes test hooks for unit testing
- Automatically includes all vendor headers (registers, pins, etc)
- Zero runtime overhead (fully inlined)

Part of Platform Abstraction Layer
"""

import sys
from pathlib import Path
from typing import List, Dict, Set

# Add parent to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

from cli.core.logger import print_header, print_success, print_error, print_info, logger
from cli.core.config import normalize_name, REPO_ROOT, HAL_VENDORS_DIR
from cli.parsers.svd_discovery import discover_all_svds


def get_platform_dir(family: str) -> Path:
    """
    Get platform directory for a family.

    Args:
        family: Family name (e.g., "same70", "samv71", "samd21")

    Returns:
        Path to platform directory (e.g., src/hal/platform/same70/)
    """
    return REPO_ROOT / "src" / "hal" / "platform" / family.lower()


def get_vendor_mcu_path(mcu_name: str) -> Path:
    """
    Find the vendor path for an MCU that has pins generated.

    Args:
        mcu_name: MCU name (e.g., "atsame70q21b")

    Returns:
        Path to MCU vendor directory, or None if not found
    """
    # Search for MCU with pins in vendors directory
    pin_files = list(HAL_VENDORS_DIR.rglob(f"**/{mcu_name.lower()}/pins.hpp"))

    if not pin_files:
        return None

    # Return parent directory of pins.hpp
    return pin_files[0].parent


def detect_gpio_architecture(mcu_path: Path) -> str:
    """
    Detect which GPIO architecture the MCU uses.

    Now checks FAMILY-LEVEL registers first, falling back to MCU-level.

    Returns:
        'pio' for Atmel SAME70/SAMV71 (PIOx)
        'gpio' for STM32 (GPIOx)
        'port' for Atmel SAMD21 (PORT)
        'unknown' if cannot detect
    """
    # Try family-level registers first (NEW)
    family_path = mcu_path.parent
    family_registers_dir = family_path / "registers"

    # Fall back to MCU-level registers (OLD)
    mcu_registers_dir = mcu_path / "registers"

    # Use whichever exists
    registers_dir = family_registers_dir if family_registers_dir.exists() else mcu_registers_dir

    if not registers_dir.exists():
        return 'unknown'

    # Check for PIO architecture (SAME70/SAMV71)
    if list(registers_dir.glob("pio*_registers.hpp")):
        return 'pio'

    # Check for GPIO architecture (STM32)
    if list(registers_dir.glob("gpio*_registers.hpp")):
        return 'gpio'

    # Check for PORT architecture (SAMD21)
    if (registers_dir / "port_registers.hpp").exists():
        return 'port'

    return 'unknown'


def get_port_info_from_mcu(mcu_path: Path) -> Dict[str, any]:
    """
    Extract port information from generated register files.

    Now looks at FAMILY-LEVEL registers first (new architecture),
    falling back to MCU-level for backward compatibility.

    Args:
        mcu_path: Path to MCU vendor directory

    Returns:
        Dict with port info: {
            'architecture': 'pio' | 'gpio' | 'port',
            'ports': ['A', 'B', 'C', 'D', 'E'],
            'port_registers': {'A': 'pioa', 'B': 'piob', ...},
            'port_bases': {'A': '0x400E0E00', ...}
        }
    """
    import re

    # Detect architecture from MCU path (still works)
    arch = detect_gpio_architecture(mcu_path)

    ports = []
    port_bases = {}
    port_registers = {}

    # Try family-level registers first (NEW: vendors/{vendor}/{family}/registers/)
    family_path = mcu_path.parent  # Go up one level from MCU to family
    family_registers_dir = family_path / "registers"

    # Fall back to MCU-level registers (OLD: vendors/{vendor}/{family}/{mcu}/registers/)
    mcu_registers_dir = mcu_path / "registers"

    # Prefer family-level, but support MCU-level for backward compatibility
    registers_dir = family_registers_dir if family_registers_dir.exists() else mcu_registers_dir

    if not registers_dir.exists():
        return {
            'architecture': arch,
            'ports': [],
            'port_registers': {},
            'port_bases': {}
        }

    if arch == 'pio':
        # Atmel SAME70/SAMV71 - PIO architecture
        for reg_file in registers_dir.glob("pio*_registers.hpp"):
            port_letter = reg_file.stem.replace("pio", "").replace("_registers", "").upper()
            if len(port_letter) == 1:
                ports.append(port_letter)
                port_registers[port_letter] = f"pio{port_letter.lower()}"

                # Extract base address
                content = reg_file.read_text()
                pattern = rf'inline\s+PIO{port_letter}_Registers\*\s+PIO{port_letter}\(\)\s*{{\s*return\s+reinterpret_cast<.*>\((0x[0-9A-Fa-f]+)\);'
                match = re.search(pattern, content)
                if match:
                    port_bases[port_letter] = match.group(1)

    elif arch == 'gpio':
        # STM32 - GPIO architecture
        for reg_file in registers_dir.glob("gpio*_registers.hpp"):
            port_letter = reg_file.stem.replace("gpio", "").replace("_registers", "").upper()
            if len(port_letter) == 1:
                ports.append(port_letter)
                port_registers[port_letter] = f"gpio{port_letter.lower()}"

                # Extract base address
                content = reg_file.read_text()
                pattern = rf'inline\s+GPIO{port_letter}_Registers\*\s+GPIO{port_letter}\(\)\s*{{\s*return\s+reinterpret_cast<.*>\((0x[0-9A-Fa-f]+)\);'
                match = re.search(pattern, content)
                if match:
                    port_bases[port_letter] = match.group(1)

    elif arch == 'port':
        # Atmel SAMD21 - PORT architecture (single PORT with groups)
        port_file = registers_dir / "port_registers.hpp"
        if port_file.exists():
            content = port_file.read_text()

            # SAMD21 uses groups (GROUP0, GROUP1) internally
            # For simplicity, we'll treat it as port 'A' and 'B'
            pattern = r'inline\s+PORT_Registers\*\s+PORT\(\)\s*{\s*return\s+reinterpret_cast<.*>\((0x[0-9A-Fa-f]+)\);'
            match = re.search(pattern, content)

            if match:
                # SAMD21 has 1 PORT peripheral but multiple groups
                # We'll expose it as a single base
                ports = ['PORT']
                port_registers['PORT'] = 'port'
                port_bases['PORT'] = match.group(1)

    return {
        'architecture': arch,
        'ports': sorted(ports),
        'port_registers': port_registers,
        'port_bases': port_bases
    }


def generate_platform_gpio_header(mcu_name: str, family: str, vendor: str, port_info: Dict) -> str:
    """
    Generate platform GPIO header with Result<T> and test hooks.

    Args:
        mcu_name: MCU name (e.g., "atsame70q21b")
        family: Family name (e.g., "same70")
        vendor: Vendor name (e.g., "atmel")
        port_info: Port information dict

    Returns:
        C++ header content
    """

    mcu_lower = mcu_name.lower()
    mcu_upper = mcu_name.upper()
    family_lower = family.lower()
    family_upper = family.upper()
    vendor_lower = vendor.lower()

    arch = port_info.get('architecture', 'unknown')
    ports = port_info['ports']
    port_bases = port_info.get('port_bases', {})

    # Dispatch to architecture-specific generator
    if arch == 'pio':
        return generate_pio_gpio_header(mcu_name, family, vendor, port_info)
    elif arch == 'gpio':
        return generate_stm32_gpio_header(mcu_name, family, vendor, port_info)
    elif arch == 'port':
        logger.warning(f"Architecture 'port' (SAMD21) not yet supported for {family}, skipping...")
        return None
    else:
        logger.warning(f"Architecture '{arch}' unknown for {family}, skipping...")
        return None


def generate_pio_gpio_header(mcu_name: str, family: str, vendor: str, port_info: Dict) -> str:
    """
    Generate platform GPIO header for PIO architecture (Atmel SAME70/SAMV71).

    This is the original implementation moved to its own function.
    """
    mcu_lower = mcu_name.lower()
    mcu_upper = mcu_name.upper()
    family_lower = family.lower()
    family_upper = family.upper()
    vendor_lower = vendor.lower()

    ports = port_info['ports']
    port_bases = port_info.get('port_bases', {})

    # Generate port base address constants
    port_base_defs = []
    for port in ports:
        base_addr = port_bases.get(port, f"0x{40000000 + int(port, 36) * 0x200:08X}")  # Fallback
        port_base_defs.append(f"constexpr uint32_t PIO{port}_BASE = {base_addr};")

    port_base_constants = "\n".join(port_base_defs)

    # Determine the primary register namespace (use first port)
    primary_port = ports[0] if ports else 'A'
    primary_port_lower = f"pio{primary_port.lower()}"

    content = f"""/**
 * @file gpio.hpp
 * @brief Template-based GPIO implementation for {family_upper} (Platform Layer)
 *
 * This file implements GPIO peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead. It automatically includes all
 * necessary vendor-specific headers.
 *
 * Design Principles:
 * - Template-based: Port address and pin number resolved at compile-time
 * - Zero overhead: Fully inlined, single instruction for read/write
 * - Compile-time masks: Pin masks computed at compile-time
 * - Type-safe: Strong typing prevents pin conflicts
 * - Error handling: Uses Result<T> for robust error handling
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: {mcu_name}
 * Generator: generate_platform_gpio.py
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/{vendor_lower}/{family_lower}/registers/{primary_port_lower}_registers.hpp"

// Bitfields (family-level, if available)
// #include "hal/vendors/{vendor_lower}/{family_lower}/bitfields/{primary_port_lower}_bitfields.hpp"

// Hardware definitions (MCU-specific - port bases, etc)
// Note: Board files should include hardware.hpp from specific MCU if needed
// #include "hal/vendors/{vendor_lower}/{family_lower}/{mcu_lower}/hardware.hpp"

// Pin definitions and functions (MCU-specific)
// Note: These should be included by board files as they're MCU-specific
// Example: #include "hal/vendors/{vendor_lower}/{family_lower}/stm32f407vg/pins.hpp"

namespace alloy::hal::{family_lower} {{

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types (now from family-level namespace)
using namespace alloy::hal::{vendor_lower}::{family_lower};

/**
 * @brief GPIO pin modes
 */
enum class GpioMode {{
    Input,           ///< Input mode
    Output,          ///< Output mode (push-pull)
    OutputOpenDrain  ///< Output mode with open-drain
}};

/**
 * @brief GPIO pull resistor configuration
 */
enum class GpioPull {{
    None,      ///< No pull resistor
    Up,        ///< Pull-up resistor enabled
    Down       ///< Pull-down resistor enabled (if supported)
}};

/**
 * @brief Template-based GPIO pin for {family_upper}
 *
 * This class provides a template-based GPIO implementation with ZERO runtime
 * overhead. All pin masks and operations are resolved at compile-time.
 *
 * Template Parameters:
 * - PORT_BASE: PIO port base address (compile-time constant)
 * - PIN_NUM: Pin number within port (0-31)
 *
 * Example usage:
 * @code
 * // Define LED pin (PIOC pin 8)
 * using LedGreen = GpioPin<PIOC_BASE, 8>;
 *
 * // Use it
 * auto led = LedGreen{{}};
 * auto result = led.setMode(GpioMode::Output);
 * if (result.isOk()) {{
 *     led.set();     // Turn on
 *     led.clear();   // Turn off
 *     led.toggle();  // Toggle state
 * }}
 * @endcode
 *
 * @tparam PORT_BASE PIO port base address
 * @tparam PIN_NUM Pin number (0-31)
 */
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {{
public:
    // Compile-time constants
    static constexpr uint32_t port_base = PORT_BASE;
    static constexpr uint8_t pin_number = PIN_NUM;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);

    // Validate pin number at compile-time
    static_assert(PIN_NUM < 32, "Pin number must be 0-31");

    /**
     * @brief Get PIO port registers
     *
     * Returns pointer to PIO registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile {primary_port_lower}::PIO{primary_port}_Registers* get_port() {{
#ifdef ALLOY_GPIO_MOCK_PORT
        // In tests, use the mock port pointer
        return ALLOY_GPIO_MOCK_PORT();
#else
        return reinterpret_cast<volatile {primary_port_lower}::PIO{primary_port}_Registers*>(PORT_BASE);
#endif
    }}

    /**
     * @brief Set GPIO pin mode
     *
     * Configures pin as input or output with optional open-drain.
     *
     * @param mode Desired pin mode
     * @return Result<void> Ok() if successful
     */
    Result<void> setMode(GpioMode mode) {{
        auto* port = get_port();

        // Enable PIO control (disable peripheral function)
        port->PER = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_PER
        ALLOY_GPIO_TEST_HOOK_PER();
#endif

        switch (mode) {{
            case GpioMode::Input:
                // Configure as input
                port->ODR = pin_mask;  // Disable output
#ifdef ALLOY_GPIO_TEST_HOOK_ODR
                ALLOY_GPIO_TEST_HOOK_ODR();
#endif
                break;

            case GpioMode::Output:
                // Configure as output (push-pull)
                port->OER = pin_mask;   // Enable output
#ifdef ALLOY_GPIO_TEST_HOOK_OER
                ALLOY_GPIO_TEST_HOOK_OER();
#endif
                port->MDDR = pin_mask;  // Disable multi-driver (open-drain)
#ifdef ALLOY_GPIO_TEST_HOOK_MDDR
                ALLOY_GPIO_TEST_HOOK_MDDR();
#endif
                break;

            case GpioMode::OutputOpenDrain:
                // Configure as output with open-drain
                port->OER = pin_mask;   // Enable output
#ifdef ALLOY_GPIO_TEST_HOOK_OER
                ALLOY_GPIO_TEST_HOOK_OER();
#endif
                port->MDER = pin_mask;  // Enable multi-driver (open-drain)
#ifdef ALLOY_GPIO_TEST_HOOK_MDER
                ALLOY_GPIO_TEST_HOOK_MDER();
#endif
                break;
        }}

        return Result<void>::ok();
    }}

    /**
     * @brief Set pin HIGH (output = 1)
     *
     * Single instruction: writes pin mask to SODR register.
     * Only affects this specific pin.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> set() {{
        auto* port = get_port();
        port->SODR = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_SODR
        ALLOY_GPIO_TEST_HOOK_SODR();
#endif
        return Result<void>::ok();
    }}

    /**
     * @brief Set pin LOW (output = 0)
     *
     * Single instruction: writes pin mask to CODR register.
     * Only affects this specific pin.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> clear() {{
        auto* port = get_port();
        port->CODR = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_CODR
        ALLOY_GPIO_TEST_HOOK_CODR();
#endif
        return Result<void>::ok();
    }}

    /**
     * @brief Toggle pin state
     *
     * Reads current output state and inverts it.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> toggle() {{
        auto* port = get_port();

        // Read current output data status
        if (port->ODSR & pin_mask) {{
            // Pin is HIGH, set it LOW
            port->CODR = pin_mask;
        }} else {{
            // Pin is LOW, set it HIGH
            port->SODR = pin_mask;
        }}

        return Result<void>::ok();
    }}

    /**
     * @brief Write pin value
     *
     * @param value true for HIGH, false for LOW
     * @return Result<void> Always Ok()
     */
    Result<void> write(bool value) {{
        if (value) {{
            return set();
        }} else {{
            return clear();
        }}
    }}

    /**
     * @brief Read pin input value
     *
     * Reads the actual pin state from PDSR register.
     *
     * @return Result<bool> Pin state (true = HIGH, false = LOW)
     */
    Result<bool> read() const {{
        auto* port = get_port();
        bool value = (port->PDSR & pin_mask) != 0;
        return Result<bool>::ok(value);
    }}

    /**
     * @brief Configure pull resistor
     *
     * Note: {family_upper} PIO has built-in pull-up resistors.
     * Pull-down support depends on hardware.
     *
     * @param pull Pull resistor configuration
     * @return Result<void> Ok() if successful, Err() if not supported
     */
    Result<void> setPull(GpioPull pull) {{
        auto* port = get_port();

        switch (pull) {{
            case GpioPull::None:
                // Disable pull-up
                port->PUDR = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_PUDR
                ALLOY_GPIO_TEST_HOOK_PUDR();
#endif
                break;

            case GpioPull::Up:
                // Enable pull-up
                port->PUER = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_PUER
                ALLOY_GPIO_TEST_HOOK_PUER();
#endif
                break;

            case GpioPull::Down:
                // Pull-down not supported in {family_upper} hardware
                return Result<void>::error(ErrorCode::NotSupported);
        }}

        return Result<void>::ok();
    }}

    /**
     * @brief Enable input glitch filter
     *
     * @return Result<void> Always Ok()
     */
    Result<void> enableFilter() {{
        auto* port = get_port();
        port->IFER = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_IFER
        ALLOY_GPIO_TEST_HOOK_IFER();
#endif
        return Result<void>::ok();
    }}

    /**
     * @brief Disable input glitch filter
     *
     * @return Result<void> Always Ok()
     */
    Result<void> disableFilter() {{
        auto* port = get_port();
        port->IFDR = pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_IFDR
        ALLOY_GPIO_TEST_HOOK_IFDR();
#endif
        return Result<void>::ok();
    }}

    /**
     * @brief Check if pin is configured as output
     *
     * @return Result<bool> true if output, false if input
     */
    Result<bool> isOutput() const {{
        auto* port = get_port();
        bool is_output = (port->OSR & pin_mask) != 0;
        return Result<bool>::ok(is_output);
    }}
}};

// ==============================================================================
// Port Base Address Constants
// ==============================================================================

{port_base_constants}

// ==============================================================================
// Common Pin Type Aliases
// ==============================================================================

// Board-specific pin definitions should be in board.hpp
// Example:
// using LedGreen = GpioPin<PIOC_BASE, 8>;
// using Button0 = GpioPin<PIOA_BASE, 11>;

}} // namespace alloy::hal::{family_lower}
"""

    return content


def generate_stm32_gpio_header(mcu_name: str, family: str, vendor: str, port_info: Dict) -> str:
    """
    Generate platform GPIO header for GPIO architecture (STM32).

    Args:
        mcu_name: MCU name (e.g., "stm32f407")
        family: Family name (e.g., "stm32f4")
        vendor: Vendor name (e.g., "st")
        port_info: Port information dict

    Returns:
        C++ header content
    """
    mcu_lower = mcu_name.lower()
    mcu_upper = mcu_name.upper()
    family_lower = family.lower()
    family_upper = family.upper()
    vendor_lower = vendor.lower()

    ports = port_info['ports']
    port_bases = port_info.get('port_bases', {})

    # Generate port base address constants
    port_base_defs = []
    for port in ports:
        base_addr = port_bases.get(port, f"0x{0x40020000 + (ord(port) - ord('A')) * 0x400:08X}")
        port_base_defs.append(f"constexpr uint32_t GPIO{port}_BASE = {base_addr};")

    port_base_constants = "\n".join(port_base_defs)

    # Determine the primary register namespace (use first port)
    primary_port = ports[0] if ports else 'A'
    primary_port_lower = f"gpio{primary_port.lower()}"

    content = f"""/**
 * @file gpio.hpp
 * @brief Template-based GPIO implementation for {family_upper} (Platform Layer)
 *
 * This file implements GPIO peripheral using templates with ZERO virtual
 * functions and ZERO runtime overhead. It automatically includes all
 * necessary vendor-specific headers.
 *
 * Design Principles:
 * - Template-based: Port address and pin number resolved at compile-time
 * - Zero overhead: Fully inlined, single instruction for read/write
 * - Compile-time masks: Pin masks computed at compile-time
 * - Type-safe: Strong typing prevents pin conflicts
 * - Error handling: Uses Result<T> for robust error handling
 * - Testable: Includes test hooks for unit testing
 *
 * Auto-generated from: {mcu_name}
 * Generator: generate_platform_gpio.py
 *
 * @note Part of Alloy HAL Platform Abstraction Layer
 */

#pragma once

// ============================================================================
// Core Types
// ============================================================================

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

// ============================================================================
// Vendor-Specific Includes (Auto-Generated)
// ============================================================================

// Register definitions from vendor (family-level)
#include "hal/vendors/{vendor_lower}/{family_lower}/registers/{primary_port_lower}_registers.hpp"

// Bitfields (family-level, if available)
// #include "hal/vendors/{vendor_lower}/{family_lower}/bitfields/{primary_port_lower}_bitfields.hpp"

// Hardware definitions (MCU-specific - if available)
// Note: Board files should include hardware.hpp from specific MCU if needed
// #include "hal/vendors/{vendor_lower}/{family_lower}/{{mcu}}/hardware.hpp"

// Pin definitions (MCU-specific - if available)
// Note: These should be included by board files as they're MCU-specific
// Example: #include "hal/vendors/{vendor_lower}/{family_lower}/stm32f407vg/pins.hpp"

namespace alloy::hal::{family_lower} {{

using namespace alloy::core;
using namespace alloy::hal;

// Import vendor-specific register types (now from family-level namespace)
using namespace alloy::hal::{vendor_lower}::{family_lower};

/**
 * @brief GPIO pin modes
 */
enum class GpioMode {{
    Input,           ///< Input mode
    Output,          ///< Output mode (push-pull)
    OutputOpenDrain, ///< Output mode with open-drain
    Alternate,       ///< Alternate function mode
    Analog           ///< Analog mode
}};

/**
 * @brief GPIO pull resistor configuration
 */
enum class GpioPull {{
    None,      ///< No pull resistor (floating)
    Up,        ///< Pull-up resistor enabled
    Down       ///< Pull-down resistor enabled
}};

/**
 * @brief GPIO output speed
 */
enum class GpioSpeed {{
    Low,       ///< Low speed
    Medium,    ///< Medium speed
    High,      ///< High speed (fast)
    VeryHigh   ///< Very high speed
}};

/**
 * @brief Template-based GPIO pin for {family_upper}
 *
 * This class provides a template-based GPIO implementation with ZERO runtime
 * overhead. All pin masks and operations are resolved at compile-time.
 *
 * Template Parameters:
 * - PORT_BASE: GPIO port base address (compile-time constant)
 * - PIN_NUM: Pin number within port (0-15)
 *
 * Example usage:
 * @code
 * // Define LED pin (GPIOC pin 13)
 * using LedGreen = GpioPin<GPIOC_BASE, 13>;
 *
 * // Use it
 * auto led = LedGreen{{}};
 * auto result = led.setMode(GpioMode::Output);
 * if (result.isOk()) {{
 *     led.set();     // Turn on
 *     led.clear();   // Turn off
 *     led.toggle();  // Toggle state
 * }}
 * @endcode
 *
 * @tparam PORT_BASE GPIO port base address
 * @tparam PIN_NUM Pin number (0-15)
 */
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {{
public:
    // Compile-time constants
    static constexpr uint32_t port_base = PORT_BASE;
    static constexpr uint8_t pin_number = PIN_NUM;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);
    static constexpr uint8_t pin_pos = PIN_NUM;

    // Validate pin number at compile-time
    static_assert(PIN_NUM < 16, "STM32 GPIO pin number must be 0-15");

    /**
     * @brief Get GPIO port registers
     *
     * Returns pointer to GPIO registers. Uses conditional compilation
     * for test hook injection.
     */
    static inline volatile {primary_port_lower}::GPIO{primary_port}_Registers* get_port() {{
#ifdef ALLOY_GPIO_MOCK_PORT
        // In tests, use the mock port pointer
        return ALLOY_GPIO_MOCK_PORT();
#else
        return reinterpret_cast<volatile {primary_port_lower}::GPIO{primary_port}_Registers*>(PORT_BASE);
#endif
    }}

    /**
     * @brief Set GPIO pin mode
     *
     * Configures pin as input, output, alternate function, or analog.
     *
     * @param mode Desired pin mode
     * @return Result<void> Ok() if successful
     */
    Result<void> setMode(GpioMode mode) {{
        auto* port = get_port();

        // Clear mode bits for this pin (2 bits per pin)
        uint32_t moder = port->MODER;
        moder &= ~(0x3u << (pin_pos * 2));

        // Set new mode
        switch (mode) {{
            case GpioMode::Input:
                // 00: Input mode
                break;

            case GpioMode::Output:
                // 01: General purpose output mode
                moder |= (0x1u << (pin_pos * 2));
                // Set to push-pull by default
                port->OTYPER &= ~pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_OTYPER
                ALLOY_GPIO_TEST_HOOK_OTYPER();
#endif
                break;

            case GpioMode::OutputOpenDrain:
                // 01: General purpose output mode
                moder |= (0x1u << (pin_pos * 2));
                // Set to open-drain
                port->OTYPER |= pin_mask;
#ifdef ALLOY_GPIO_TEST_HOOK_OTYPER
                ALLOY_GPIO_TEST_HOOK_OTYPER();
#endif
                break;

            case GpioMode::Alternate:
                // 10: Alternate function mode
                moder |= (0x2u << (pin_pos * 2));
                break;

            case GpioMode::Analog:
                // 11: Analog mode
                moder |= (0x3u << (pin_pos * 2));
                break;
        }}

        port->MODER = moder;
#ifdef ALLOY_GPIO_TEST_HOOK_MODER
        ALLOY_GPIO_TEST_HOOK_MODER();
#endif

        return Result<void>::ok();
    }}

    /**
     * @brief Set pin HIGH (output = 1)
     *
     * Single instruction: writes pin mask to BSRR register.
     * Only affects this specific pin atomically.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> set() {{
        auto* port = get_port();
        port->BSRR = pin_mask;  // Set bit (lower 16 bits)
#ifdef ALLOY_GPIO_TEST_HOOK_BSRR
        ALLOY_GPIO_TEST_HOOK_BSRR();
#endif
        return Result<void>::ok();
    }}

    /**
     * @brief Set pin LOW (output = 0)
     *
     * Single instruction: writes pin mask to BSRR register (upper 16 bits).
     * Only affects this specific pin atomically.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> clear() {{
        auto* port = get_port();
        port->BSRR = (pin_mask << 16);  // Reset bit (upper 16 bits)
#ifdef ALLOY_GPIO_TEST_HOOK_BSRR
        ALLOY_GPIO_TEST_HOOK_BSRR();
#endif
        return Result<void>::ok();
    }}

    /**
     * @brief Toggle pin state
     *
     * Reads current output state and inverts it.
     *
     * @return Result<void> Always Ok()
     */
    Result<void> toggle() {{
        auto* port = get_port();

        // Read current output data register
        if (port->ODR & pin_mask) {{
            // Pin is HIGH, set it LOW
            port->BSRR = (pin_mask << 16);
        }} else {{
            // Pin is LOW, set it HIGH
            port->BSRR = pin_mask;
        }}

        return Result<void>::ok();
    }}

    /**
     * @brief Write pin value
     *
     * @param value true for HIGH, false for LOW
     * @return Result<void> Always Ok()
     */
    Result<void> write(bool value) {{
        if (value) {{
            return set();
        }} else {{
            return clear();
        }}
    }}

    /**
     * @brief Read pin input value
     *
     * Reads the actual pin state from IDR register.
     *
     * @return Result<bool> Pin state (true = HIGH, false = LOW)
     */
    Result<bool> read() const {{
        auto* port = get_port();
        bool value = (port->IDR & pin_mask) != 0;
        return Result<bool>::ok(value);
    }}

    /**
     * @brief Configure pull resistor
     *
     * STM32 GPIO supports both pull-up and pull-down.
     *
     * @param pull Pull resistor configuration
     * @return Result<void> Ok() if successful
     */
    Result<void> setPull(GpioPull pull) {{
        auto* port = get_port();

        // Clear pull bits for this pin (2 bits per pin)
        uint32_t pupdr = port->PUPDR;
        pupdr &= ~(0x3u << (pin_pos * 2));

        switch (pull) {{
            case GpioPull::None:
                // 00: No pull-up, pull-down
                break;

            case GpioPull::Up:
                // 01: Pull-up
                pupdr |= (0x1u << (pin_pos * 2));
                break;

            case GpioPull::Down:
                // 10: Pull-down
                pupdr |= (0x2u << (pin_pos * 2));
                break;
        }}

        port->PUPDR = pupdr;
#ifdef ALLOY_GPIO_TEST_HOOK_PUPDR
        ALLOY_GPIO_TEST_HOOK_PUPDR();
#endif

        return Result<void>::ok();
    }}

    /**
     * @brief Set output speed
     *
     * @param speed Desired output speed
     * @return Result<void> Always Ok()
     */
    Result<void> setSpeed(GpioSpeed speed) {{
        auto* port = get_port();

        // Clear speed bits for this pin (2 bits per pin)
        uint32_t ospeedr = port->OSPEEDR;
        ospeedr &= ~(0x3u << (pin_pos * 2));

        uint8_t speed_val = static_cast<uint8_t>(speed);
        ospeedr |= (speed_val << (pin_pos * 2));

        port->OSPEEDR = ospeedr;
#ifdef ALLOY_GPIO_TEST_HOOK_OSPEEDR
        ALLOY_GPIO_TEST_HOOK_OSPEEDR();
#endif

        return Result<void>::ok();
    }}

    /**
     * @brief Check if pin is configured as output
     *
     * @return Result<bool> true if output, false if input
     */
    Result<bool> isOutput() const {{
        auto* port = get_port();
        uint32_t mode = (port->MODER >> (pin_pos * 2)) & 0x3;
        // Mode 01 = output
        bool is_output = (mode == 0x1);
        return Result<bool>::ok(is_output);
    }}
}};

// ==============================================================================
// Port Base Address Constants
// ==============================================================================

{port_base_constants}

// ==============================================================================
// Common Pin Type Aliases
// ==============================================================================

// Board-specific pin definitions should be in board.hpp
// Example:
// using LedGreen = GpioPin<GPIOC_BASE, 13>;
// using Button0 = GpioPin<GPIOA_BASE, 0>;

}} // namespace alloy::hal::{family_lower}
"""

    return content


def discover_mcus_with_pins() -> List[Dict[str, str]]:
    """
    Discover all MCUs that have pins generated and extract their info.

    Returns:
        List of dicts with: {
            'mcu': 'atsame70q21b',
            'family': 'same70',
            'vendor': 'atmel',
            'path': Path(...)
        }
    """
    mcus = []

    # Find all pin_functions.hpp files
    pin_files = HAL_VENDORS_DIR.rglob("**/pins.hpp")

    for pin_file in pin_files:
        mcu_dir = pin_file.parent
        mcu_name = mcu_dir.name

        # Extract family and vendor from path
        # Path: .../vendors/{vendor}/{family}/{mcu}/pins.hpp
        family_dir = mcu_dir.parent
        vendor_dir = family_dir.parent

        family = family_dir.name
        vendor = vendor_dir.name

        mcus.append({
            'mcu': mcu_name,
            'family': family,
            'vendor': vendor,
            'path': mcu_dir
        })

    return mcus


def generate_for_all_mcus(verbose: bool = False) -> int:
    """
    Generate platform GPIO for all MCUs with pins.

    Args:
        verbose: Enable verbose output

    Returns:
        Exit code (0 for success)
    """
    print_header("Generating Platform GPIO Abstractions")

    # Discover all MCUs with pins
    mcus = discover_mcus_with_pins()

    if not mcus:
        print_error("No MCUs with pins found!")
        print_info("Run pin generation first: python3 codegen.py generate --pins")
        return 1

    print_info(f"Found {len(mcus)} MCU(s) with pins")

    # Group by family
    families: Dict[str, List[Dict]] = {}
    for mcu_info in mcus:
        family = mcu_info['family']
        if family not in families:
            families[family] = []
        families[family].append(mcu_info)

    print_info(f"Grouped into {len(families)} family/families:")
    for family, mcu_list in families.items():
        print_info(f"  • {family}: {len(mcu_list)} MCU(s)")

    success_count = 0

    # Generate platform GPIO for each family (use MCU with most ports as reference)
    for family, mcu_list in sorted(families.items()):
        # Find MCU with most ports that has registers generated
        best_mcu = None
        max_ports = 0

        for mcu_info in mcu_list:
            # Check for registers at either family-level or MCU-level
            family_path = mcu_info['path'].parent
            family_registers = family_path / "registers"
            mcu_registers = mcu_info['path'] / "registers"

            # Skip if no registers exist at either level
            if not family_registers.exists() and not mcu_registers.exists():
                continue

            port_info_temp = get_port_info_from_mcu(mcu_info['path'])
            port_count = len(port_info_temp['ports'])

            if port_count > max_ports:
                max_ports = port_count
                best_mcu = mcu_info

        if not best_mcu:
            # No MCU with registers found in this family
            if verbose:
                print_info(f"\nGenerating platform GPIO for {family}...")
                print_error(f"  ✗ No MCU with registers found")
            continue

        reference_mcu = best_mcu

        mcu_name = reference_mcu['mcu']
        vendor = reference_mcu['vendor']
        mcu_path = reference_mcu['path']

        if verbose:
            print_info(f"\nGenerating platform GPIO for {family} (using {mcu_name})...")

        # Get port information from MCU
        port_info = get_port_info_from_mcu(mcu_path)

        if not port_info['ports']:
            print_error(f"  ✗ No ports found for {family}")
            continue

        if verbose:
            print_info(f"  Ports: {', '.join(port_info['ports'])}")

        # Generate platform GPIO header
        gpio_content = generate_platform_gpio_header(
            mcu_name, family, vendor, port_info
        )

        if not gpio_content:
            # Architecture not supported yet
            if verbose:
                print_error(f"  ✗ Architecture '{port_info.get('architecture')}' not yet supported")
            continue

        # Create platform directory
        platform_dir = get_platform_dir(family)
        platform_dir.mkdir(parents=True, exist_ok=True)

        # Write gpio.hpp
        gpio_file = platform_dir / "gpio.hpp"
        gpio_file.write_text(gpio_content)

        if verbose:
            print_success(f"  ✓ {gpio_file}")

        success_count += 1

    print_success(f"\nGenerated platform GPIO for {success_count}/{len(families)} family/families")

    return 0 if success_count == len(families) else 1


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description='Generate platform GPIO abstractions')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    args = parser.parse_args()

    return generate_for_all_mcus(args.verbose)


if __name__ == '__main__':
    sys.exit(main())
