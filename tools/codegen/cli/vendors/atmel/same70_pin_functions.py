#!/usr/bin/env python3
"""
SAME70 Pin Function Database

Pin function mappings for Atmel/Microchip SAME70 family (Cortex-M7).
Based on SAME70 datasheet - Peripheral Signal Multiplexing.

Architecture: SAME70 uses PIO (Parallel I/O) controller with peripheral multiplexing
- Each pin can be assigned to PIO or up to 4 peripherals (A, B, C, D)
- Controlled via PIO_ABCDSR1 and PIO_ABCDSR2 registers
"""

from dataclasses import dataclass
from typing import List, Dict


@dataclass
class PinFunction:
    """Pin alternate function definition for SAME70"""
    function_name: str      # e.g., "UART0_RXD", "SPI0_MISO"
    peripheral_type: str    # e.g., "UART", "SPI", "TWI", "PWM"
    peripheral: str         # Peripheral select: "A", "B", "C", or "D"

    def __repr__(self):
        return f"PinFunction({self.function_name}, {self.peripheral_type}, Periph{self.peripheral})"


# SAME70 Pin Function Database
# Format: Pin name -> List of alternate functions
# Based on SAME70Q21B datasheet Table 7-1: I/O Multiplexing
SAME70_PIN_FUNCTIONS: Dict[str, List[PinFunction]] = {
    # Port A
    "PA0": [
        PinFunction("PWMC0_PWMH0", "PWM", "A"),
        PinFunction("TC0_TIOA0", "TC", "B"),
        PinFunction("PIOA_0", "PIO", "PIO"),
    ],
    "PA1": [
        PinFunction("PWMC0_PWMH1", "PWM", "A"),
        PinFunction("TC0_TIOB0", "TC", "B"),
        PinFunction("PIOA_1", "PIO", "PIO"),
    ],
    "PA2": [
        PinFunction("PWMC0_PWMH2", "PWM", "A"),
        PinFunction("DACC_DATRG", "DAC", "C"),
        PinFunction("PIOA_2", "PIO", "PIO"),
    ],
    "PA3": [
        PinFunction("TWD0", "TWI", "A"),  # I2C Data
        PinFunction("LCDDAT3", "LCD", "C"),
        PinFunction("PIOA_3", "PIO", "PIO"),
    ],
    "PA4": [
        PinFunction("TWCK0", "TWI", "A"),  # I2C Clock
        PinFunction("TCLK0", "TC", "B"),
        PinFunction("PIOA_4", "PIO", "PIO"),
    ],
    "PA5": [
        PinFunction("RXDA", "UART", "C"),  # UART0 RX
        PinFunction("URXD0", "USART", "A"),  # USART0 RX
        PinFunction("PIOA_5", "PIO", "PIO"),
    ],
    "PA6": [
        PinFunction("TXDA", "UART", "C"),  # UART0 TX
        PinFunction("UTXD0", "USART", "A"),  # USART0 TX
        PinFunction("PIOA_6", "PIO", "PIO"),
    ],
    "PA7": [
        PinFunction("PWMC0_PWMH3", "PWM", "A"),
        PinFunction("RTS0", "USART", "C"),
        PinFunction("PIOA_7", "PIO", "PIO"),
    ],
    "PA8": [
        PinFunction("PWMC0_PWML3", "PWM", "A"),
        PinFunction("AFE0_ADTRG", "ADC", "C"),
        PinFunction("PIOA_8", "PIO", "PIO"),
    ],
    "PA9": [
        PinFunction("URXD0", "USART", "A"),
        PinFunction("ISI_D3", "ISI", "B"),
        PinFunction("PIOA_9", "PIO", "PIO"),
    ],
    "PA10": [
        PinFunction("UTXD0", "USART", "A"),
        PinFunction("ISI_D4", "ISI", "B"),
        PinFunction("PIOA_10", "PIO", "PIO"),
    ],
    "PA11": [
        PinFunction("QSP_I_CS", "QSPI", "A"),
        PinFunction("PWMC0_PWMH0", "PWM", "B"),
        PinFunction("PIOA_11", "PIO", "PIO"),
    ],
    "PA12": [
        PinFunction("MCCI", "HSMCI", "A"),
        PinFunction("PWMC0_PWMH1", "PWM", "B"),
        PinFunction("PIOA_12", "PIO", "PIO"),
    ],
    "PA13": [
        PinFunction("MCCK", "HSMCI", "A"),
        PinFunction("TC0_TCLK0", "TC", "B"),
        PinFunction("PIOA_13", "PIO", "PIO"),
    ],
    "PA14": [
        PinFunction("MCDA0", "HSMCI", "A"),
        PinFunction("TC0_TIOA1", "TC", "B"),
        PinFunction("PIOA_14", "PIO", "PIO"),
    ],
    "PA15": [
        PinFunction("MCDA1", "HSMCI", "A"),
        PinFunction("TC0_TIOB1", "TC", "B"),
        PinFunction("PIOA_15", "PIO", "PIO"),
    ],
    "PA16": [
        PinFunction("MCDA2", "HSMCI", "A"),
        PinFunction("TC0_TIOA2", "TC", "B"),
        PinFunction("PIOA_16", "PIO", "PIO"),
    ],
    "PA17": [
        PinFunction("MCDA3", "HSMCI", "A"),
        PinFunction("TC0_TIOB2", "TC", "B"),
        PinFunction("PIOA_17", "PIO", "PIO"),
    ],
    "PA18": [
        PinFunction("PCK0", "PMC", "A"),
        PinFunction("ISI_D6", "ISI", "B"),
        PinFunction("PIOA_18", "PIO", "PIO"),
    ],
    "PA19": [
        PinFunction("MCDA1", "HSMCI", "A"),
        PinFunction("ISI_D7", "ISI", "B"),
        PinFunction("PIOA_19", "PIO", "PIO"),
    ],
    "PA20": [
        PinFunction("MCDA0", "HSMCI", "A"),
        PinFunction("ISI_D8", "ISI", "B"),
        PinFunction("PIOA_20", "PIO", "PIO"),
    ],
    "PA21": [
        PinFunction("RXD1", "USART", "A"),
        PinFunction("PCK1", "PMC", "B"),
        PinFunction("PIOA_21", "PIO", "PIO"),
    ],
    "PA22": [
        PinFunction("TXD1", "USART", "A"),
        PinFunction("SPI0_SPCK", "SPI", "C"),
        PinFunction("PIOA_22", "PIO", "PIO"),
    ],
    "PA23": [
        PinFunction("SCK1", "USART", "A"),
        PinFunction("SPI0_MOSI", "SPI", "C"),
        PinFunction("PIOA_23", "PIO", "PIO"),
    ],
    "PA24": [
        PinFunction("RTS1", "USART", "A"),
        PinFunction("SPI0_MISO", "SPI", "C"),
        PinFunction("PIOA_24", "PIO", "PIO"),
    ],
    "PA25": [
        PinFunction("CTS1", "USART", "A"),
        PinFunction("SPI0_NPCS0", "SPI", "C"),
        PinFunction("PIOA_25", "PIO", "PIO"),
    ],
    "PA26": [
        PinFunction("DCD1", "USART", "A"),
        PinFunction("TC0_TIOA2", "TC", "B"),
        PinFunction("PIOA_26", "PIO", "PIO"),
    ],
    "PA27": [
        PinFunction("DTR1", "USART", "A"),
        PinFunction("TC0_TIOB2", "TC", "B"),
        PinFunction("PIOA_27", "PIO", "PIO"),
    ],
    "PA28": [
        PinFunction("DSR1", "USART", "A"),
        PinFunction("TC0_TCLK1", "TC", "B"),
        PinFunction("PIOA_28", "PIO", "PIO"),
    ],
    "PA29": [
        PinFunction("RI1", "USART", "A"),
        PinFunction("TC0_TCLK2", "TC", "B"),
        PinFunction("PIOA_29", "PIO", "PIO"),
    ],
    "PA30": [
        PinFunction("PWMC0_PWMH2", "PWM", "A"),
        PinFunction("SPI0_NPCS2", "SPI", "B"),
        PinFunction("PIOA_30", "PIO", "PIO"),
    ],
    "PA31": [
        PinFunction("SPI0_NPCS3", "SPI", "A"),
        PinFunction("TC0_TIOA0", "TC", "B"),
        PinFunction("PIOA_31", "PIO", "PIO"),
    ],

    # Port B
    "PB0": [
        PinFunction("AFE0_AD10", "ADC", "A"),
        PinFunction("PWMC0_PWMH0", "PWM", "B"),
        PinFunction("PIOB_0", "PIO", "PIO"),
    ],
    "PB1": [
        PinFunction("AFE1_AD0", "ADC", "A"),
        PinFunction("PWMC0_PWMH1", "PWM", "B"),
        PinFunction("PIOB_1", "PIO", "PIO"),
    ],
    "PB2": [
        PinFunction("CANTX0", "CAN", "A"),
        PinFunction("TWD1", "TWI", "B"),
        PinFunction("PIOB_2", "PIO", "PIO"),
    ],
    "PB3": [
        PinFunction("CANRX0", "CAN", "A"),
        PinFunction("TWCK1", "TWI", "B"),
        PinFunction("PIOB_3", "PIO", "PIO"),
    ],
    "PB4": [
        PinFunction("TWD1", "TWI", "A"),
        PinFunction("PWMC0_PWMH2", "PWM", "B"),
        PinFunction("PIOB_4", "PIO", "PIO"),
    ],
    "PB5": [
        PinFunction("TWCK1", "TWI", "A"),
        PinFunction("PWMC0_PWML0", "PWM", "B"),
        PinFunction("PIOB_5", "PIO", "PIO"),
    ],
    "PB6": [
        PinFunction("TXD1", "USART", "A"),
        PinFunction("ISI_VSYNC", "ISI", "B"),
        PinFunction("PIOB_6", "PIO", "PIO"),
    ],
    "PB7": [
        PinFunction("RXD1", "USART", "A"),
        PinFunction("ISI_HSYNC", "ISI", "B"),
        PinFunction("PIOB_7", "PIO", "PIO"),
    ],
    "PB8": [
        PinFunction("TXD2", "USART", "A"),
        PinFunction("ISI_PCK", "ISI", "B"),
        PinFunction("PIOB_8", "PIO", "PIO"),
    ],
    "PB9": [
        PinFunction("RXD2", "USART", "A"),
        PinFunction("ISI_D0", "ISI", "B"),
        PinFunction("PIOB_9", "PIO", "PIO"),
    ],
    "PB10": [
        PinFunction("TXD3", "USART", "A"),
        PinFunction("ISI_D1", "ISI", "B"),
        PinFunction("PIOB_10", "PIO", "PIO"),
    ],
    "PB11": [
        PinFunction("RXD3", "USART", "A"),
        PinFunction("ISI_D2", "ISI", "B"),
        PinFunction("PIOB_11", "PIO", "PIO"),
    ],
    "PB12": [
        PinFunction("TWD0", "TWI", "A"),
        PinFunction("ISI_D5", "ISI", "B"),
        PinFunction("PIOB_12", "PIO", "PIO"),
    ],
    "PB13": [
        PinFunction("TWCK0", "TWI", "A"),
        PinFunction("PCK0", "PMC", "B"),
        PinFunction("PIOB_13", "PIO", "PIO"),
    ],

    # Port C
    "PC0": [
        PinFunction("EBIA10", "EBI", "A"),
        PinFunction("PWMC0_PWMH0", "PWM", "B"),
        PinFunction("PIOC_0", "PIO", "PIO"),
    ],
    "PC1": [
        PinFunction("EBIA11", "EBI", "A"),
        PinFunction("PWMC0_PWMH1", "PWM", "B"),
        PinFunction("PIOC_1", "PIO", "PIO"),
    ],
    "PC2": [
        PinFunction("EBIA12", "EBI", "A"),
        PinFunction("PWMC0_PWMH2", "PWM", "B"),
        PinFunction("PIOC_2", "PIO", "PIO"),
    ],
    "PC3": [
        PinFunction("EBIA13", "EBI", "A"),
        PinFunction("PWMC0_PWMH3", "PWM", "B"),
        PinFunction("PIOC_3", "PIO", "PIO"),
    ],
    "PC4": [
        PinFunction("EBIA14", "EBI", "A"),
        PinFunction("SPI0_NPCS1", "SPI", "C"),
        PinFunction("PIOC_4", "PIO", "PIO"),
    ],
    "PC5": [
        PinFunction("EBIA15", "EBI", "A"),
        PinFunction("TC1_TIOA0", "TC", "C"),
        PinFunction("PIOC_5", "PIO", "PIO"),
    ],
    "PC6": [
        PinFunction("EBIA16", "EBI", "A"),
        PinFunction("TC1_TIOB0", "TC", "C"),
        PinFunction("PIOC_6", "PIO", "PIO"),
    ],
    "PC7": [
        PinFunction("EBIA17", "EBI", "A"),
        PinFunction("TC1_TCLK0", "TC", "C"),
        PinFunction("PIOC_7", "PIO", "PIO"),
    ],
    "PC8": [
        PinFunction("EBIA18", "EBI", "A"),
        PinFunction("PWMC0_PWML0", "PWM", "B"),
        PinFunction("PIOC_8", "PIO", "PIO"),
    ],
    "PC9": [
        PinFunction("EBIA19", "EBI", "A"),
        PinFunction("PWMC0_PWML1", "PWM", "B"),
        PinFunction("PIOC_9", "PIO", "PIO"),
    ],
    "PC10": [
        PinFunction("EBIA20", "EBI", "A"),
        PinFunction("PWMC0_PWML2", "PWM", "B"),
        PinFunction("PIOC_10", "PIO", "PIO"),
    ],
    "PC11": [
        PinFunction("EBIA21", "EBI", "A"),
        PinFunction("PWMC0_PWML3", "PWM", "B"),
        PinFunction("PIOC_11", "PIO", "PIO"),
    ],
    "PC12": [
        PinFunction("EBIA22", "EBI", "A"),
        PinFunction("PWMC0_PWMH0", "PWM", "B"),
        PinFunction("PIOC_12", "PIO", "PIO"),
    ],
    "PC13": [
        PinFunction("EBIA23", "EBI", "A"),
        PinFunction("PWMC0_PWMH1", "PWM", "B"),
        PinFunction("PIOC_13", "PIO", "PIO"),
    ],
    "PC14": [
        PinFunction("EBINWR0", "EBI", "A"),
        PinFunction("PWMC0_PWMH2", "PWM", "B"),
        PinFunction("PIOC_14", "PIO", "PIO"),
    ],
    "PC15": [
        PinFunction("EBINCS0", "EBI", "A"),
        PinFunction("PWMC0_PWMH3", "PWM", "B"),
        PinFunction("PIOC_15", "PIO", "PIO"),
    ],
    "PC16": [
        PinFunction("EBID16", "EBI", "A"),
        PinFunction("SPI0_MISO", "SPI", "C"),
        PinFunction("PIOC_16", "PIO", "PIO"),
    ],
    "PC17": [
        PinFunction("EBID17", "EBI", "A"),
        PinFunction("SPI0_MOSI", "SPI", "C"),
        PinFunction("PIOC_17", "PIO", "PIO"),
    ],
    "PC18": [
        PinFunction("EBID18", "EBI", "A"),
        PinFunction("SPI0_SPCK", "SPI", "C"),
        PinFunction("PIOC_18", "PIO", "PIO"),
    ],
    "PC19": [
        PinFunction("EBID19", "EBI", "A"),
        PinFunction("SPI0_NPCS0", "SPI", "C"),
        PinFunction("PIOC_19", "PIO", "PIO"),
    ],
    "PC20": [
        PinFunction("EBID20", "EBI", "A"),
        PinFunction("QSPI_IO3", "QSPI", "C"),
        PinFunction("PIOC_20", "PIO", "PIO"),
    ],
    "PC21": [
        PinFunction("EBID21", "EBI", "A"),
        PinFunction("QSPI_IO2", "QSPI", "C"),
        PinFunction("PIOC_21", "PIO", "PIO"),
    ],
    "PC22": [
        PinFunction("EBID22", "EBI", "A"),
        PinFunction("QSPI_IO1", "QSPI", "C"),
        PinFunction("PIOC_22", "PIO", "PIO"),
    ],
    "PC23": [
        PinFunction("EBID23", "EBI", "A"),
        PinFunction("QSPI_IO0", "QSPI", "C"),
        PinFunction("PIOC_23", "PIO", "PIO"),
    ],
    "PC24": [
        PinFunction("EBID24", "EBI", "A"),
        PinFunction("QSPI_SCK", "QSPI", "C"),
        PinFunction("PIOC_24", "PIO", "PIO"),
    ],
    "PC25": [
        PinFunction("EBID25", "EBI", "A"),
        PinFunction("QSPI_CS", "QSPI", "C"),
        PinFunction("PIOC_25", "PIO", "PIO"),
    ],
    "PC26": [
        PinFunction("EBID26", "EBI", "A"),
        PinFunction("TC1_TIOA1", "TC", "C"),
        PinFunction("PIOC_26", "PIO", "PIO"),
    ],
    "PC27": [
        PinFunction("EBID27", "EBI", "A"),
        PinFunction("TC1_TIOB1", "TC", "C"),
        PinFunction("PIOC_27", "PIO", "PIO"),
    ],
    "PC28": [
        PinFunction("EBID28", "EBI", "A"),
        PinFunction("TC1_TCLK1", "TC", "C"),
        PinFunction("PIOC_28", "PIO", "PIO"),
    ],
    "PC29": [
        PinFunction("EBID29", "EBI", "A"),
        PinFunction("TC1_TIOA2", "TC", "C"),
        PinFunction("PIOC_29", "PIO", "PIO"),
    ],
    "PC30": [
        PinFunction("EBID30", "EBI", "A"),
        PinFunction("TC1_TIOB2", "TC", "C"),
        PinFunction("PIOC_30", "PIO", "PIO"),
    ],
    "PC31": [
        PinFunction("EBID31", "EBI", "A"),
        PinFunction("TC1_TCLK2", "TC", "C"),
        PinFunction("PIOC_31", "PIO", "PIO"),
    ],

    # Port D
    "PD0": [
        PinFunction("GTSUCOMP", "GMAC", "A"),
        PinFunction("PWMC0_PWMH0", "PWM", "B"),
        PinFunction("PIOD_0", "PIO", "PIO"),
    ],
    "PD1": [
        PinFunction("GRXCK", "GMAC", "A"),
        PinFunction("PWMC0_PWMH1", "PWM", "B"),
        PinFunction("PIOD_1", "PIO", "PIO"),
    ],
    "PD2": [
        PinFunction("GTXCK", "GMAC", "A"),
        PinFunction("PWMC0_PWMH2", "PWM", "B"),
        PinFunction("PIOD_2", "PIO", "PIO"),
    ],
    "PD3": [
        PinFunction("GTXEN", "GMAC", "A"),
        PinFunction("PWMC0_PWMH3", "PWM", "B"),
        PinFunction("PIOD_3", "PIO", "PIO"),
    ],
    "PD4": [
        PinFunction("GRXDV", "GMAC", "A"),
        PinFunction("PWMC0_PWML0", "PWM", "B"),
        PinFunction("PIOD_4", "PIO", "PIO"),
    ],
    "PD5": [
        PinFunction("GRX0", "GMAC", "A"),
        PinFunction("PWMC0_PWML1", "PWM", "B"),
        PinFunction("PIOD_5", "PIO", "PIO"),
    ],
    "PD6": [
        PinFunction("GRX1", "GMAC", "A"),
        PinFunction("PWMC0_PWML2", "PWM", "B"),
        PinFunction("PIOD_6", "PIO", "PIO"),
    ],
    "PD7": [
        PinFunction("GRXER", "GMAC", "A"),
        PinFunction("PWMC0_PWML3", "PWM", "B"),
        PinFunction("PIOD_7", "PIO", "PIO"),
    ],
    "PD8": [
        PinFunction("GMDC", "GMAC", "A"),
        PinFunction("PWMC0_PWMFI0", "PWM", "B"),
        PinFunction("PIOD_8", "PIO", "PIO"),
    ],
    "PD9": [
        PinFunction("GMDIO", "GMAC", "A"),
        PinFunction("PWMC0_PWMFI1", "PWM", "B"),
        PinFunction("PIOD_9", "PIO", "PIO"),
    ],
    "PD10": [
        PinFunction("GTX0", "GMAC", "A"),
        PinFunction("PWMC0_PWMEXTRG0", "PWM", "B"),
        PinFunction("PIOD_10", "PIO", "PIO"),
    ],
    "PD11": [
        PinFunction("GTX1", "GMAC", "A"),
        PinFunction("PWMC0_PWMEXTRG1", "PWM", "B"),
        PinFunction("PIOD_11", "PIO", "PIO"),
    ],
    "PD12": [
        PinFunction("GTXER", "GMAC", "A"),
        PinFunction("PWMC0_PWMFI0", "PWM", "B"),
        PinFunction("PIOD_12", "PIO", "PIO"),
    ],
    "PD13": [
        PinFunction("GCOL", "GMAC", "A"),
        PinFunction("PWMC0_PWMFI1", "PWM", "B"),
        PinFunction("PIOD_13", "PIO", "PIO"),
    ],
    "PD14": [
        PinFunction("GRXCK", "GMAC", "A"),
        PinFunction("PWMC0_PWMFI2", "PWM", "B"),
        PinFunction("PIOD_14", "PIO", "PIO"),
    ],
    "PD15": [
        PinFunction("GTX2", "GMAC", "A"),
        PinFunction("TC2_TIOA0", "TC", "C"),
        PinFunction("PIOD_15", "PIO", "PIO"),
    ],
    "PD16": [
        PinFunction("GTX3", "GMAC", "A"),
        PinFunction("TC2_TIOB0", "TC", "C"),
        PinFunction("PIOD_16", "PIO", "PIO"),
    ],
    "PD17": [
        PinFunction("GRX2", "GMAC", "A"),
        PinFunction("TC2_TCLK0", "TC", "C"),
        PinFunction("PIOD_17", "PIO", "PIO"),
    ],
    "PD18": [
        PinFunction("GRX3", "GMAC", "A"),
        PinFunction("TC2_TIOA1", "TC", "C"),
        PinFunction("PIOD_18", "PIO", "PIO"),
    ],
    "PD19": [
        PinFunction("GCRS", "GMAC", "A"),
        PinFunction("TC2_TIOB1", "TC", "C"),
        PinFunction("PIOD_19", "PIO", "PIO"),
    ],
    "PD20": [
        PinFunction("URXD2", "USART", "A"),
        PinFunction("TC2_TCLK1", "TC", "C"),
        PinFunction("PIOD_20", "PIO", "PIO"),
    ],
    "PD21": [
        PinFunction("UTXD2", "USART", "A"),
        PinFunction("TC2_TIOA2", "TC", "C"),
        PinFunction("PIOD_21", "PIO", "PIO"),
    ],
    "PD22": [
        PinFunction("SCK2", "USART", "A"),
        PinFunction("TC2_TIOB2", "TC", "C"),
        PinFunction("PIOD_22", "PIO", "PIO"),
    ],
    "PD23": [
        PinFunction("RTS2", "USART", "A"),
        PinFunction("TC2_TCLK2", "TC", "C"),
        PinFunction("PIOD_23", "PIO", "PIO"),
    ],
    "PD24": [
        PinFunction("CTS2", "USART", "A"),
        PinFunction("PWMC0_PWMH0", "PWM", "B"),
        PinFunction("PIOD_24", "PIO", "PIO"),
    ],
    "PD25": [
        PinFunction("RXD2", "USART", "A"),
        PinFunction("TC0_TIOA0", "TC", "C"),
        PinFunction("PIOD_25", "PIO", "PIO"),
    ],
    "PD26": [
        PinFunction("TXD2", "USART", "A"),
        PinFunction("TC0_TIOB0", "TC", "C"),
        PinFunction("PIOD_26", "PIO", "PIO"),
    ],
    "PD27": [
        PinFunction("PCK0", "PMC", "A"),
        PinFunction("TC0_TCLK0", "TC", "C"),
        PinFunction("PIOD_27", "PIO", "PIO"),
    ],
    "PD28": [
        PinFunction("SPI0_NPCS3", "SPI", "A"),
        PinFunction("TC0_TIOA1", "TC", "C"),
        PinFunction("PIOD_28", "PIO", "PIO"),
    ],
    "PD29": [
        PinFunction("TXD3", "USART", "A"),
        PinFunction("TC0_TIOB1", "TC", "C"),
        PinFunction("PIOD_29", "PIO", "PIO"),
    ],
    "PD30": [
        PinFunction("RXD3", "USART", "A"),
        PinFunction("TC0_TCLK1", "TC", "C"),
        PinFunction("PIOD_30", "PIO", "PIO"),
    ],
    "PD31": [
        PinFunction("SCK3", "USART", "A"),
        PinFunction("TC0_TIOA2", "TC", "C"),
        PinFunction("PIOD_31", "PIO", "PIO"),
    ],

    # Port E (limited pins on some packages)
    "PE0": [
        PinFunction("LCDDAT0", "LCD", "A"),
        PinFunction("PWMC0_PWMH0", "PWM", "B"),
        PinFunction("PIOE_0", "PIO", "PIO"),
    ],
    "PE1": [
        PinFunction("LCDDAT1", "LCD", "A"),
        PinFunction("PWMC0_PWMH1", "PWM", "B"),
        PinFunction("PIOE_1", "PIO", "PIO"),
    ],
    "PE2": [
        PinFunction("LCDDAT2", "LCD", "A"),
        PinFunction("PWMC0_PWMH2", "PWM", "B"),
        PinFunction("PIOE_2", "PIO", "PIO"),
    ],
    "PE3": [
        PinFunction("LCDDAT3", "LCD", "A"),
        PinFunction("PWMC0_PWMH3", "PWM", "B"),
        PinFunction("PIOE_3", "PIO", "PIO"),
    ],
    "PE4": [
        PinFunction("LCDDAT4", "LCD", "A"),
        PinFunction("PWMC0_PWML0", "PWM", "B"),
        PinFunction("PIOE_4", "PIO", "PIO"),
    ],
    "PE5": [
        PinFunction("LCDDAT5", "LCD", "A"),
        PinFunction("PWMC0_PWML1", "PWM", "B"),
        PinFunction("PIOE_5", "PIO", "PIO"),
    ],
}


