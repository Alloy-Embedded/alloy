#!/usr/bin/env python3
"""
Generate Pin Headers for Atmel SAME70 Family

This script generates complete pin definition headers for SAME70 MCUs.
Architecture: PIO (Parallel I/O) controller with 4 alternate functions (A, B, C, D)
"""

from pathlib import Path
from typing import Dict, List
import sys

# Add codegen directory to path for imports
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

# Import the pin function database
try:
    from cli.vendors.atmel.same70_pin_functions import (
        get_pin_functions,
        get_all_pins,
        PinFunction
    )
except ModuleNotFoundError:
    # If running directly, use absolute import
    from same70_pin_functions import (
        get_pin_functions,
        get_all_pins,
        PinFunction
    )

# Import centralized path utilities and config
from cli.core.paths import get_mcu_output_dir, ensure_dir
from cli.core.config import normalize_vendor, detect_family

# Import progress tracking
from cli.core.progress import get_global_tracker


# SAME70 MCU variants and their configurations
SAME70_VARIANTS = {
    "ATSAME70Q21B": {
        "package": "LQFP144",
        "flash_kb": 2048,
        "ram_kb": 384,
        "ports": {
            "A": list(range(32)),  # PA0-PA31
            "B": list(range(14)),  # PB0-PB13
            "C": list(range(32)),  # PC0-PC31
            "D": list(range(32)),  # PD0-PD31
            "E": list(range(6)),   # PE0-PE5
        }
    },
    "ATSAME70Q21": {
        "package": "LQFP144",
        "flash_kb": 2048,
        "ram_kb": 384,
        "ports": {
            "A": list(range(32)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(32)),
            "E": list(range(6)),
        }
    },
    "ATSAME70Q20B": {
        "package": "LQFP144",
        "flash_kb": 1024,
        "ram_kb": 384,
        "ports": {
            "A": list(range(32)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(32)),
            "E": list(range(6)),
        }
    },
    "ATSAME70Q19B": {
        "package": "LQFP144",
        "flash_kb": 512,
        "ram_kb": 256,
        "ports": {
            "A": list(range(32)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(32)),
            "E": list(range(6)),
        }
    },
    # SAMV71 variants (identical pinout to SAME70, different peripherals)
    "ATSAMV71Q21B": {
        "package": "LQFP144",
        "flash_kb": 2048,
        "ram_kb": 384,
        "ports": {
            "A": list(range(32)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(32)),
            "E": list(range(6)),
        }
    },
    "ATSAMV71Q21": {
        "package": "LQFP144",
        "flash_kb": 2048,
        "ram_kb": 384,
        "ports": {
            "A": list(range(32)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(32)),
            "E": list(range(6)),
        }
    },
    "ATSAMV71Q20B": {
        "package": "LQFP144",
        "flash_kb": 1024,
        "ram_kb": 384,
        "ports": {
            "A": list(range(32)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(32)),
            "E": list(range(6)),
        }
    },
    "ATSAMV71Q19B": {
        "package": "LQFP144",
        "flash_kb": 512,
        "ram_kb": 256,
        "ports": {
            "A": list(range(32)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(32)),
            "E": list(range(6)),
        }
    },
    # N variants (100-pin LQFP packages)
    "ATSAME70N21B": {
        "package": "LQFP100",
        "flash_kb": 2048,
        "ram_kb": 384,
        "ports": {
            "A": list(range(25)),  # PA0-PA24
            "B": list(range(14)),  # PB0-PB13
            "C": list(range(32)),  # PC0-PC31
            "D": list(range(28)),  # PD0-PD27
        }
    },
    "ATSAME70N20B": {
        "package": "LQFP100",
        "flash_kb": 1024,
        "ram_kb": 384,
        "ports": {
            "A": list(range(25)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(28)),
        }
    },
    "ATSAME70N19B": {
        "package": "LQFP100",
        "flash_kb": 512,
        "ram_kb": 256,
        "ports": {
            "A": list(range(25)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(28)),
        }
    },
    "ATSAMV71N21B": {
        "package": "LQFP100",
        "flash_kb": 2048,
        "ram_kb": 384,
        "ports": {
            "A": list(range(25)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(28)),
        }
    },
    "ATSAMV71N20B": {
        "package": "LQFP100",
        "flash_kb": 1024,
        "ram_kb": 384,
        "ports": {
            "A": list(range(25)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(28)),
        }
    },
    "ATSAMV71N19B": {
        "package": "LQFP100",
        "flash_kb": 512,
        "ram_kb": 256,
        "ports": {
            "A": list(range(25)),
            "B": list(range(14)),
            "C": list(range(32)),
            "D": list(range(28)),
        }
    },
    # J variants (64-pin QFP packages)
    "ATSAME70J21B": {
        "package": "LQFP64",
        "flash_kb": 2048,
        "ram_kb": 384,
        "ports": {
            "A": list(range(24)),  # PA0-PA23
            "B": list(range(4)),   # PB0-PB3
            "C": list(range(20)),  # PC0-PC19
            "D": list(range(22)),  # PD0-PD21
        }
    },
    "ATSAME70J20B": {
        "package": "LQFP64",
        "flash_kb": 1024,
        "ram_kb": 384,
        "ports": {
            "A": list(range(24)),
            "B": list(range(4)),
            "C": list(range(20)),
            "D": list(range(22)),
        }
    },
    "ATSAME70J19B": {
        "package": "LQFP64",
        "flash_kb": 512,
        "ram_kb": 256,
        "ports": {
            "A": list(range(24)),
            "B": list(range(4)),
            "C": list(range(20)),
            "D": list(range(22)),
        }
    },
    "ATSAMV71J21B": {
        "package": "LQFP64",
        "flash_kb": 2048,
        "ram_kb": 384,
        "ports": {
            "A": list(range(24)),
            "B": list(range(4)),
            "C": list(range(20)),
            "D": list(range(22)),
        }
    },
    "ATSAMV71J20B": {
        "package": "LQFP64",
        "flash_kb": 1024,
        "ram_kb": 384,
        "ports": {
            "A": list(range(24)),
            "B": list(range(4)),
            "C": list(range(20)),
            "D": list(range(22)),
        }
    },
    "ATSAMV71J19B": {
        "package": "LQFP64",
        "flash_kb": 512,
        "ram_kb": 256,
        "ports": {
            "A": list(range(24)),
            "B": list(range(4)),
            "C": list(range(20)),
            "D": list(range(22)),
        }
    },
}


