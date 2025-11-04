"""
ESP32 Pin Function Database

ESP32 uses a GPIO Matrix that allows almost any peripheral to be routed to any GPIO pin.
Unlike other MCUs with fixed alternate functions, ESP32 has flexible routing.

This database defines the most common/recommended pin mappings, but users can
configure different mappings through the GPIO Matrix at runtime.

Key ESP32 GPIO characteristics:
- GPIO 0-39 (40 pins total)
- GPIO 34-39 are INPUT ONLY (no output capability)
- GPIO 6-11 are connected to SPI flash (usually not available)
- GPIO 1, 3 are UART0 TX/RX (used for programming)
- Most peripherals can be mapped to any valid GPIO via GPIO Matrix
"""

from dataclasses import dataclass
from typing import List, Dict

@dataclass
class PinFunction:
    """Represents a peripheral function for a pin"""
    function_name: str      # e.g., "UART0_TX", "SPI2_MOSI"
    peripheral_type: str    # e.g., "UART", "SPI", "I2C"
    is_default: bool = False  # True if this is the recommended/default mapping

# ESP32 Pin Function Database
# Note: ESP32 GPIO Matrix allows flexible routing, these are recommended mappings
ESP32_PIN_FUNCTIONS: Dict[str, List[PinFunction]] = {
    # GPIO 0 - Strapping pin, boot mode
    "GPIO0": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("CLK_OUT1", "CLOCK", False),
        PinFunction("EMAC_TX_CLK", "ETHERNET", False),
    ],
    
    # GPIO 1 - UART0 TX (default debug)
    "GPIO1": [
        PinFunction("UART0_TX", "UART", True),
        PinFunction("CLK_OUT3", "CLOCK", False),
        PinFunction("EMAC_RXD2", "ETHERNET", False),
    ],
    
    # GPIO 2 - Strapping pin, LED on many boards
    "GPIO2": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("HSPIWP", "SPI", False),
        PinFunction("HS2_DATA0", "SDIO", False),
        PinFunction("SD_DATA0", "SDIO", False),
    ],
    
    # GPIO 3 - UART0 RX (default debug)
    "GPIO3": [
        PinFunction("UART0_RX", "UART", True),
        PinFunction("CLK_OUT2", "CLOCK", False),
    ],
    
    # GPIO 4 - Touch sensor capable
    "GPIO4": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("HSPIHD", "SPI", False),
        PinFunction("HS2_DATA1", "SDIO", False),
        PinFunction("SD_DATA1", "SDIO", False),
        PinFunction("EMAC_TX_ER", "ETHERNET", False),
        PinFunction("TOUCH0", "TOUCH", False),
    ],
    
    # GPIO 5 - Strapping pin, VSPI SS
    "GPIO5": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("VSPICS0", "SPI", True),  # Default VSPI CS
        PinFunction("HS1_DATA6", "SDIO", False),
        PinFunction("EMAC_RX_CLK", "ETHERNET", False),
    ],
    
    # GPIO 12 - Strapping pin, Touch sensor, HSPI MISO
    "GPIO12": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("HSPIQ", "SPI", True),  # HSPI MISO
        PinFunction("HS2_DATA2", "SDIO", False),
        PinFunction("SD_DATA2", "SDIO", False),
        PinFunction("TOUCH5", "TOUCH", False),
    ],
    
    # GPIO 13 - Touch sensor, HSPI MOSI
    "GPIO13": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("HSPID", "SPI", True),  # HSPI MOSI
        PinFunction("HS2_DATA3", "SDIO", False),
        PinFunction("SD_DATA3", "SDIO", False),
        PinFunction("TOUCH4", "TOUCH", False),
    ],
    
    # GPIO 14 - Touch sensor, HSPI CLK
    "GPIO14": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("HSPICLK", "SPI", True),  # HSPI CLK
        PinFunction("HS2_CLK", "SDIO", False),
        PinFunction("SD_CLK", "SDIO", False),
        PinFunction("TOUCH6", "TOUCH", False),
    ],
    
    # GPIO 15 - Strapping pin, Touch sensor, HSPI CS
    "GPIO15": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("HSPICS0", "SPI", True),  # HSPI CS
        PinFunction("HS2_CMD", "SDIO", False),
        PinFunction("SD_CMD", "SDIO", False),
        PinFunction("TOUCH3", "TOUCH", False),
    ],
    
    # GPIO 16 - UART2 RX
    "GPIO16": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("UART2_RX", "UART", True),
        PinFunction("HS1_DATA4", "SDIO", False),
        PinFunction("EMAC_CLK_OUT", "ETHERNET", False),
    ],
    
    # GPIO 17 - UART2 TX
    "GPIO17": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("UART2_TX", "UART", True),
        PinFunction("HS1_DATA5", "SDIO", False),
        PinFunction("EMAC_CLK_180", "ETHERNET", False),
    ],
    
    # GPIO 18 - VSPI CLK
    "GPIO18": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("VSPICLK", "SPI", True),  # Default VSPI CLK
        PinFunction("HS1_DATA7", "SDIO", False),
    ],
    
    # GPIO 19 - VSPI MISO
    "GPIO19": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("VSPIQ", "SPI", True),  # Default VSPI MISO
        PinFunction("EMAC_TXD0", "ETHERNET", False),
    ],
    
    # GPIO 21 - I2C SDA (default)
    "GPIO21": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("I2C0_SDA", "I2C", True),  # Default I2C SDA
        PinFunction("VSPIHD", "SPI", False),
        PinFunction("EMAC_TX_EN", "ETHERNET", False),
    ],
    
    # GPIO 22 - I2C SCL (default)
    "GPIO22": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("I2C0_SCL", "I2C", True),  # Default I2C SCL
        PinFunction("VSPIWP", "SPI", False),
        PinFunction("EMAC_TXD1", "ETHERNET", False),
    ],
    
    # GPIO 23 - VSPI MOSI
    "GPIO23": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("VSPID", "SPI", True),  # Default VSPI MOSI
        PinFunction("HS1_STROBE", "SDIO", False),
    ],
    
    # GPIO 25 - DAC1, ADC2_CH8
    "GPIO25": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("DAC1", "DAC", True),
        PinFunction("ADC2_CH8", "ADC", False),
        PinFunction("EMAC_RXD0", "ETHERNET", False),
    ],
    
    # GPIO 26 - DAC2, ADC2_CH9
    "GPIO26": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("DAC2", "DAC", True),
        PinFunction("ADC2_CH9", "ADC", False),
        PinFunction("EMAC_RXD1", "ETHERNET", False),
    ],
    
    # GPIO 27 - Touch sensor, ADC2_CH7
    "GPIO27": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("TOUCH7", "TOUCH", True),
        PinFunction("ADC2_CH7", "ADC", False),
        PinFunction("EMAC_RX_DV", "ETHERNET", False),
    ],
    
    # GPIO 32 - Touch sensor, ADC1_CH4
    "GPIO32": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("TOUCH9", "TOUCH", True),
        PinFunction("ADC1_CH4", "ADC", True),
    ],
    
    # GPIO 33 - Touch sensor, ADC1_CH5
    "GPIO33": [
        PinFunction("GPIO", "GPIO", True),
        PinFunction("TOUCH8", "TOUCH", True),
        PinFunction("ADC1_CH5", "ADC", True),
    ],
    
    # GPIO 34 - INPUT ONLY, ADC1_CH6
    "GPIO34": [
        PinFunction("GPIO_INPUT", "GPIO", True),
        PinFunction("ADC1_CH6", "ADC", True),
    ],
    
    # GPIO 35 - INPUT ONLY, ADC1_CH7
    "GPIO35": [
        PinFunction("GPIO_INPUT", "GPIO", True),
        PinFunction("ADC1_CH7", "ADC", True),
    ],
    
    # GPIO 36 (VP) - INPUT ONLY, ADC1_CH0
    "GPIO36": [
        PinFunction("GPIO_INPUT", "GPIO", True),
        PinFunction("ADC1_CH0", "ADC", True),
    ],
    
    # GPIO 39 (VN) - INPUT ONLY, ADC1_CH3
    "GPIO39": [
        PinFunction("GPIO_INPUT", "GPIO", True),
        PinFunction("ADC1_CH3", "ADC", True),
    ],
}

# GPIO capabilities flags
GPIO_CAPABILITIES = {
    "GPIO0": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO1": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO2": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO3": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO4": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO5": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO12": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO13": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO14": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO15": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO16": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO17": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO18": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO19": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO21": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO22": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO23": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO25": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO26": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO27": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO32": {"output": True, "input": True, "pullup": True, "pulldown": True},
    "GPIO33": {"output": True, "input": True, "pullup": True, "pulldown": True},
    # Input-only pins (34-39)
    "GPIO34": {"output": False, "input": True, "pullup": False, "pulldown": False},
    "GPIO35": {"output": False, "input": True, "pullup": False, "pulldown": False},
    "GPIO36": {"output": False, "input": True, "pullup": False, "pulldown": False},
    "GPIO39": {"output": False, "input": True, "pullup": False, "pulldown": False},
}