def get_pin_functions(pin_name: str) -> List[PinFunction]:
    """
    Get alternate functions for a specific pin.

    Args:
        pin_name: Pin name in format "PAx", "PBx", "PCx", "PDx", or "PEx"

    Returns:
        List of PinFunction objects for this pin
    """
    return SAME70_PIN_FUNCTIONS.get(pin_name.upper(), [])


def get_all_pins() -> List[str]:
    """Get list of all available pins"""
    return sorted(SAME70_PIN_FUNCTIONS.keys())


def get_peripheral_pins(peripheral_type: str) -> Dict[str, List[PinFunction]]:
    """
    Get all pins that support a specific peripheral type.

    Args:
        peripheral_type: Peripheral type (e.g., "UART", "SPI", "TWI")

    Returns:
        Dict mapping pin names to their functions for this peripheral
    """
    result = {}
    for pin_name, functions in SAME70_PIN_FUNCTIONS.items():
        matching_funcs = [f for f in functions if f.peripheral_type == peripheral_type]
        if matching_funcs:
            result[pin_name] = matching_funcs
    return result


# Example usage and validation
if __name__ == "__main__":
    print("SAME70 Pin Function Database")
    print("=" * 60)
    print(f"Total pins defined: {len(SAME70_PIN_FUNCTIONS)}")
    print()

    # Show example pins
    example_pins = ["PA5", "PA6", "PB2", "PB3", "PD0"]
    for pin in example_pins:
        funcs = get_pin_functions(pin)
        print(f"{pin}:")
        for func in funcs:
            print(f"  • {func.function_name:<20} ({func.peripheral_type}, Periph {func.peripheral})")
        print()

    # Show peripheral summary
    peripheral_types = set()
    for functions in SAME70_PIN_FUNCTIONS.values():
        for func in functions:
            peripheral_types.add(func.peripheral_type)

    print("Supported peripheral types:")
    for ptype in sorted(peripheral_types):
        count = len(get_peripheral_pins(ptype))
        print(f"  • {ptype:<10} - {count} pins")
