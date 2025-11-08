#!/usr/bin/env python3
"""
Generate Pin Headers for Atmel SAMD21 Family

This script generates complete pin definition headers for SAMD21 MCUs.
Architecture: PORT controller with 8 alternate functions (A-H)
"""

from pathlib import Path
from typing import Dict, List
import sys

# Add codegen directory to path for imports
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

# Import the pin function database
try:
    from cli.vendors.atmel.samd21_pin_functions import (
        get_pin_functions,
        get_all_pins,
        PinFunction
    )
except ModuleNotFoundError:
    from samd21_pin_functions import (
        get_pin_functions,
        get_all_pins,
        PinFunction
    )

# Import centralized path utilities and config
from cli.core.paths import get_mcu_output_dir, ensure_dir
from cli.core.config import normalize_vendor, detect_family

# Import progress tracking
from cli.core.progress import get_global_tracker


# SAMD21 MCU variants and their configurations
SAMD21_VARIANTS = {
    "ATSAMD21G18A": {
        "package": "TQFP48",
        "flash_kb": 256,
        "ram_kb": 32,
        "ports": {
            "A": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 14, 15, 16, 17, 18, 19, 22, 23, 24, 25, 27, 28, 30, 31],
            "B": [2, 3, 8, 9, 10, 11, 22, 23],
        }
    },
    "ATSAMD21E18A": {
        "package": "TQFP32",
        "flash_kb": 256,
        "ram_kb": 32,
        "ports": {
            "A": [2, 3, 4, 5, 6, 7, 8, 9, 14, 15, 16, 17, 22, 23, 24, 25, 27, 28, 30, 31],
        }
    },
    "ATSAMD21J18A": {
        "package": "TQFP64",
        "flash_kb": 256,
        "ram_kb": 32,
        "ports": {
            "A": list(range(32)),  # PA0-PA31
            "B": list(range(32)),  # PB0-PB31
        }
    },
}


def generate_hardware_header(device_name: str, variant_config: Dict) -> str:
    """Generate hardware.hpp with PORT register definitions"""

    content = f"""#pragma once

#include <cstdint>

namespace alloy::hal::atmel::samd21::{device_name.lower()}::hardware {{

// ============================================================================
// Hardware Register Definitions for {device_name.upper()}
// Based on SAMD21 PORT Controller Architecture
// ============================================================================

// Memory map
constexpr uint32_t FLASH_BASE = 0x00000000;
constexpr uint32_t FLASH_SIZE = {variant_config['flash_kb']}U * 1024U;
constexpr uint32_t SRAM_BASE  = 0x20000000;
constexpr uint32_t SRAM_SIZE  = {variant_config['ram_kb']}U * 1024U;

// PORT Controller base addresses
constexpr uint32_t PORT_BASE = 0x41004400;

// ============================================================================
// PORT Register Structure (SAMD21 Architecture)
// ============================================================================

struct PORT_Group {{
    volatile uint32_t DIR;          // 0x00: Data Direction
    volatile uint32_t DIRCLR;       // 0x04: Data Direction Clear
    volatile uint32_t DIRSET;       // 0x08: Data Direction Set
    volatile uint32_t DIRTGL;       // 0x0C: Data Direction Toggle
    volatile uint32_t OUT;          // 0x10: Data Output Value
    volatile uint32_t OUTCLR;       // 0x14: Data Output Value Clear
    volatile uint32_t OUTSET;       // 0x18: Data Output Value Set
    volatile uint32_t OUTTGL;       // 0x1C: Data Output Value Toggle
    volatile uint32_t IN;           // 0x20: Data Input Value
    volatile uint32_t CTRL;         // 0x24: Control
    volatile uint32_t WRCONFIG;     // 0x28: Write Configuration
    uint32_t RESERVED1;             // 0x2C: Reserved
    volatile uint8_t  PMUX[16];     // 0x30: Peripheral Multiplexing (0-15)
    volatile uint8_t  PINCFG[32];   // 0x40: Pin Configuration (0-31)
    uint32_t RESERVED2[8];          // 0x60: Reserved
}};

struct PORT_Registers {{
    PORT_Group GROUP[2];            // Group 0 = PORT A, Group 1 = PORT B
}};

// PORT instance
static_assert(sizeof(PORT_Group) == 0x80, "PORT_Group size mismatch");

inline PORT_Registers* PORT = reinterpret_cast<PORT_Registers*>(PORT_BASE);
inline PORT_Group* PORTA = &PORT->GROUP[0];
inline PORT_Group* PORTB = &PORT->GROUP[1];

// Pin configuration bits
constexpr uint8_t PORT_PINCFG_PMUXEN  = (1 << 0);  // Peripheral Multiplexer Enable
constexpr uint8_t PORT_PINCFG_INEN    = (1 << 1);  // Input Enable
constexpr uint8_t PORT_PINCFG_PULLEN  = (1 << 2);  // Pull Enable
constexpr uint8_t PORT_PINCFG_DRVSTR  = (1 << 6);  // Output Driver Strength

}}  // namespace alloy::hal::atmel::samd21::{device_name.lower()}::hardware
"""

    return content


