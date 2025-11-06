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
    """Generate hardware.hpp with PIO register definitions"""

    family = get_family_name(device_name)

    content = f"""#pragma once

#include <cstdint>

namespace alloy::hal::atmel::{family}::{device_name.lower()}::hardware {{

// ============================================================================
// Hardware Register Definitions for {device_name.upper()}
// Based on SAME70 PIO (Parallel I/O) Controller Architecture
// ============================================================================

// Memory map
constexpr uint32_t FLASH_BASE = 0x00400000;
constexpr uint32_t FLASH_SIZE = {variant_config['flash_kb']}U * 1024U;
constexpr uint32_t SRAM_BASE  = 0x20000000;
constexpr uint32_t SRAM_SIZE  = {variant_config['ram_kb']}U * 1024U;

// PIO Controller base addresses
constexpr uint32_t PIOA_BASE = 0x400E0E00;
constexpr uint32_t PIOB_BASE = 0x400E1000;
constexpr uint32_t PIOC_BASE = 0x400E1200;
constexpr uint32_t PIOD_BASE = 0x400E1400;
constexpr uint32_t PIOE_BASE = 0x400E1600;

// ============================================================================
// PIO Register Structure (SAME70 Architecture)
// ============================================================================

struct PIO_Registers {{
    volatile uint32_t PER;         // 0x00: PIO Enable Register
    volatile uint32_t PDR;         // 0x04: PIO Disable Register
    volatile uint32_t PSR;         // 0x08: PIO Status Register
    uint32_t RESERVED1;            // 0x0C: Reserved
    volatile uint32_t OER;         // 0x10: Output Enable Register
    volatile uint32_t ODR;         // 0x14: Output Disable Register
    volatile uint32_t OSR;         // 0x18: Output Status Register
    uint32_t RESERVED2;            // 0x1C: Reserved
    volatile uint32_t IFER;        // 0x20: Glitch Input Filter Enable
    volatile uint32_t IFDR;        // 0x24: Glitch Input Filter Disable
    volatile uint32_t IFSR;        // 0x28: Glitch Input Filter Status
    uint32_t RESERVED3;            // 0x2C: Reserved
    volatile uint32_t SODR;        // 0x30: Set Output Data Register
    volatile uint32_t CODR;        // 0x34: Clear Output Data Register
    volatile uint32_t ODSR;        // 0x38: Output Data Status Register
    volatile uint32_t PDSR;        // 0x3C: Pin Data Status Register
    volatile uint32_t IER;         // 0x40: Interrupt Enable Register
    volatile uint32_t IDR;         // 0x44: Interrupt Disable Register
    volatile uint32_t IMR;         // 0x48: Interrupt Mask Register
    volatile uint32_t ISR;         // 0x4C: Interrupt Status Register
    volatile uint32_t MDER;        // 0x50: Multi-driver Enable Register
    volatile uint32_t MDDR;        // 0x54: Multi-driver Disable Register
    volatile uint32_t MDSR;        // 0x58: Multi-driver Status Register
    uint32_t RESERVED4;            // 0x5C: Reserved
    volatile uint32_t PUDR;        // 0x60: Pull-up Disable Register
    volatile uint32_t PUER;        // 0x64: Pull-up Enable Register
    volatile uint32_t PUSR;        // 0x68: Pull-up Status Register
    uint32_t RESERVED5;            // 0x6C: Reserved
    volatile uint32_t ABCDSR[2];   // 0x70-0x74: Peripheral ABCD Select Register
    uint32_t RESERVED6[2];         // 0x78-0x7C: Reserved
    volatile uint32_t IFSCDR;      // 0x80: Input Filter Slow Clock Disable
    volatile uint32_t IFSCER;      // 0x84: Input Filter Slow Clock Enable
    volatile uint32_t IFSCSR;      // 0x88: Input Filter Slow Clock Status
    volatile uint32_t SCDR;        // 0x8C: Slow Clock Divider Debouncing
    volatile uint32_t PPDDR;       // 0x90: Pad Pull-down Disable Register
    volatile uint32_t PPDER;       // 0x94: Pad Pull-down Enable Register
    volatile uint32_t PPDSR;       // 0x98: Pad Pull-down Status Register
    uint32_t RESERVED7;            // 0x9C: Reserved
    volatile uint32_t OWER;        // 0xA0: Output Write Enable
    volatile uint32_t OWDR;        // 0xA4: Output Write Disable
    volatile uint32_t OWSR;        // 0xA8: Output Write Status Register
    uint32_t RESERVED8;            // 0xAC: Reserved
    volatile uint32_t AIMER;       // 0xB0: Additional Interrupt Modes Enable
    volatile uint32_t AIMDR;       // 0xB4: Additional Interrupt Modes Disable
    volatile uint32_t AIMMR;       // 0xB8: Additional Interrupt Modes Mask
    uint32_t RESERVED9;            // 0xBC: Reserved
    volatile uint32_t ESR;         // 0xC0: Edge Select Register
    volatile uint32_t LSR;         // 0xC4: Level Select Register
    volatile uint32_t ELSR;        // 0xC8: Edge/Level Status Register
    uint32_t RESERVED10;           // 0xCC: Reserved
    volatile uint32_t FELLSR;      // 0xD0: Falling Edge/Low-Level Select
    volatile uint32_t REHLSR;      // 0xD4: Rising Edge/High-Level Select
    volatile uint32_t FRLHSR;      // 0xD8: Fall/Rise - Low/High Status
    uint32_t RESERVED11;           // 0xDC: Reserved
    volatile uint32_t LOCKSR;      // 0xE0: Lock Status
    volatile uint32_t WPMR;        // 0xE4: Write Protection Mode Register
    volatile uint32_t WPSR;        // 0xE8: Write Protection Status Register
    uint32_t RESERVED12[5];        // 0xEC-0xFC: Reserved
    volatile uint32_t SCHMITT;     // 0x100: Schmitt Trigger Register
    uint32_t RESERVED13[5];        // 0x104-0x114: Reserved
    volatile uint32_t DRIVER;      // 0x118: I/O Drive Register
    uint32_t RESERVED14[13];       // 0x11C-0x14C: Reserved
    volatile uint32_t PCMR;        // 0x150: Parallel Capture Mode Register
    volatile uint32_t PCIER;       // 0x154: Parallel Capture Interrupt Enable
    volatile uint32_t PCIDR;       // 0x158: Parallel Capture Interrupt Disable
    volatile uint32_t PCIMR;       // 0x15C: Parallel Capture Interrupt Mask
    volatile uint32_t PCISR;       // 0x160: Parallel Capture Interrupt Status
    volatile uint32_t PCRHR;       // 0x164: Parallel Capture Reception Holding
}};

// PIO port instances
static_assert(sizeof(PIO_Registers) >= 0x164, "PIO_Registers size mismatch");

inline PIO_Registers* PIOA = reinterpret_cast<PIO_Registers*>(PIOA_BASE);
inline PIO_Registers* PIOB = reinterpret_cast<PIO_Registers*>(PIOB_BASE);
inline PIO_Registers* PIOC = reinterpret_cast<PIO_Registers*>(PIOC_BASE);
inline PIO_Registers* PIOD = reinterpret_cast<PIO_Registers*>(PIOD_BASE);
inline PIO_Registers* PIOE = reinterpret_cast<PIO_Registers*>(PIOE_BASE);

}}  // namespace alloy::hal::atmel::{family}::{device_name.lower()}::hardware
"""

    return content


