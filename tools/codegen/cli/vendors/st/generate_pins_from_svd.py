#!/usr/bin/env python3
"""
Generate pin headers from SVD files - Full implementation

This script extracts MCU information from SVD files and generates
complete pin definition headers with compile-time validation.
"""

from pathlib import Path
from typing import Dict, List
import sys

from cli.parsers.svd_pin_extractor import extract_mcu_info_from_svd, MCUInfo, MCUPackageInfo
from cli.parsers.svd_discovery import discover_all_svds
from cli.core.paths import get_mcu_output_dir, get_family_dir, ensure_dir, HAL_VENDORS_DIR
import xml.etree.ElementTree as ET


# Package configurations for known MCU variants
# This supplements SVD data with package-specific pin availability
VARIANT_PACKAGES = {
    # STM32F103 variants
    'STM32F103C4': {'code': 'C', 'flash_kb': 16, 'ram_kb': 6},
    'STM32F103C6': {'code': 'C', 'flash_kb': 32, 'ram_kb': 10},
    'STM32F103C8': {'code': 'C', 'flash_kb': 64, 'ram_kb': 20},
    'STM32F103CB': {'code': 'C', 'flash_kb': 128, 'ram_kb': 20},

    'STM32F103R4': {'code': 'R', 'flash_kb': 16, 'ram_kb': 6},
    'STM32F103R6': {'code': 'R', 'flash_kb': 32, 'ram_kb': 10},
    'STM32F103R8': {'code': 'R', 'flash_kb': 64, 'ram_kb': 20},
    'STM32F103RB': {'code': 'R', 'flash_kb': 128, 'ram_kb': 20},
    'STM32F103RC': {'code': 'R', 'flash_kb': 256, 'ram_kb': 48},
    'STM32F103RD': {'code': 'R', 'flash_kb': 384, 'ram_kb': 64},
    'STM32F103RE': {'code': 'R', 'flash_kb': 512, 'ram_kb': 64},
}


def apply_variant_config(base_mcu: MCUInfo, variant_name: str) -> MCUInfo:
    """Apply variant-specific configuration to base MCU info"""

    if variant_name not in VARIANT_PACKAGES:
        # Unknown variant, return base
        return base_mcu

    config = VARIANT_PACKAGES[variant_name]

    # Update memory
    base_mcu.flash_kb = config['flash_kb']
    base_mcu.sram_kb = config['ram_kb']  # Note: config uses 'ram_kb' but MCUInfo uses 'sram_kb'
    base_mcu.device_name = variant_name

    # Fix vendor mapping for STM32 devices
    if base_mcu.vendor == 'Unknown' and variant_name.startswith('STM32'):
        base_mcu.vendor = 'STMicroelectronics'

    # Update package based on code
    package_code = config['code']

    if package_code == 'C':  # LQFP48
        package = MCUPackageInfo(
            package_name='LQFP48',
            total_pins=48,
            available_ports={
                'A': list(range(16)),  # PA0-PA15 (all)
                'B': list(range(16)),  # PB0-PB15 (all)
                'C': [13, 14, 15],     # PC13-PC15 only
                'D': [0, 1],           # PD0-PD1 only (OSC)
            }
        )
        base_mcu.packages = [package]

    elif package_code == 'R':  # LQFP64
        package = MCUPackageInfo(
            package_name='LQFP64',
            total_pins=64,
            available_ports={
                'A': list(range(16)),  # PA0-PA15 (all)
                'B': list(range(16)),  # PB0-PB15 (all)
                'C': list(range(16)),  # PC0-PC15 (all)
                'D': [0, 1, 2],        # PD0-PD2
                'E': list(range(16)) if 'E' in base_mcu.gpio_ports else [],
            }
        )
        # Remove empty ports
        package.available_ports = {k: v for k, v in package.available_ports.items() if v}
        base_mcu.packages = [package]

    return base_mcu