def generate_pins_header(device_name: str, variant_config: Dict) -> str:
    """Generate pins.hpp with pin number definitions"""

    content = f"""#pragma once

#include <cstdint>

namespace alloy::hal::atmel::samd21::{device_name.lower()}::pins {{

// ============================================================================
// Pin Definitions for {device_name.upper()}
// Package: {variant_config['package']}
// ============================================================================

"""

    # Generate pin numbers for each port
    for port_name, pin_numbers in variant_config['ports'].items():
        content += f"// Port {port_name} pins\n"
        for pin_num in pin_numbers:
            pin_name = f"P{port_name}{pin_num:02d}"
            content += f"constexpr uint8_t {pin_name} = {pin_num};  // {port_name}{pin_num}\n"
        content += "\n"

    content += f"""
// Port indices
enum class Port : uint8_t {{
    A = 0,
    B = 1,
}};

}}  // namespace alloy::hal::atmel::samd21::{device_name.lower()}::pins
"""

    return content


def generate_gpio_header(device_name: str, variant_config: Dict) -> str:
    """
    Generate type-safe, zero-overhead GPIO abstraction.

    SAMD21 uses PORT controller with arrays [0]=PORTA, [1]=PORTB
    """

    # Determine which ports this variant has
    ports = variant_config.get('ports', {})
    port_count = len(ports)

    content = f"""#pragma once

#include <cstdint>
#include "hardware.hpp"
#include "pins.hpp"

namespace alloy::hal::atmel::samd21::{device_name.lower()} {{

// ============================================================================
// Type-Safe GPIO Pin Abstraction
//
// Template-based GPIO abstraction using hardware PORT registers.
// All operations are constexpr and inline, resulting in assembly identical
// to direct register manipulation.
//
// SAMD21 Architecture:
// - PORT controller with groups: PORT->GROUP[0] = PORTA, GROUP[1] = PORTB
// - Each port has 32 pins (0-31)
// - 8 peripheral functions (A-H) via PMUX register
//
// Usage:
//   using Led = GPIOPin<pins::PA17>;
//   Led::configureOutput();
//   Led::set();
//   Led::toggle();
// ============================================================================

template<uint8_t GlobalPin>
class GPIOPin {{
public:
    // Compile-time port/pin calculation
    static constexpr uint8_t PORT_IDX = GlobalPin / 32;  // 0=A, 1=B
    static constexpr uint8_t PIN = GlobalPin % 32;
    static constexpr uint32_t MASK = (1U << PIN);

    static_assert(PORT_IDX < {port_count}, "Invalid port index for this MCU variant");

    // Get PORT register group at compile-time
    static inline hardware::PORT_Group* getPortGroup() {{
        return &hardware::PORT->GROUP[PORT_IDX];
    }}

    // ========================================================================
    // Pin Mode Configuration
    // ========================================================================

    /// Configure pin as GPIO output
    static inline void configureOutput() {{
        auto* port = getPortGroup();
        port->PINCFG[PIN] = 0;                     // Disable PMUX (GPIO mode)
        port->DIRSET = MASK;                       // Set direction to output
    }}

    /// Configure pin as GPIO input
    static inline void configureInput() {{
        auto* port = getPortGroup();
        port->PINCFG[PIN] = hardware::PORT_PINCFG_INEN;  // Enable input buffer
        port->DIRCLR = MASK;                              // Clear direction (input)
    }}

    /// Configure pin as input with pull-up
    static inline void configureInputPullUp() {{
        auto* port = getPortGroup();
        port->PINCFG[PIN] = hardware::PORT_PINCFG_INEN | hardware::PORT_PINCFG_PULLEN;
        port->DIRCLR = MASK;                       // Clear direction (input)
        port->OUTSET = MASK;                       // Pull-up (OUT selects up/down)
    }}

    /// Configure pin as input with pull-down
    static inline void configureInputPullDown() {{
        auto* port = getPortGroup();
        port->PINCFG[PIN] = hardware::PORT_PINCFG_INEN | hardware::PORT_PINCFG_PULLEN;
        port->DIRCLR = MASK;                       // Clear direction (input)
        port->OUTCLR = MASK;                       // Pull-down (OUT selects up/down)
    }}

    // ========================================================================
    // Digital I/O Operations
    // ========================================================================

    /// Set pin high
    static inline void set() {{
        getPortGroup()->OUTSET = MASK;
    }}

    /// Set pin low
    static inline void clear() {{
        getPortGroup()->OUTCLR = MASK;
    }}

    /// Toggle pin state (hardware toggle on SAMD21)
    static inline void toggle() {{
        getPortGroup()->OUTTGL = MASK;
    }}

    /// Write boolean value to pin
    static inline void write(bool value) {{
        if (value) {{
            set();
        }} else {{
            clear();
        }}
    }}

    /// Read pin state
    [[nodiscard]] static inline bool read() {{
        return (getPortGroup()->IN & MASK) != 0;
    }}

    // ========================================================================
    // Peripheral Function Configuration (A-H)
    // ========================================================================

    enum class PeripheralFunction : uint8_t {{
        GPIO = 0xFF,  // GPIO mode (PMUX disabled)
        A = 0,        // Peripheral A
        B = 1,        // Peripheral B
        C = 2,        // Peripheral C
        D = 3,        // Peripheral D
        E = 4,        // Peripheral E
        F = 5,        // Peripheral F
        G = 6,        // Peripheral G
        H = 7,        // Peripheral H
    }};

    /// Configure pin for peripheral function (A-H)
    static inline void configurePeripheral(PeripheralFunction func) {{
        auto* port = getPortGroup();

        if (func == PeripheralFunction::GPIO) {{
            port->PINCFG[PIN] &= ~hardware::PORT_PINCFG_PMUXEN;  // Disable PMUX
            return;
        }}

        // Configure PMUX register (each register controls 2 pins)
        uint8_t pmux_idx = PIN / 2;
        uint8_t pmux_shift = (PIN & 1) ? 4 : 0;  // Odd pin = upper nibble, even = lower
        uint8_t func_val = static_cast<uint8_t>(func);

        uint8_t pmux_val = port->PMUX[pmux_idx];
        pmux_val &= ~(0xF << pmux_shift);            // Clear old value
        pmux_val |= (func_val << pmux_shift);        // Set new value
        port->PMUX[pmux_idx] = pmux_val;

        port->PINCFG[PIN] |= hardware::PORT_PINCFG_PMUXEN;  // Enable PMUX
    }}

    // ========================================================================
    // Advanced Features
    // ========================================================================

    /// Enable strong drive strength
    static inline void enableStrongDrive() {{
        getPortGroup()->PINCFG[PIN] |= hardware::PORT_PINCFG_DRVSTR;
    }}

    /// Disable strong drive (normal strength)
    static inline void disableStrongDrive() {{
        getPortGroup()->PINCFG[PIN] &= ~hardware::PORT_PINCFG_DRVSTR;
    }}

    /// Enable input buffer
    static inline void enableInput() {{
        getPortGroup()->PINCFG[PIN] |= hardware::PORT_PINCFG_INEN;
    }}

    /// Disable input buffer
    static inline void disableInput() {{
        getPortGroup()->PINCFG[PIN] &= ~hardware::PORT_PINCFG_INEN;
    }}
}};

// Re-export from sub-namespaces for convenience
using namespace pins;

}}  // namespace alloy::hal::atmel::samd21::{device_name.lower()}
"""

    return content


