#!/usr/bin/env python3
"""
STM32F1 Pin Function Database

Maps each pin to its available alternate functions based on STM32F103 datasheet.
This is used to generate pin function headers for compile-time validation.
"""

from typing import Dict, List, Set
from dataclasses import dataclass

@dataclass
class PinFunction:
    """Represents an alternate function for a pin"""
    name: str           # Function name (e.g., "USART1_TX")
    peripheral: str     # Peripheral type (e.g., "USART")
    af_number: int      # Alternate function number (for STM32F1, this is remap info)
    remap_level: int    # 0 = default, 1 = partial remap, 2 = full remap

# STM32F103 Pin Function Database
# Based on STM32F103 datasheet Table 5 (Pin definitions)
# Format: pin_name -> list of alternate functions
STM32F103_PIN_FUNCTIONS = {
    # Port A
    "PA0": [
        PinFunction("WKUP", "SYSTEM", 0, 0),           # Wakeup pin
        PinFunction("USART2_CTS", "USART", 2, 0),
        PinFunction("ADC12_IN0", "ADC", 0, 0),
        PinFunction("TIM2_CH1_ETR", "TIM", 2, 0),
    ],
    "PA1": [
        PinFunction("USART2_RTS", "USART", 2, 0),
        PinFunction("ADC12_IN1", "ADC", 0, 0),
        PinFunction("TIM2_CH2", "TIM", 2, 0),
    ],
    "PA2": [
        PinFunction("USART2_TX", "USART", 2, 0),
        PinFunction("ADC12_IN2", "ADC", 0, 0),
        PinFunction("TIM2_CH3", "TIM", 2, 0),
    ],
    "PA3": [
        PinFunction("USART2_RX", "USART", 2, 0),
        PinFunction("ADC12_IN3", "ADC", 0, 0),
        PinFunction("TIM2_CH4", "TIM", 2, 0),
    ],
    "PA4": [
        PinFunction("SPI1_NSS", "SPI", 1, 0),
        PinFunction("USART2_CK", "USART", 2, 0),
        PinFunction("ADC12_IN4", "ADC", 0, 0),
    ],
    "PA5": [
        PinFunction("SPI1_SCK", "SPI", 1, 0),
        PinFunction("ADC12_IN5", "ADC", 0, 0),
    ],
    "PA6": [
        PinFunction("SPI1_MISO", "SPI", 1, 0),
        PinFunction("ADC12_IN6", "ADC", 0, 0),
        PinFunction("TIM3_CH1", "TIM", 3, 0),
        PinFunction("TIM1_BKIN", "TIM", 1, 0),
    ],
    "PA7": [
        PinFunction("SPI1_MOSI", "SPI", 1, 0),
        PinFunction("ADC12_IN7", "ADC", 0, 0),
        PinFunction("TIM3_CH2", "TIM", 3, 0),
        PinFunction("TIM1_CH1N", "TIM", 1, 0),
    ],
    "PA8": [
        PinFunction("USART1_CK", "USART", 1, 0),
        PinFunction("TIM1_CH1", "TIM", 1, 0),
        PinFunction("MCO", "SYSTEM", 0, 0),            # Master Clock Output
    ],
    "PA9": [
        PinFunction("USART1_TX", "USART", 1, 0),
        PinFunction("TIM1_CH2", "TIM", 1, 0),
    ],
    "PA10": [
        PinFunction("USART1_RX", "USART", 1, 0),
        PinFunction("TIM1_CH3", "TIM", 1, 0),
    ],
    "PA11": [
        PinFunction("USART1_CTS", "USART", 1, 0),
        PinFunction("CAN_RX", "CAN", 0, 0),
        PinFunction("TIM1_CH4", "TIM", 1, 0),
        PinFunction("USB_DM", "USB", 0, 0),
    ],
    "PA12": [
        PinFunction("USART1_RTS", "USART", 1, 0),
        PinFunction("CAN_TX", "CAN", 0, 0),
        PinFunction("TIM1_ETR", "TIM", 1, 0),
        PinFunction("USB_DP", "USB", 0, 0),
    ],
    "PA13": [
        PinFunction("JTMS-SWDIO", "DEBUG", 0, 0),      # JTAG/SWD
    ],
    "PA14": [
        PinFunction("JTCK-SWCLK", "DEBUG", 0, 0),      # JTAG/SWD
    ],
    "PA15": [
        PinFunction("JTDI", "DEBUG", 0, 0),            # JTAG
        PinFunction("TIM2_CH1_ETR", "TIM", 2, 1),      # Partial remap
        PinFunction("SPI1_NSS", "SPI", 1, 1),          # Remap
    ],

    # Port B
    "PB0": [
        PinFunction("ADC12_IN8", "ADC", 0, 0),
        PinFunction("TIM3_CH3", "TIM", 3, 0),
        PinFunction("TIM1_CH2N", "TIM", 1, 0),
    ],
    "PB1": [
        PinFunction("ADC12_IN9", "ADC", 0, 0),
        PinFunction("TIM3_CH4", "TIM", 3, 0),
        PinFunction("TIM1_CH3N", "TIM", 1, 0),
    ],
    "PB2": [
        PinFunction("BOOT1", "SYSTEM", 0, 0),
    ],
    "PB3": [
        PinFunction("JTDO", "DEBUG", 0, 0),            # JTAG
        PinFunction("TRACESWO", "DEBUG", 0, 0),
        PinFunction("TIM2_CH2", "TIM", 2, 1),          # Partial remap
        PinFunction("SPI1_SCK", "SPI", 1, 1),          # Remap
    ],
    "PB4": [
        PinFunction("NJTRST", "DEBUG", 0, 0),          # JTAG
        PinFunction("TIM3_CH1", "TIM", 3, 1),          # Partial remap
        PinFunction("SPI1_MISO", "SPI", 1, 1),         # Remap
    ],
    "PB5": [
        PinFunction("I2C1_SMBA", "I2C", 1, 0),
        PinFunction("TIM3_CH2", "TIM", 3, 1),          # Partial remap
        PinFunction("SPI1_MOSI", "SPI", 1, 1),         # Remap
    ],
    "PB6": [
        PinFunction("I2C1_SCL", "I2C", 1, 0),
        PinFunction("TIM4_CH1", "TIM", 4, 0),
        PinFunction("USART1_TX", "USART", 1, 1),       # Remap
    ],
    "PB7": [
        PinFunction("I2C1_SDA", "I2C", 1, 0),
        PinFunction("TIM4_CH2", "TIM", 4, 0),
        PinFunction("USART1_RX", "USART", 1, 1),       # Remap
    ],
    "PB8": [
        PinFunction("TIM4_CH3", "TIM", 4, 0),
        PinFunction("I2C1_SCL", "I2C", 1, 1),          # Remap
        PinFunction("CAN_RX", "CAN", 0, 1),            # Remap
    ],
    "PB9": [
        PinFunction("TIM4_CH4", "TIM", 4, 0),
        PinFunction("I2C1_SDA", "I2C", 1, 1),          # Remap
        PinFunction("CAN_TX", "CAN", 0, 1),            # Remap
    ],
    "PB10": [
        PinFunction("I2C2_SCL", "I2C", 2, 0),
        PinFunction("USART3_TX", "USART", 3, 0),
        PinFunction("TIM2_CH3", "TIM", 2, 1),          # Partial remap
    ],
    "PB11": [
        PinFunction("I2C2_SDA", "I2C", 2, 0),
        PinFunction("USART3_RX", "USART", 3, 0),
        PinFunction("TIM2_CH4", "TIM", 2, 1),          # Partial remap
    ],
    "PB12": [
        PinFunction("SPI2_NSS", "SPI", 2, 0),
        PinFunction("I2C2_SMBA", "I2C", 2, 0),
        PinFunction("USART3_CK", "USART", 3, 0),
        PinFunction("TIM1_BKIN", "TIM", 1, 0),
    ],
    "PB13": [
        PinFunction("SPI2_SCK", "SPI", 2, 0),
        PinFunction("USART3_CTS", "USART", 3, 0),
        PinFunction("TIM1_CH1N", "TIM", 1, 0),
    ],
    "PB14": [
        PinFunction("SPI2_MISO", "SPI", 2, 0),
        PinFunction("USART3_RTS", "USART", 3, 0),
        PinFunction("TIM1_CH2N", "TIM", 1, 0),
    ],
    "PB15": [
        PinFunction("SPI2_MOSI", "SPI", 2, 0),
        PinFunction("TIM1_CH3N", "TIM", 1, 0),
    ],

    # Port C (limited pins in LQFP48)
    "PC0": [
        PinFunction("ADC12_IN10", "ADC", 0, 0),
    ],
    "PC1": [
        PinFunction("ADC12_IN11", "ADC", 0, 0),
    ],
    "PC2": [
        PinFunction("ADC12_IN12", "ADC", 0, 0),
    ],
    "PC3": [
        PinFunction("ADC12_IN13", "ADC", 0, 0),
    ],
    "PC4": [
        PinFunction("ADC12_IN14", "ADC", 0, 0),
    ],
    "PC5": [
        PinFunction("ADC12_IN15", "ADC", 0, 0),
    ],
    "PC6": [
        PinFunction("TIM3_CH1", "TIM", 3, 2),          # Full remap
    ],
    "PC7": [
        PinFunction("TIM3_CH2", "TIM", 3, 2),          # Full remap
    ],
    "PC8": [
        PinFunction("TIM3_CH3", "TIM", 3, 2),          # Full remap
    ],
    "PC9": [
        PinFunction("TIM3_CH4", "TIM", 3, 2),          # Full remap
    ],
    "PC10": [
        PinFunction("USART3_TX", "USART", 3, 1),       # Partial remap
    ],
    "PC11": [
        PinFunction("USART3_RX", "USART", 3, 1),       # Partial remap
    ],
    "PC12": [
        PinFunction("USART3_CK", "USART", 3, 1),       # Partial remap
    ],
    "PC13": [
        PinFunction("TAMPER-RTC", "RTC", 0, 0),
    ],
    "PC14": [
        PinFunction("OSC32_IN", "RTC", 0, 0),
    ],
    "PC15": [
        PinFunction("OSC32_OUT", "RTC", 0, 0),
    ],

    # Port D (limited availability)
    "PD0": [
        PinFunction("OSC_IN", "SYSTEM", 0, 0),
        PinFunction("CAN_RX", "CAN", 0, 2),            # Remap 2
    ],
    "PD1": [
        PinFunction("OSC_OUT", "SYSTEM", 0, 0),
        PinFunction("CAN_TX", "CAN", 0, 2),            # Remap 2
    ],
    "PD2": [
        PinFunction("TIM3_ETR", "TIM", 3, 0),
    ],
}