def generate_pins_header(mcu_info: MCUInfo, package: MCUPackageInfo) -> str:
    """Generate pins.hpp content"""

    # Map port letters to base values
    port_bases = {'A': 0, 'B': 16, 'C': 32, 'D': 48, 'E': 64, 'F': 80, 'G': 96}

    # Generate pin list
    pin_definitions = []
    for port_name in sorted(package.available_ports.keys()):
        pins = package.available_ports[port_name]
        port_base = port_bases.get(port_name, 0)

        for pin_num in pins:
            pin_name = f"P{port_name}{pin_num}"
            pin_value = port_base + pin_num
            pin_definitions.append((pin_name, pin_value, port_name, pin_num))

    pin_definitions.sort(key=lambda x: x[1])

    # Generate header
    content = f"""#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()}::pins {{

// ============================================================================
// Auto-generated pin definitions for {mcu_info.device_name}
// Generated from SVD: {mcu_info.series}
// Package: {package.package_name}
// GPIO Pins: {len(pin_definitions)}
// ============================================================================

"""

    # Generate pin constants grouped by port
    current_port = None
    for pin_name, pin_value, port, pin_num in pin_definitions:
        if port != current_port:
            if current_port is not None:
                content += "\n"
            content += f"// Port {port} pins ({len([p for p in pin_definitions if p[2] == port])} pins)\n"
            current_port = port

        content += f"constexpr uint8_t {pin_name:<6} = {pin_value};\n"

    # Generate validation
    valid_pins = [str(p[1]) for p in pin_definitions]
    content += f"""

// ============================================================================
// Compile-time pin validation
// ============================================================================

namespace detail {{
    // Lookup table for valid pins (constexpr array)
    constexpr uint8_t valid_pins[] = {{
        {", ".join(valid_pins)}
    }};

    constexpr size_t PIN_COUNT = sizeof(valid_pins) / sizeof(valid_pins[0]);

    // Check if pin exists in valid_pins array (constexpr)
    constexpr bool is_valid_pin(uint8_t pin) {{
        for (size_t i = 0; i < PIN_COUNT; ++i) {{
            if (valid_pins[i] == pin) return true;
        }}
        return false;
    }}
}}

// Type trait for compile-time validation
template<uint8_t Pin>
inline constexpr bool is_valid_pin_v = detail::is_valid_pin(Pin);

// C++20 concept for compile-time pin validation
template<uint8_t Pin>
concept ValidPin = is_valid_pin_v<Pin>;

// Helper to trigger compile error with clear message
template<uint8_t Pin>
constexpr void validate_pin() {{
    static_assert(ValidPin<Pin>,
        "Invalid pin for {mcu_info.device_name} ({package.package_name})! "
        "Check the datasheet for available pins on this package.");
}}

// Constants
constexpr size_t TOTAL_PIN_COUNT = detail::PIN_COUNT;  // {len(pin_definitions)} pins
constexpr size_t GPIO_PORT_COUNT = {len(package.available_ports)};  // {len(package.available_ports)} ports

}}  // namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()}::pins
"""

    return content


def extract_hardware_addresses(svd_file: Path) -> Dict[str, str]:
    """Extract peripheral base addresses from SVD file"""
    tree = ET.parse(svd_file)
    root = tree.getroot()

    addresses = {}

    # Extract GPIO addresses
    for peripheral in root.findall('.//peripheral'):
        name_elem = peripheral.find('name')
        base_elem = peripheral.find('baseAddress')

        if name_elem is not None and base_elem is not None:
            name = name_elem.text
            if name.startswith('GPIO') or name == 'RCC' or name == 'AFIO':
                addresses[name] = base_elem.text

    return addresses