def generate_variant(device_name: str, variant_config: Dict) -> None:
    """Generate all headers for a specific MCU variant"""

    print(f"\n🔧 Generating {device_name} (SAMD21)...")

    # Use centralized config
    vendor = normalize_vendor("Atmel")
    family = detect_family(device_name, vendor)

    # Create output directory using centralized path management
    device_dir = ensure_dir(get_mcu_output_dir(vendor, family, device_name.lower()))

    # Get progress tracker
    tracker = get_global_tracker()
    # Note: Removed gpio.hpp, hardware.hpp - no longer needed with family-level architecture
    expected_files = ["pins.hpp"]

    if tracker:
        tracker.add_mcu_task(vendor, family, device_name.lower(), expected_files)
        tracker.mark_mcu_generating(vendor, family, device_name.lower())

    # Generate headers (only MCU-specific files)
    headers = {
        "pins.hpp": generate_pins_header(device_name, variant_config),
    }

    success = True
    for filename, content in headers.items():
        try:
            if tracker:
                tracker.mark_file_generating(vendor, family, device_name.lower(), filename)

            file_path = device_dir / filename
            file_path.write_text(content)
            print(f"  ✅ {filename}")

            if tracker:
                tracker.mark_file_success(vendor, family, device_name.lower(), filename, file_path)
        except Exception as e:
            print(f"  ❌ {filename}: {e}")
            if tracker:
                tracker.mark_file_failed(vendor, family, device_name.lower(), filename, str(e))
            success = False

    if tracker:
        tracker.complete_mcu_generation(vendor, family, device_name.lower(), success)

    print(f"  📦 Package: {variant_config['package']}")
    print(f"  💾 Flash: {variant_config['flash_kb']} KB")
    print(f"  🧠 RAM: {variant_config['ram_kb']} KB")

    # Count total pins
    total_pins = sum(len(pins) for pins in variant_config['ports'].values())
    print(f"  📌 Pins: {total_pins}")


def main():
    """Main entry point"""
    print("=" * 80)
    print("🚀 Alloy SAMD21 Pin Header Generator")
    print("=" * 80)

    # Set generator ID for manifest tracking
    from cli.core.progress import get_global_tracker
    tracker = get_global_tracker()
    if tracker:
        tracker.set_generator("samd21_pins")

    # Generate for all variants
    for device_name, variant_config in SAMD21_VARIANTS.items():
        generate_variant(device_name, variant_config)

    # Copy the PORT HAL template to the SAMD21 directory
    from cli.core.paths import get_family_dir
    samd21_dir = get_family_dir("microchip", "samd21")
    port_hal_template = Path(__file__).parent / "port_hal_template.hpp"
    port_hal_dest = samd21_dir / "port_hal.hpp"

    if port_hal_template.exists():
        import shutil
        shutil.copy(port_hal_template, port_hal_dest)
        print(f"\n📋 Copied PORT HAL template to {port_hal_dest}")

    print(f"\n✅ Generated {len(SAMD21_VARIANTS)} SAMD21 variants")
    print(f"📁 Output: {samd21_dir}")
    print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
