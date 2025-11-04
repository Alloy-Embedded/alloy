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

REPO_ROOT = Path(__file__).parent.parent.parent.parent.parent.parent
OUTPUT_DIR = REPO_ROOT / "src" / "hal" / "vendors"


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


def generate_gpio_header(device_name: str) -> str:
    """Generate main gpio.hpp that includes all pin headers"""

    content = f"""#pragma once

#include "hardware.hpp"
#include "pins.hpp"
#include "../port_hal.hpp"

namespace alloy::hal::atmel::samd21::{device_name.lower()} {{

// Re-export from sub-namespaces for convenience
using namespace hardware;
using namespace pins;

// Use the SAMD21 PORT HAL
template<uint8_t PortIndex, uint8_t Pin>
using GPIOPin = samd21::PORTPin<hardware::PORT_Registers, PortIndex, Pin>;

}}  // namespace alloy::hal::atmel::samd21::{device_name.lower()}
"""

    return content


def generate_variant(device_name: str, variant_config: Dict) -> None:
    """Generate all headers for a specific MCU variant"""

    print(f"\n🔧 Generating {device_name} (SAMD21)...")

    # Create output directory
    device_dir = OUTPUT_DIR / "atmel" / "samd21" / device_name.lower()
    device_dir.mkdir(parents=True, exist_ok=True)

    # Generate headers
    headers = {
        "hardware.hpp": generate_hardware_header(device_name, variant_config),
        "pins.hpp": generate_pins_header(device_name, variant_config),
        "gpio.hpp": generate_gpio_header(device_name),
    }

    for filename, content in headers.items():
        file_path = device_dir / filename
        file_path.write_text(content)
        print(f"  ✅ {filename}")

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

    # Generate for all variants
    for device_name, variant_config in SAMD21_VARIANTS.items():
        generate_variant(device_name, variant_config)

    # Copy the PORT HAL template to the SAMD21 directory
    samd21_dir = OUTPUT_DIR / "atmel" / "samd21"
    port_hal_template = Path(__file__).parent / "port_hal_template.hpp"
    port_hal_dest = samd21_dir / "port_hal.hpp"

    if port_hal_template.exists():
        import shutil
        shutil.copy(port_hal_template, port_hal_dest)
        print(f"\n📋 Copied PORT HAL template to {port_hal_dest}")

    print(f"\n✅ Generated {len(SAMD21_VARIANTS)} SAMD21 variants")
    print(f"📁 Output: {OUTPUT_DIR / 'atmel' / 'samd21'}")
    print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