def get_family_name(device_name: str) -> str:
    """Extract family name from device name (same70 or samv71)"""
    device_upper = device_name.upper()
    if device_upper.startswith("ATSAME70"):
        return "same70"
    elif device_upper.startswith("ATSAMV71"):
        return "samv71"
    else:
        return "same70"  # default


def generate_hardware_header(device_name: str, variant_config: Dict) -> str:
    """
    Generate hardware.hpp as thin adapter to generated register files.

    This replaces the old hand-written PIO_Registers with references to
    auto-generated register structures from SVD files.

    Benefits:
    - Uses auto-generated registers (always correct)
    - Eliminates ~100 lines of duplicated code per MCU
    - Maintains backward compatibility
    - Zero overhead (compile-time aliases)
    """

    family = get_family_name(device_name)

    # Determine which ports this variant has
    ports = variant_config.get('ports', {})
    port_includes = []
    port_exports = []

    for port_name in sorted(ports.keys()):
        port_lower = f"pio{port_name.lower()}"
        port_upper = f"PIO{port_name.upper()}"
        port_includes.append(f'#include "registers/{port_lower}_registers.hpp"')
        port_exports.append(f"using {port_lower}::{port_upper};")

    includes_str = '\n'.join(port_includes)
    exports_str = '\n'.join(port_exports)

    content = f"""#pragma once

#include <stdint.h>

// Include auto-generated register definitions from SVD
{includes_str}

namespace alloy::hal::atmel::{family}::{device_name.lower()}::hardware {{

// ============================================================================
// Hardware Adapter for {device_name.upper()}
//
// This file provides backward compatibility by re-exporting auto-generated
// register definitions. The actual register structures are generated from
// CMSIS-SVD files and are always correct.
// ============================================================================

// Memory map (not available in register files)
constexpr uint32_t FLASH_BASE = 0x00400000;
constexpr uint32_t FLASH_SIZE = {variant_config['flash_kb']}U * 1024U;
constexpr uint32_t SRAM_BASE  = 0x20000000;
constexpr uint32_t SRAM_SIZE  = {variant_config['ram_kb']}U * 1024U;

// ============================================================================
// Type Alias for PIO Registers
//
// Uses auto-generated type from pioa_registers.hpp
// All PIO ports (A, B, C, D, E) share the same register structure
// ============================================================================

using PIO_Registers = pioa::PIOA_Registers;

// ============================================================================
// PIO Port Instances
//
// Re-export auto-generated peripheral instances from register files
// These are constexpr pointers with correct base addresses from SVD
// ============================================================================

{exports_str}

}}  // namespace alloy::hal::atmel::{family}::{device_name.lower()}::hardware
"""

    return content