def generate_hardware_header(mcu_info: MCUInfo, addresses: Dict[str, str]) -> str:
    """Generate hardware.hpp with register addresses from SVD"""

    # Detect architecture based on family
    family_upper = mcu_info.family.upper()
    uses_modern_gpio = (
        "STM32F4" in family_upper or "STM32F2" in family_upper or
        "STM32F7" in family_upper or "STM32L4" in family_upper or
        "STM32G4" in family_upper or "STM32H7" in family_upper or
        "STM32U5" in family_upper or "STM32L5" in family_upper
    )

    content = f"""#pragma once

#include <cstdint>

namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()}::hardware {{

// ============================================================================
// Hardware Register Addresses for {mcu_info.device_name}
// Auto-generated from SVD: {mcu_info.series}
// ============================================================================

"""

    # Generate GPIO register structure based on architecture
    if uses_modern_gpio:
        # STM32F4/F2/F7/L4/G4/H7/U5 use MODER/OSPEEDR/etc
        content += """// GPIO Registers structure (STM32F4/F2/F7/L4/G4/H7/U5 architecture)
struct GPIO_Registers {
    volatile uint32_t MODER;    // Mode register
    volatile uint32_t OTYPER;   // Output type register
    volatile uint32_t OSPEEDR;  // Output speed register
    volatile uint32_t PUPDR;    // Pull-up/pull-down register
    volatile uint32_t IDR;      // Input data register
    volatile uint32_t ODR;      // Output data register
    volatile uint32_t BSRR;     // Bit set/reset register
    volatile uint32_t LCKR;     // Port configuration lock register
    volatile uint32_t AFR[2];   // Alternate function registers (AFRL, AFRH)
};

// RCC Registers structure (STM32F4 architecture)
struct RCC_Registers {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    volatile uint32_t _reserved0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t _reserved1[2];
    volatile uint32_t AHB1ENR;     // AHB1 peripheral clock enable register
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    volatile uint32_t _reserved2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
};

"""
    else:
        # STM32F1/F0 use CRL/CRH
        content += """// GPIO Registers structure (STM32F1/F0 architecture)
struct GPIO_Registers {
    volatile uint32_t CRL;      // Port configuration register low (pins 0-7)
    volatile uint32_t CRH;      // Port configuration register high (pins 8-15)
    volatile uint32_t IDR;      // Input data register
    volatile uint32_t ODR;      // Output data register
    volatile uint32_t BSRR;     // Bit set/reset register
    volatile uint32_t BRR;      // Bit reset register
    volatile uint32_t LCKR;     // Port configuration lock register
};

// RCC Registers structure (STM32F1 architecture)
struct RCC_Registers {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;      // APB2 peripheral clock enable register
    volatile uint32_t APB1ENR;
};

"""

    content += "// Base addresses from SVD\n"

    # Add GPIO port addresses
    for port_letter in ['A', 'B', 'C', 'D', 'E', 'F', 'G']:
        gpio_name = f"GPIO{port_letter}"
        if gpio_name in addresses:
            content += f"constexpr uintptr_t GPIO{port_letter}_BASE = {addresses[gpio_name]};\n"

    content += "\n"

    # Add RCC address
    if 'RCC' in addresses:
        content += f"constexpr uintptr_t RCC_BASE = {addresses['RCC']};\n\n"

    # Add port instances
    content += "// GPIO port instances\n"
    for port_letter in ['A', 'B', 'C', 'D', 'E', 'F', 'G']:
        gpio_name = f"GPIO{port_letter}"
        if gpio_name in addresses:
            content += f"inline GPIO_Registers* const GPIO{port_letter} = reinterpret_cast<GPIO_Registers*>(GPIO{port_letter}_BASE);\n"

    content += "\n// RCC instance\n"
    content += "inline RCC_Registers* const RCC = reinterpret_cast<RCC_Registers*>(RCC_BASE);\n\n"

    # Add RCC clock enable bits
    content += """// RCC APB2ENR bits for GPIO clocks
constexpr uint32_t RCC_APB2ENR_IOPAEN = (1U << 2);
constexpr uint32_t RCC_APB2ENR_IOPBEN = (1U << 3);
constexpr uint32_t RCC_APB2ENR_IOPCEN = (1U << 4);
constexpr uint32_t RCC_APB2ENR_IOPDEN = (1U << 5);
constexpr uint32_t RCC_APB2ENR_IOPEEN = (1U << 6);
constexpr uint32_t RCC_APB2ENR_AFIOEN = (1U << 0);

"""

    # Add a Hardware struct that wraps the namespace for template use
    content += f"""

}}  // namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()}::hardware

// Hardware traits struct for template usage
namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()} {{

struct Hardware {{
    using GPIO_Registers = hardware::GPIO_Registers;
    using RCC_Registers = hardware::RCC_Registers;

    static inline GPIO_Registers* const GPIOA = hardware::GPIOA;
    static inline GPIO_Registers* const GPIOB = hardware::GPIOB;
    static inline GPIO_Registers* const GPIOC = hardware::GPIOC;
    static inline GPIO_Registers* const GPIOD = hardware::GPIOD;
    static inline GPIO_Registers* const GPIOE = hardware::GPIOE;
    static inline GPIO_Registers* const GPIOF = hardware::GPIOF;
    static inline GPIO_Registers* const GPIOG = hardware::GPIOG;
    static inline RCC_Registers* const RCC = hardware::RCC;

    static constexpr uint32_t RCC_APB2ENR_AFIOEN = hardware::RCC_APB2ENR_AFIOEN;
}};

}}  // namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()}
"""

    return content


