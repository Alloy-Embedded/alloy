#!/usr/bin/env python3
"""
Generate STM32 pin definitions - Minimal implementation

This is a straightforward implementation that generates pin headers for STM32 MCUs
based on known pin configurations. Future versions can extract this from SVD files.
"""

from pathlib import Path
from typing import Dict, List

REPO_ROOT = Path(__file__).parent.parent.parent
OUTPUT_DIR = REPO_ROOT / "src" / "hal" / "st"

# Known STM32F1 pin configurations
# Format: (device_name, package, flash_kb, ram_kb, ports_with_pin_counts)
STM32F1_DEVICES = [
    {
        "name": "stm32f103c8",
        "display_name": "STM32F103C8",
        "package": "LQFP48",
        "flash_kb": 64,
        "ram_kb": 20,
        "family": "stm32f1",
        "ports": {
            "A": list(range(16)),  # PA0-PA15
            "B": list(range(16)),  # PB0-PB15
            "C": [13, 14, 15],     # PC13-PC15 (limited on LQFP48)
            "D": [0, 1],           # PD0-PD1 (OSC pins)
        },
        "peripherals": {
            "uart_count": 3,
            "i2c_count": 2,
            "spi_count": 2,
            "adc_count": 2,
            "adc_channels": 10,
            "timer_count": 7,
            "dma_channels": 7,
            "has_usb": True,
            "has_can": True,
            "has_dac": False,
        }
    },
    {
        "name": "stm32f103cb",
        "display_name": "STM32F103CB",
        "package": "LQFP48",
        "flash_kb": 128,
        "ram_kb": 20,
        "family": "stm32f1",
        "ports": {
            "A": list(range(16)),
            "B": list(range(16)),
            "C": [13, 14, 15],
            "D": [0, 1],
        },
        "peripherals": {
            "uart_count": 3,
            "i2c_count": 2,
            "spi_count": 2,
            "adc_count": 2,
            "adc_channels": 10,
            "timer_count": 7,
            "dma_channels": 7,
            "has_usb": True,
            "has_can": True,
            "has_dac": False,
        }
    },
]


def generate_pins_header(device: Dict) -> str:
    """Generate pins.hpp content for a device"""

    # Calculate pin numbers
    pin_definitions = []
    port_offset = 0

    # Map port letters to base values: A=0, B=16, C=32, D=48
    port_bases = {'A': 0, 'B': 16, 'C': 32, 'D': 48, 'E': 64, 'F': 80, 'G': 96}

    for port_name in sorted(device["ports"].keys()):
        pins = device["ports"][port_name]
        port_base = port_bases.get(port_name, 0)

        for pin_num in pins:
            pin_name = f"P{port_name}{pin_num}"
            pin_value = port_base + pin_num
            pin_definitions.append((pin_name, pin_value, port_name, pin_num))

    # Sort by pin value
    pin_definitions.sort(key=lambda x: x[1])

    # Generate header content
    content = f"""#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace alloy::hal::{device["family"]}::{device["name"]}::pins {{

// ============================================================================
// Auto-generated pin definitions for {device["display_name"]}
// Package: {device["package"]}
// Pin Count: {len(pin_definitions)}
// ============================================================================

"""

    # Generate pin constants grouped by port
    current_port = None
    for pin_name, pin_value, port, pin_num in pin_definitions:
        if port != current_port:
            if current_port is not None:
                content += "\n"
            content += f"// Port {port} pins\n"
            current_port = port

        content += f"constexpr uint8_t {pin_name:<6} = {pin_value};\n"

    # Generate validation infrastructure
    valid_pins = [str(p[1]) for p in pin_definitions]
    content += f"""
// ============================================================================
// Compile-time pin validation
// ============================================================================

namespace detail {{
    // Lookup table for valid pins
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
        "Invalid pin for {device["display_name"]} ({device["package"]})! "
        "Check the datasheet for available pins on this package.");
}}

// Constants
constexpr size_t TOTAL_PIN_COUNT = detail::PIN_COUNT;  // {len(pin_definitions)} pins
constexpr size_t GPIO_PORT_COUNT = {len(device["ports"])};  // {len(device["ports"])} ports

}}  // namespace alloy::hal::{device["family"]}::{device["name"]}::pins
"""

    return content