def generate_pins_header(device_name: str, variant_config: Dict) -> str:
    """Generate pins.hpp with pin number definitions"""

    family = get_family_name(device_name)

    content = f"""#pragma once

#include <stdint.h>

namespace alloy::hal::atmel::{family}::{device_name.lower()}::pins {{

// ============================================================================
// Pin Definitions for {device_name.upper()}
// Package: {variant_config['package']}
// ============================================================================

"""

    # Generate pin numbers for each port
    # Port numbering scheme: GlobalPin = (Port * 32) + Pin
    # Port A: 0-31, Port B: 32-63, Port C: 64-95, Port D: 96-127, Port E: 128-159
    port_base = {'A': 0, 'B': 32, 'C': 64, 'D': 96, 'E': 128}

    for port_name, pin_numbers in variant_config['ports'].items():
        content += f"// Port {port_name} pins\n"
        base = port_base.get(port_name, 0)
        for pin_num in pin_numbers:
            pin_name = f"P{port_name}{pin_num}"
            global_pin = base + pin_num  # Global pin number
            content += f"constexpr uint8_t {pin_name} = {global_pin};  // {port_name}{pin_num}\n"
        content += "\n"

    content += f"""
// Port base indices for pin addressing
enum class Port : uint8_t {{
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
}};

// Helper to get port from pin name (compile-time)
template<char PortChar>
constexpr Port get_port() {{
"""

    for port_name in variant_config['ports'].keys():
        content += f"    if constexpr (PortChar == '{port_name}') return Port::{port_name};\n"

    content += f"""    else static_assert(PortChar >= 'A' && PortChar <= 'E', "Invalid port");
}}

}}  // namespace alloy::hal::atmel::{family}::{device_name.lower()}::pins
"""

    return content


def generate_pin_functions_header(device_name: str, variant_config: Dict) -> str:
    """Generate pin_functions.hpp with alternate function mappings"""

    family = get_family_name(device_name)

    content = f"""#pragma once

#include <cstdint>
#include "pins.hpp"

namespace alloy::hal::atmel::{family}::{device_name.lower()}::pin_functions {{

// ============================================================================
// Pin Alternate Functions for {device_name.upper()}
// Based on SAME70 I/O Multiplexing Table
// ============================================================================

// Peripheral function selection
enum class PeripheralFunction : uint8_t {{
    PIO = 0,     // GPIO mode
    A = 1,       // Peripheral A
    B = 2,       // Peripheral B
    C = 3,       // Peripheral C
    D = 4,       // Peripheral D
}};

"""

    # Group functions by peripheral type
    peripheral_groups = {}

    for port_name, pin_numbers in variant_config['ports'].items():
        for pin_num in pin_numbers:
            pin_name = f"P{port_name}{pin_num}"
            functions = get_pin_functions(pin_name)

            for func in functions:
                if func.peripheral_type == "PIO":
                    continue  # Skip GPIO, it's default

                if func.peripheral_type not in peripheral_groups:
                    peripheral_groups[func.peripheral_type] = []

                peripheral_groups[func.peripheral_type].append((pin_name, func))

    # Generate peripheral sections
    for periph_type in sorted(peripheral_groups.keys()):
        content += f"// ============================================================================\n"
        content += f"// {periph_type} Peripheral Functions\n"
        content += f"// ============================================================================\n\n"

        pins_with_funcs = peripheral_groups[periph_type]

        for pin_name, func in sorted(pins_with_funcs, key=lambda x: x[0]):
            # Map peripheral letter to enum
            periph_enum = f"PeripheralFunction::{func.peripheral}" if func.peripheral != "PIO" else "PeripheralFunction::PIO"

            content += f"// {pin_name}: {func.function_name} (Peripheral {func.peripheral})\n"
            content += f"template<>\n"
            content += f"struct PinFunction<pins::{pin_name}, {periph_enum}> {{\n"
            content += f"    static constexpr const char* name = \"{func.function_name}\";\n"
            content += f"    static constexpr const char* peripheral_type = \"{func.peripheral_type}\";\n"
            content += f"}};\n\n"

    content += f"""
}}  // namespace alloy::hal::atmel::{family}::{device_name.lower()}::pin_functions
"""

    return content