def generate_traits_header(mcu_info: MCUInfo, package: MCUPackageInfo) -> str:
    """Generate traits.hpp content"""

    content = f"""#pragma once

#include <cstdint>

namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()} {{

// ============================================================================
// MCU Characteristics for {mcu_info.device_name}
// Auto-generated from SVD: {mcu_info.series}
// ============================================================================

struct Traits {{
    // Device identification
    static constexpr const char* DEVICE_NAME = "{mcu_info.device_name}";
    static constexpr const char* VENDOR = "{mcu_info.vendor if mcu_info.vendor != 'Unknown' else 'STMicroelectronics'}";
    static constexpr const char* FAMILY = "{mcu_info.family.upper()}";
    static constexpr const char* SERIES = "{mcu_info.series.upper()}";

    // Package information
    static constexpr const char* PACKAGE = "{package.package_name}";
    static constexpr uint8_t PIN_COUNT = {package.get_gpio_pin_count()};

    // Memory configuration
    static constexpr uint32_t FLASH_SIZE = {mcu_info.flash_kb} * 1024;      // {mcu_info.flash_kb} KB
    static constexpr uint32_t SRAM_SIZE = {mcu_info.sram_kb} * 1024;       // {mcu_info.sram_kb} KB
    static constexpr uint32_t FLASH_BASE = 0x08000000;
    static constexpr uint32_t SRAM_BASE = 0x20000000;

    // Clock configuration
    static constexpr uint32_t MAX_FREQ_HZ = 72'000'000;    // 72 MHz max
    static constexpr uint32_t HSI_FREQ_HZ = 8'000'000;     // 8 MHz HSI
    static constexpr uint32_t LSI_FREQ_HZ = 40'000;        // 40 kHz LSI

    // Peripheral availability
    struct Peripherals {{
        static constexpr uint8_t UART_COUNT = {mcu_info.uart_count};
        static constexpr uint8_t I2C_COUNT = {mcu_info.i2c_count};
        static constexpr uint8_t SPI_COUNT = {mcu_info.spi_count};
        static constexpr uint8_t ADC_COUNT = {mcu_info.adc_count};
        static constexpr uint8_t ADC_CHANNELS = {mcu_info.adc_channels};
        static constexpr uint8_t TIMER_COUNT = {mcu_info.timer_count};
        static constexpr uint8_t DMA_CHANNELS = {mcu_info.dma_channels};

        // Feature flags
        static constexpr bool HAS_USB = {"true" if mcu_info.has_usb else "false"};
        static constexpr bool HAS_CAN = {"true" if mcu_info.has_can else "false"};
        static constexpr bool HAS_DAC = {"true" if mcu_info.has_dac else "false"};
        static constexpr bool HAS_RTC = {"true" if mcu_info.has_rtc else "false"};
        static constexpr bool HAS_FSMC = false;
    }};
}};

}}  // namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()}
"""

    return content


