#!/usr/bin/env python3
"""
Generate Pin Headers for Raspberry Pi RP2040

RP2040 Architecture:
- Dual Cortex-M0+ @ 133MHz
- 30 GPIO pins (GPIO0-GPIO29)
- SIO (Single-cycle I/O) for GPIO control
- 9 functions per pin via FUNCSEL
"""

from pathlib import Path
import sys

# Add codegen directory to path
CODEGEN_DIR = Path(__file__).parent.parent.parent
sys.path.insert(0, str(CODEGEN_DIR))

REPO_ROOT = Path(__file__).parent.parent.parent.parent.parent.parent
OUTPUT_DIR = REPO_ROOT / "src" / "hal" / "vendors"


def generate_hardware_header() -> str:
    """Generate hardware.hpp with SIO register definitions"""

    content = """#pragma once

#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040::hardware {

// ============================================================================
// Hardware Register Definitions for RP2040
// Based on RP2040 Datasheet - SIO and PADS
// ============================================================================

// Memory map
constexpr uint32_t FLASH_BASE = 0x10000000;
constexpr uint32_t SRAM_BASE  = 0x20000000;
constexpr uint32_t SRAM_SIZE  = 264U * 1024U;

// SIO (Single-cycle I/O) base address
constexpr uint32_t SIO_BASE = 0xD0000000;

// IO Bank 0 base address (for FUNCSEL and other config)
constexpr uint32_t IO_BANK0_BASE = 0x40014000;

// PADS Bank 0 base address (for pull-up/down, drive strength)
constexpr uint32_t PADS_BANK0_BASE = 0x4001C000;

// ============================================================================
// SIO Register Structure
// ============================================================================

struct SIO_Registers {
    volatile uint32_t CPUID;              // 0x00: Processor core identifier
    volatile uint32_t GPIO_IN;            // 0x04: Input value for GPIO0-29
    volatile uint32_t GPIO_HI_IN;         // 0x08: Input value for GPIO30-47 (QSPI)
    uint32_t RESERVED0;                   // 0x0C
    volatile uint32_t GPIO_OUT;           // 0x10: GPIO output value
    volatile uint32_t GPIO_OUT_SET;       // 0x14: GPIO output value set
    volatile uint32_t GPIO_OUT_CLR;       // 0x18: GPIO output value clear
    volatile uint32_t GPIO_OUT_XOR;       // 0x1C: GPIO output value XOR
    volatile uint32_t GPIO_OE;            // 0x20: GPIO output enable
    volatile uint32_t GPIO_OE_SET;        // 0x24: GPIO output enable set
    volatile uint32_t GPIO_OE_CLR;        // 0x28: GPIO output enable clear
    volatile uint32_t GPIO_OE_XOR;        // 0x2C: GPIO output enable XOR
};

// IO Bank 0 - per GPIO config
struct IO_BANK0_GPIO_CTRL {
    volatile uint32_t STATUS;             // GPIO status
    volatile uint32_t CTRL;               // GPIO control (includes FUNCSEL)
};

struct IO_BANK0_Registers {
    IO_BANK0_GPIO_CTRL GPIO[30];          // GPIO0-GPIO29 control
};

// PADS Bank 0 - per GPIO pad config
struct PADS_BANK0_Registers {
    volatile uint32_t VOLTAGE_SELECT;     // 0x00: Voltage select
    volatile uint32_t GPIO[30];           // 0x04+: GPIO0-GPIO29 pad control
};

// SIO instance
inline SIO_Registers* SIO = reinterpret_cast<SIO_Registers*>(SIO_BASE);
inline IO_BANK0_Registers* IO_BANK0 = reinterpret_cast<IO_BANK0_Registers*>(IO_BANK0_BASE);
inline PADS_BANK0_Registers* PADS_BANK0 = reinterpret_cast<PADS_BANK0_Registers*>(PADS_BANK0_BASE);

// FUNCSEL values for IO_BANK0 CTRL register
constexpr uint32_t FUNCSEL_SPI    = 1;
constexpr uint32_t FUNCSEL_UART   = 2;
constexpr uint32_t FUNCSEL_I2C    = 3;
constexpr uint32_t FUNCSEL_PWM    = 4;
constexpr uint32_t FUNCSEL_SIO    = 5;  // GPIO
constexpr uint32_t FUNCSEL_PIO0   = 6;
constexpr uint32_t FUNCSEL_PIO1   = 7;
constexpr uint32_t FUNCSEL_CLOCK  = 8;
constexpr uint32_t FUNCSEL_USB    = 9;
constexpr uint32_t FUNCSEL_NULL   = 31;

// PADS control bits
constexpr uint32_t PADS_SLEWFAST  = (1 << 0);
constexpr uint32_t PADS_SCHMITT   = (1 << 1);
constexpr uint32_t PADS_PDE       = (1 << 2);  // Pull-down enable
constexpr uint32_t PADS_PUE       = (1 << 3);  // Pull-up enable
constexpr uint32_t PADS_DRIVE_2MA = (0 << 4);
constexpr uint32_t PADS_DRIVE_4MA = (1 << 4);
constexpr uint32_t PADS_DRIVE_8MA = (2 << 4);
constexpr uint32_t PADS_DRIVE_12MA = (3 << 4);
constexpr uint32_t PADS_IE        = (1 << 6);  // Input enable
constexpr uint32_t PADS_OD        = (1 << 7);  // Output disable

}  // namespace alloy::hal::raspberrypi::rp2040::hardware
"""

    return content