def generate_gpio_header(device_name: str, variant_config: Dict) -> str:
    """
    Generate type-safe, zero-overhead GPIO abstraction using generated registers.

    This creates a GPIOPin template that:
    - Uses auto-generated register types from SVD
    - Is fully type-safe (compile-time checks)
    - Has zero runtime overhead (all constexpr)
    - Provides clean, high-level API
    """

    family = get_family_name(device_name)

    # Determine which ports this variant has
    ports = variant_config.get('ports', {})
    port_includes = []

    for port_name in sorted(ports.keys()):
        port_lower = f"pio{port_name.lower()}"
        port_includes.append(f'#include "registers/{port_lower}_registers.hpp"')

    includes_str = '\n'.join(port_includes)

    # Generate port mapping cases
    port_count = len(ports)
    port_cases = []
    for idx, port_name in enumerate(sorted(ports.keys())):
        port_lower = f"pio{port_name.lower()}"
        port_upper = f"PIO{port_name.upper()}"
        port_cases.append(f"    if constexpr (Port == {idx}) return {port_lower}::{port_upper};")

    port_mapping = '\n'.join(port_cases)

    content = f"""#pragma once

#include <stdint.h>

// Include auto-generated register definitions
{includes_str}

#include "hardware.hpp"
#include "pins.hpp"
#include "pin_functions.hpp"

namespace alloy::hal::atmel::{family}::{device_name.lower()} {{

// ============================================================================
// Port Register Mapping (Compile-Time)
//
// Maps port index to actual PIO register instance from generated files
// This is fully resolved at compile time with zero runtime overhead
// ============================================================================

template<uint8_t Port>
constexpr auto getPortRegister() {{
{port_mapping}
    else static_assert(Port < {port_count}, "Invalid port index");
}}

// ============================================================================
// Type-Safe GPIO Pin Abstraction
//
// Template-based GPIO abstraction using auto-generated register types.
// All operations are constexpr and inline, resulting in assembly identical
// to direct register manipulation.
//
// Features:
// - Type-safe at compile time
// - Zero runtime overhead
// - Uses auto-generated register definitions
// - Clean, high-level API
//
// Usage:
//   using Led = GPIOPin<pins::PC8>;
//   Led::configureOutput();
//   Led::set();
//   Led::toggle();
// ============================================================================

template<uint8_t GlobalPin>
class GPIOPin {{
public:
    // Compile-time port/pin calculation
    static constexpr uint8_t PORT = GlobalPin / 32;
    static constexpr uint8_t PIN = GlobalPin % 32;
    static constexpr uint32_t MASK = (1U << PIN);

    // Get port register at compile-time
    using PortRegType = decltype(getPortRegister<PORT>());
    static constexpr PortRegType PORT_REG = getPortRegister<PORT>();

    // ========================================================================
    // Pin Mode Configuration
    // ========================================================================

    /// Configure pin as GPIO output
    static inline void configureOutput() {{
        PORT_REG->PER = MASK;    // Enable PIO control
        PORT_REG->OER = MASK;    // Enable output
        PORT_REG->PUDR = MASK;   // Disable pull-up
        PORT_REG->PPDDR = MASK;  // Disable pull-down
    }}

    /// Configure pin as GPIO input
    static inline void configureInput() {{
        PORT_REG->PER = MASK;    // Enable PIO control
        PORT_REG->ODR = MASK;    // Disable output (input mode)
        PORT_REG->PUDR = MASK;   // Disable pull-up
        PORT_REG->PPDDR = MASK;  // Disable pull-down
    }}

    /// Configure pin as input with pull-up
    static inline void configureInputPullUp() {{
        PORT_REG->PER = MASK;    // Enable PIO control
        PORT_REG->ODR = MASK;    // Disable output
        PORT_REG->PUER = MASK;   // Enable pull-up
        PORT_REG->PPDDR = MASK;  // Disable pull-down
    }}

    /// Configure pin as input with pull-down
    static inline void configureInputPullDown() {{
        PORT_REG->PER = MASK;    // Enable PIO control
        PORT_REG->ODR = MASK;    // Disable output
        PORT_REG->PUDR = MASK;   // Disable pull-up
        PORT_REG->PPDER = MASK;  // Enable pull-down
    }}

    /// Configure pin as input with glitch filter
    static inline void configureInputFiltered() {{
        configureInput();
        PORT_REG->IFER = MASK;   // Enable glitch filter
    }}

    // ========================================================================
    // Digital I/O Operations
    // ========================================================================

    /// Set pin high
    static inline void set() {{
        PORT_REG->SODR = MASK;   // Set Output Data Register
    }}

    /// Set pin low
    static inline void clear() {{
        PORT_REG->CODR = MASK;   // Clear Output Data Register
    }}

    /// Toggle pin state
    static inline void toggle() {{
        if (PORT_REG->ODSR & MASK) {{
            clear();
        }} else {{
            set();
        }}
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
        return (PORT_REG->PDSR & MASK) != 0;
    }}

    // ========================================================================
    // Peripheral Function Configuration
    // ========================================================================

    enum class PeripheralFunction : uint8_t {{
        PIO = 0,  // GPIO mode
        A = 1,    // Peripheral A
        B = 2,    // Peripheral B
        C = 3,    // Peripheral C
        D = 4,    // Peripheral D
    }};

    /// Configure pin for peripheral function (A, B, C, D)
    static inline void configurePeripheral(PeripheralFunction func) {{
        if (func == PeripheralFunction::PIO) {{
            PORT_REG->PER = MASK;  // Enable PIO control
            return;
        }}

        // Disable PIO control (enable peripheral)
        PORT_REG->PDR = MASK;

        // Set peripheral function using ABCDSR registers
        // ABCDSR[0] = bit 0, ABCDSR[1] = bit 1
        // 00 = A, 01 = B, 10 = C, 11 = D
        uint8_t periph_val = static_cast<uint8_t>(func) - 1;

        if (periph_val & 0x01) {{
            PORT_REG->ABCDSR[0] |= MASK;
        }} else {{
            PORT_REG->ABCDSR[0] &= ~MASK;
        }}

        if (periph_val & 0x02) {{
            PORT_REG->ABCDSR[1] |= MASK;
        }} else {{
            PORT_REG->ABCDSR[1] &= ~MASK;
        }}
    }}

    // ========================================================================
    // Advanced Features
    // ========================================================================

    /// Enable multi-driver (open-drain) mode
    static inline void enableMultiDriver() {{
        PORT_REG->MDER = MASK;
    }}

    /// Disable multi-driver mode
    static inline void disableMultiDriver() {{
        PORT_REG->MDDR = MASK;
    }}

    /// Enable Schmitt trigger
    static inline void enableSchmitt() {{
        PORT_REG->SCHMITT |= MASK;
    }}

    /// Disable Schmitt trigger
    static inline void disableSchmitt() {{
        PORT_REG->SCHMITT &= ~MASK;
    }}

    // ========================================================================
    // Interrupt Configuration
    // ========================================================================

    enum class InterruptMode : uint8_t {{
        Disabled = 0,
        RisingEdge = 1,
        FallingEdge = 2,
        BothEdges = 3,
        LowLevel = 4,
        HighLevel = 5,
    }};

    /// Configure interrupt mode
    static inline void configureInterrupt(InterruptMode mode) {{
        if (mode == InterruptMode::Disabled) {{
            PORT_REG->IDR = MASK;
            return;
        }}

        PORT_REG->AIMER = MASK;  // Enable additional interrupt modes

        switch (mode) {{
            case InterruptMode::RisingEdge:
                PORT_REG->ESR = MASK;      // Edge select
                PORT_REG->REHLSR = MASK;   // Rising edge
                break;

            case InterruptMode::FallingEdge:
                PORT_REG->ESR = MASK;      // Edge select
                PORT_REG->FELLSR = MASK;   // Falling edge
                break;

            case InterruptMode::BothEdges:
                PORT_REG->ESR = MASK;      // Edge select
                break;

            case InterruptMode::LowLevel:
                PORT_REG->LSR = MASK;      // Level select
                PORT_REG->FELLSR = MASK;   // Low level
                break;

            case InterruptMode::HighLevel:
                PORT_REG->LSR = MASK;      // Level select
                PORT_REG->REHLSR = MASK;   // High level
                break;

            default:
                break;
        }}

        PORT_REG->IER = MASK;  // Enable interrupt
    }}

    /// Check if interrupt is pending
    [[nodiscard]] static inline bool isInterruptPending() {{
        return (PORT_REG->ISR & MASK) != 0;
    }}
}};

// Re-export from sub-namespaces for convenience
using namespace pins;
using namespace pin_functions;

}}  // namespace alloy::hal::atmel::{family}::{device_name.lower()}
"""

    return content