def get_pin_functions(pin_name: str) -> List[PinFunction]:
    """Get all functions available for a pin"""
    return STM32F103_PIN_FUNCTIONS.get(pin_name, [])


def get_peripheral_pins(peripheral_type: str) -> Dict[str, List[str]]:
    """Get all pins that support a peripheral type (e.g., 'ADC', 'USART')"""
    result = {}
    for pin_name, functions in STM32F103_PIN_FUNCTIONS.items():
        for func in functions:
            if func.peripheral == peripheral_type:
                if func.name not in result:
                    result[func.name] = []
                result[func.name].append(pin_name)
    return result


def get_available_peripherals() -> Set[str]:
    """Get list of all peripheral types in the database"""
    peripherals = set()
    for functions in STM32F103_PIN_FUNCTIONS.values():
        for func in functions:
            peripherals.add(func.peripheral)
    return peripherals


if __name__ == "__main__":
    # Test the database
    print("STM32F103 Pin Function Database")
    print("=" * 60)

    # Show PA9 functions (typical USART1_TX)
    print("\nPA9 Functions:")
    for func in get_pin_functions("PA9"):
        print(f"  - {func.name} ({func.peripheral})")

    # Show all ADC pins
    print("\nADC Pins:")
    adc_pins = get_peripheral_pins("ADC")
    for adc_name, pins in sorted(adc_pins.items()):
        print(f"  {adc_name}: {', '.join(pins)}")

    # Show available peripherals
    print("\nAvailable Peripherals:")
    for peripheral in sorted(get_available_peripherals()):
        print(f"  - {peripheral}")