def generate_pins_header() -> str:
    """Generate pins.hpp with pin definitions"""

    content = """#pragma once

#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040::pins {

// ============================================================================
// Pin Definitions for RP2040
// ============================================================================

// GPIO pins
constexpr uint8_t GPIO0  = 0;
constexpr uint8_t GPIO1  = 1;
constexpr uint8_t GPIO2  = 2;
constexpr uint8_t GPIO3  = 3;
constexpr uint8_t GPIO4  = 4;
constexpr uint8_t GPIO5  = 5;
constexpr uint8_t GPIO6  = 6;
constexpr uint8_t GPIO7  = 7;
constexpr uint8_t GPIO8  = 8;
constexpr uint8_t GPIO9  = 9;
constexpr uint8_t GPIO10 = 10;
constexpr uint8_t GPIO11 = 11;
constexpr uint8_t GPIO12 = 12;
constexpr uint8_t GPIO13 = 13;
constexpr uint8_t GPIO14 = 14;
constexpr uint8_t GPIO15 = 15;
constexpr uint8_t GPIO16 = 16;
constexpr uint8_t GPIO17 = 17;
constexpr uint8_t GPIO18 = 18;
constexpr uint8_t GPIO19 = 19;
constexpr uint8_t GPIO20 = 20;
constexpr uint8_t GPIO21 = 21;
constexpr uint8_t GPIO22 = 22;
constexpr uint8_t GPIO23 = 23;
constexpr uint8_t GPIO24 = 24;
constexpr uint8_t GPIO25 = 25;  // On-board LED on Pico
constexpr uint8_t GPIO26 = 26;  // ADC0
constexpr uint8_t GPIO27 = 27;  // ADC1
constexpr uint8_t GPIO28 = 28;  // ADC2
constexpr uint8_t GPIO29 = 29;  // ADC3

// Convenience aliases for Pico board
constexpr uint8_t LED = GPIO25;

}  // namespace alloy::hal::raspberrypi::rp2040::pins
"""

    return content


def generate_gpio_header() -> str:
    """Generate main gpio.hpp"""

    content = """#pragma once

#include "hardware.hpp"
#include "pins.hpp"
#include "../sio_hal.hpp"

namespace alloy::hal::raspberrypi::rp2040 {

// Re-export from sub-namespaces for convenience
using namespace hardware;
using namespace pins;

// Use the RP2040 SIO HAL
template<uint8_t Pin>
using GPIOPin = rp2040::SIOPin<hardware::SIO_Registers, Pin>;

}  // namespace alloy::hal::raspberrypi::rp2040
"""

    return content


def main():
    """Main entry point"""
    print("=" * 80)
    print("🚀 Alloy RP2040 Pin Header Generator")
    print("=" * 80)

    print("\n🔧 Generating RP2040...")

    # Create output directory
    rp2040_dir = OUTPUT_DIR / "raspberrypi" / "rp2040"
    rp2040_dir.mkdir(parents=True, exist_ok=True)

    # Generate headers
    headers = {
        "hardware.hpp": generate_hardware_header(),
        "pins.hpp": generate_pins_header(),
        "gpio.hpp": generate_gpio_header(),
    }

    for filename, content in headers.items():
        file_path = rp2040_dir / filename
        file_path.write_text(content)
        print(f"  ✅ {filename}")

    # Copy SIO HAL template
    sio_hal_template = Path(__file__).parent / "sio_hal_template.hpp"
    sio_hal_dest = (OUTPUT_DIR / "raspberrypi" / "rp2040").parent / "sio_hal.hpp"

    if sio_hal_template.exists():
        import shutil
        shutil.copy(sio_hal_template, sio_hal_dest)
        print(f"\n📋 Copied SIO HAL template to {sio_hal_dest}")

    print(f"\n✅ Generated RP2040 (30 GPIO pins)")
    print(f"📁 Output: {OUTPUT_DIR / 'raspberrypi' / 'rp2040'}")
    print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