def generate_variant(device_name: str, variant_config: Dict) -> None:
    """Generate all headers for a specific MCU variant"""

    family = get_family_name(device_name)
    print(f"\n🔧 Generating {device_name} ({family.upper()})...")

    # Create output directory using centralized path management
    device_dir = ensure_dir(get_mcu_output_dir("atmel", family, device_name.lower()))

    # Get tracker
    tracker = get_global_tracker()
    vendor = "atmel"

    # Add MCU task to tracker
    # Note: Removed gpio.hpp, hardware.hpp, peripherals.hpp - no longer needed with family-level architecture
    expected_files = ["pins.hpp", "pin_functions.hpp"]
    if tracker:
        tracker.add_mcu_task(vendor, family, device_name.lower(), expected_files)
        tracker.mark_mcu_generating(vendor, family, device_name.lower())

    # Generate headers (only MCU-specific files)
    headers = {
        "pins.hpp": generate_pins_header(device_name, variant_config),
        "pin_functions.hpp": generate_pin_functions_header(device_name, variant_config),
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


def main_same70():
    """Main entry point for SAME70/SAMV71 generation"""
    print("=" * 80)
    print("🚀 Alloy SAME70/SAMV71 Pin Header Generator")
    print("=" * 80)

    # Set generator ID for manifest tracking
    from cli.core.progress import get_global_tracker
    tracker = get_global_tracker()
    if tracker:
        tracker.set_generator("same70_pins")

    # Generate for all variants
    families_generated = set()
    for device_name, variant_config in SAME70_VARIANTS.items():
        generate_variant(device_name, variant_config)
        families_generated.add(get_family_name(device_name))

    # Copy the PIO HAL template to each family directory
    pio_hal_template = Path(__file__).parent / "pio_hal_template.hpp"
    if pio_hal_template.exists():
        import shutil
        from cli.core.paths import get_family_dir
        for family in families_generated:
            family_dir = get_family_dir("microchip", family)
            pio_hal_dest = family_dir / "pio_hal.hpp"
            shutil.copy(pio_hal_template, pio_hal_dest)
            print(f"\n📋 Copied PIO HAL template to {pio_hal_dest}")

    # Count variants per family
    same70_count = sum(1 for name in SAME70_VARIANTS.keys() if get_family_name(name) == "same70")
    samv71_count = sum(1 for name in SAME70_VARIANTS.keys() if get_family_name(name) == "samv71")

    print(f"\n✅ Generated {len(SAME70_VARIANTS)} total variants:")
    print(f"   • SAME70: {same70_count} variants")
    print(f"   • SAMV71: {samv71_count} variants")
    from cli.core.paths import get_vendor_dir
    print(f"📁 Output: {get_vendor_dir('microchip')}")
    print()

    return 0


def main():
    """Legacy main for backward compatibility"""
    return main_same70()


if __name__ == "__main__":
    sys.exit(main())