def generate_for_variant(base_svd: Path, variant_name: str) -> bool:
    """Generate pin headers for a specific variant"""
    try:
        print(f"  Processing {variant_name}...")

        # Extract base info from SVD
        base_mcu = extract_mcu_info_from_svd(base_svd, verbose=False)

        # Extract hardware addresses from SVD
        hw_addresses = extract_hardware_addresses(base_svd)

        # Apply variant configuration
        mcu_info = apply_variant_config(base_mcu, variant_name)

        if not mcu_info.packages:
            print(f"    ⚠️ No package info for {variant_name}")
            return False

        package = mcu_info.packages[0]

        # Create output directory using centralized path management
        vendor_dir = 'st' if mcu_info.vendor == 'STMicroelectronics' else mcu_info.vendor.lower().replace(' ', '_')
        device_dir = ensure_dir(get_mcu_output_dir(vendor_dir, mcu_info.family, mcu_info.device_name.lower()))

        # Generate pins.hpp
        pins_content = generate_pins_header(mcu_info, package)
        pins_file = device_dir / "pins.hpp"
        pins_file.write_text(pins_content)

        # Generate traits.hpp
        traits_content = generate_traits_header(mcu_info, package)
        traits_file = device_dir / "traits.hpp"
        traits_file.write_text(traits_content)

        # Generate hardware.hpp
        hardware_content = generate_hardware_header(mcu_info, hw_addresses)
        hardware_file = device_dir / "hardware.hpp"
        hardware_file.write_text(hardware_content)

        # Generate gpio.hpp (single include file)
        gpio_content = f"""#pragma once

// ============================================================================
// Single-include GPIO header for {mcu_info.device_name}
// Include this file to get all GPIO functionality
//
// Usage:
//   #include "hal/st/stm32f103/generated/{mcu_info.device_name.lower()}/gpio.hpp"
//
//   using namespace gpio;
//   using LED = GPIOPin<pins::PC13>;
//   LED::configureOutput();
//   LED::set();
// ============================================================================

#include "pins.hpp"
#include "pin_functions.hpp"
#include "traits.hpp"
#include "hardware.hpp"
#include "../../gpio_hal.hpp"

namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()} {{

// Alias GPIO HAL template with this MCU's hardware
template<uint8_t Pin>
using GPIOPin = alloy::hal::{mcu_info.family}::GPIOPin<Hardware, Pin>;

}}  // namespace alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()}

// Convenience namespace alias
namespace gpio = alloy::hal::{mcu_info.family}::{mcu_info.device_name.lower()};
"""
        gpio_file = device_dir / "gpio.hpp"
        gpio_file.write_text(gpio_content)

        print(f"    ✓ Generated {package.get_gpio_pin_count()} pins for {package.package_name}")
        return True

    except Exception as e:
        print(f"    ✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    print("🔍 Generating pin headers from SVD files...\n")

    # Discover all available SVDs
    all_svds = discover_all_svds()

    # Find STM32F103 SVD
    svd_key = "STM32F103xx"
    if svd_key not in all_svds:
        print(f"✗ SVD not found: {svd_key}")
        print(f"Available ST SVDs: {[k for k in all_svds.keys() if k.startswith('STM32')][:10]}")
        return 1

    svd_file = all_svds[svd_key].file_path
    print(f"📄 Using base SVD: {svd_file.name}\n")

    # Generate for all known variants
    success_count = 0
    fail_count = 0

    for variant_name in sorted(VARIANT_PACKAGES.keys()):
        if generate_for_variant(svd_file, variant_name):
            success_count += 1
        else:
            fail_count += 1

    print(f"\n✅ Generated {success_count} MCU variants")
    if fail_count > 0:
        print(f"⚠️  Failed: {fail_count} variants")

    return 0 if fail_count == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