def generate_traits_header(device: Dict) -> str:
    """Generate traits.hpp content for a device"""

    perip = device["peripherals"]

    content = f"""#pragma once

#include <cstdint>

namespace alloy::hal::{device["family"]}::{device["name"]} {{

// ============================================================================
// MCU Characteristics for {device["display_name"]}
// Auto-generated
// ============================================================================

struct Traits {{
    // Device identification
    static constexpr const char* DEVICE_NAME = "{device["display_name"]}";
    static constexpr const char* VENDOR = "STMicroelectronics";
    static constexpr const char* FAMILY = "{device["family"].upper()}";
    static constexpr const char* SERIES = "{device["display_name"][:9]}";  // STM32F103

    // Package information
    static constexpr const char* PACKAGE = "{device["package"]}";
    static constexpr uint8_t PIN_COUNT = {sum(len(pins) for pins in device["ports"].values())};

    // Memory configuration
    static constexpr uint32_t FLASH_SIZE = {device["flash_kb"]} * 1024;      // {device["flash_kb"]} KB
    static constexpr uint32_t SRAM_SIZE = {device["ram_kb"]} * 1024;       // {device["ram_kb"]} KB
    static constexpr uint32_t FLASH_BASE = 0x08000000;
    static constexpr uint32_t SRAM_BASE = 0x20000000;

    // Clock configuration
    static constexpr uint32_t MAX_FREQ_HZ = 72'000'000;    // 72 MHz max
    static constexpr uint32_t HSI_FREQ_HZ = 8'000'000;     // 8 MHz HSI
    static constexpr uint32_t LSI_FREQ_HZ = 40'000;        // 40 kHz LSI

    // Peripheral availability
    struct Peripherals {{
        static constexpr uint8_t UART_COUNT = {perip["uart_count"]};
        static constexpr uint8_t I2C_COUNT = {perip["i2c_count"]};
        static constexpr uint8_t SPI_COUNT = {perip["spi_count"]};
        static constexpr uint8_t ADC_COUNT = {perip["adc_count"]};
        static constexpr uint8_t ADC_CHANNELS = {perip["adc_channels"]};
        static constexpr uint8_t TIMER_COUNT = {perip["timer_count"]};
        static constexpr uint8_t DMA_CHANNELS = {perip["dma_channels"]};

        // Feature flags
        static constexpr bool HAS_USB = {"true" if perip["has_usb"] else "false"};
        static constexpr bool HAS_CAN = {"true" if perip["has_can"] else "false"};
        static constexpr bool HAS_DAC = {"true" if perip["has_dac"] else "false"};
        static constexpr bool HAS_RTC = true;
        static constexpr bool HAS_FSMC = false;
    }};
}};

}}  // namespace alloy::hal::{device["family"]}::{device["name"]}
"""

    return content


def generate_all_stm32f1():
    """Generate pin headers for all STM32F1 devices"""
    print("Generating STM32F1 pin definitions...")

    for device in STM32F1_DEVICES:
        # Create output directory
        device_dir = OUTPUT_DIR / device["family"] / "generated" / device["name"]
        device_dir.mkdir(parents=True, exist_ok=True)

        # Generate pins.hpp
        pins_content = generate_pins_header(device)
        pins_file = device_dir / "pins.hpp"
        pins_file.write_text(pins_content)
        print(f"  ✓ Generated: {pins_file.relative_to(REPO_ROOT)}")

        # Generate traits.hpp
        traits_content = generate_traits_header(device)
        traits_file = device_dir / "traits.hpp"
        traits_file.write_text(traits_content)
        print(f"  ✓ Generated: {traits_file.relative_to(REPO_ROOT)}")

    print(f"\n✅ Generated {len(STM32F1_DEVICES)} MCU pin definitions")


def main():
    generate_all_stm32f1()


if __name__ == "__main__":
    main()