def generate_pins_header(device_name: str, variant_config: Dict) -> str:
    """Generate pins.hpp with pin number definitions"""

    family = get_family_name(device_name)

    content = f"""#pragma once

#include <cstdint>

namespace alloy::hal::atmel::{family}::{device_name.lower()}::pins {{

// ============================================================================
// Pin Definitions for {device_name.upper()}
// Package: {variant_config['package']}
// ============================================================================

"""

    # Generate pin numbers for each port
    for port_name, pin_numbers in variant_config['ports'].items():
        content += f"// Port {port_name} pins\n"
        for pin_num in pin_numbers:
            pin_name = f"P{port_name}{pin_num}"
            content += f"constexpr uint8_t {pin_name} = {pin_num};  // {port_name}{pin_num}\n"
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


def generate_gpio_header(device_name: str) -> str:
    """Generate main gpio.hpp that includes all pin headers"""

    family = get_family_name(device_name)

    content = f"""#pragma once

#include "hardware.hpp"
#include "pins.hpp"
#include "pin_functions.hpp"
#include "../pio_hal.hpp"

namespace alloy::hal::atmel::{family}::{device_name.lower()} {{

// Re-export from sub-namespaces for convenience
using namespace hardware;
using namespace pins;
using namespace pin_functions;

// Use the {family.upper()} PIO HAL
template<uint8_t Pin>
using GPIOPin = {family}::PIOPin<hardware::PIO_Registers, Pin>;

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
    expected_files = ["hardware.hpp", "pins.hpp", "pin_functions.hpp", "gpio.hpp"]
    if tracker:
        tracker.add_mcu_task(vendor, family, device_name.lower(), expected_files)
        tracker.mark_mcu_generating(vendor, family, device_name.lower())

    # Generate headers
    headers = {
        "hardware.hpp": generate_hardware_header(device_name, variant_config),
        "pins.hpp": generate_pins_header(device_name, variant_config),
        "pin_functions.hpp": generate_pin_functions_header(device_name, variant_config),
        "gpio.hpp": generate_gpio_header(device_name),
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
