#!/usr/bin/env python3
"""
Pin Function Database for Raspberry Pi RP2040

RP2040 Architecture:
- Dual Cortex-M0+ @ 133MHz
- 30 GPIO pins (GPIO0-GPIO29)
- Each pin can be assigned to different functions via FUNCSEL
- 9 functions per pin (F1-F9): SPI, UART, I2C, PWM, SIO, PIO0, PIO1, CLOCK, USB
"""

from dataclasses import dataclass
from typing import Dict, List


@dataclass
class PinFunction:
    """Pin alternate function definition for RP2040"""
    function_name: str      # e.g., "SPI0_RX", "UART0_TX", "PWM0_A"
    peripheral_type: str    # e.g., "SPI", "UART", "I2C", "PWM", "SIO", "PIO", "CLOCK", "USB"
    function_num: int       # Function number (1-9)


# RP2040 Pin Function Database
# Based on RP2040 datasheet GPIO function table
RP2040_PIN_FUNCTIONS: Dict[str, List[PinFunction]] = {
    "GPIO0": [
        PinFunction("SPI0_RX", "SPI", 1),
        PinFunction("UART0_TX", "UART", 2),
        PinFunction("I2C0_SDA", "I2C", 3),
        PinFunction("PWM0_A", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_OVCUR_DET", "USB", 9),
    ],
    "GPIO1": [
        PinFunction("SPI0_CSn", "SPI", 1),
        PinFunction("UART0_RX", "UART", 2),
        PinFunction("I2C0_SCL", "I2C", 3),
        PinFunction("PWM0_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_DET", "USB", 9),
    ],
    "GPIO2": [
        PinFunction("SPI0_SCK", "SPI", 1),
        PinFunction("UART0_CTS", "UART", 2),
        PinFunction("I2C1_SDA", "I2C", 3),
        PinFunction("PWM1_A", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_EN", "USB", 9),
    ],
    "GPIO3": [
        PinFunction("SPI0_TX", "SPI", 1),
        PinFunction("UART0_RTS", "UART", 2),
        PinFunction("I2C1_SCL", "I2C", 3),
        PinFunction("PWM1_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_OVCUR_DET", "USB", 9),
    ],
    "GPIO4": [
        PinFunction("SPI0_RX", "SPI", 1),
        PinFunction("UART1_TX", "UART", 2),
        PinFunction("I2C0_SDA", "I2C", 3),
        PinFunction("PWM2_A", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_DET", "USB", 9),
    ],
    "GPIO5": [
        PinFunction("SPI0_CSn", "SPI", 1),
        PinFunction("UART1_RX", "UART", 2),
        PinFunction("I2C0_SCL", "I2C", 3),
        PinFunction("PWM2_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_EN", "USB", 9),
    ],
    "GPIO6": [
        PinFunction("SPI0_SCK", "SPI", 1),
        PinFunction("UART1_CTS", "UART", 2),
        PinFunction("I2C1_SDA", "I2C", 3),
        PinFunction("PWM3_A", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_OVCUR_DET", "USB", 9),
    ],
    "GPIO7": [
        PinFunction("SPI0_TX", "SPI", 1),
        PinFunction("UART1_RTS", "UART", 2),
        PinFunction("I2C1_SCL", "I2C", 3),
        PinFunction("PWM3_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_DET", "USB", 9),
    ],
    "GPIO8": [
        PinFunction("SPI1_RX", "SPI", 1),
        PinFunction("UART1_TX", "UART", 2),
        PinFunction("I2C0_SDA", "I2C", 3),
        PinFunction("PWM4_A", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_EN", "USB", 9),
    ],
    "GPIO9": [
        PinFunction("SPI1_CSn", "SPI", 1),
        PinFunction("UART1_RX", "UART", 2),
        PinFunction("I2C0_SCL", "I2C", 3),
        PinFunction("PWM4_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_OVCUR_DET", "USB", 9),
    ],
    "GPIO10": [
        PinFunction("SPI1_SCK", "SPI", 1),
        PinFunction("UART1_CTS", "UART", 2),
        PinFunction("I2C1_SDA", "I2C", 3),
        PinFunction("PWM5_A", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_DET", "USB", 9),
    ],
    "GPIO11": [
        PinFunction("SPI1_TX", "SPI", 1),
        PinFunction("UART1_RTS", "UART", 2),
        PinFunction("I2C1_SCL", "I2C", 3),
        PinFunction("PWM5_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_EN", "USB", 9),
    ],
    "GPIO12": [
        PinFunction("SPI1_RX", "SPI", 1),
        PinFunction("UART0_TX", "UART", 2),
        PinFunction("I2C0_SDA", "I2C", 3),
        PinFunction("PWM6_A", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_OVCUR_DET", "USB", 9),
    ],
    "GPIO13": [
        PinFunction("SPI1_CSn", "SPI", 1),
        PinFunction("UART0_RX", "UART", 2),
        PinFunction("I2C0_SCL", "I2C", 3),
        PinFunction("PWM6_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_DET", "USB", 9),
    ],
    "GPIO14": [
        PinFunction("SPI1_SCK", "SPI", 1),
        PinFunction("UART0_CTS", "UART", 2),
        PinFunction("I2C1_SDA", "I2C", 3),
        PinFunction("PWM7_A", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_VBUS_EN", "USB", 9),
    ],
    "GPIO15": [
        PinFunction("SPI1_TX", "SPI", 1),
        PinFunction("UART0_RTS", "UART", 2),
        PinFunction("I2C1_SCL", "I2C", 3),
        PinFunction("PWM7_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
        PinFunction("USB_OVCUR_DET", "USB", 9),
    ],
    # GPIO16-29 follow similar pattern
    "GPIO25": [  # Special: On-board LED on Pico
        PinFunction("SPI1_CSn", "SPI", 1),
        PinFunction("UART1_RX", "UART", 2),
        PinFunction("I2C0_SCL", "I2C", 3),
        PinFunction("PWM4_B", "PWM", 4),
        PinFunction("SIO", "SIO", 5),
        PinFunction("PIO0", "PIO", 6),
        PinFunction("PIO1", "PIO", 7),
    ],
}


def get_pin_functions(pin_name: str) -> List[PinFunction]:
    """Get all alternate functions for a pin"""
    return RP2040_PIN_FUNCTIONS.get(pin_name, [])


def get_all_pins() -> List[str]:
    """Get list of all available pins"""
    return [f"GPIO{i}" for i in range(30)]


def get_peripheral_pins(peripheral_type: str) -> Dict[str, List[PinFunction]]:
    """Get all pins that support a specific peripheral type"""
    result = {}
    for pin_name, functions in RP2040_PIN_FUNCTIONS.items():
        matching_funcs = [f for f in functions if f.peripheral_type == peripheral_type]
        if matching_funcs:
            result[pin_name] = matching_funcs
    return result


if __name__ == "__main__":
    print(f"Total pins: 30 (GPIO0-GPIO29)")
    print(f"Defined in database: {len(RP2040_PIN_FUNCTIONS)}")
    print(f"\nGPIO25 (Pico LED) functions:")
    for func in get_pin_functions("GPIO25"):
        print(f"  F{func.function_num}: {func.function_name} ({func.peripheral_type})")
