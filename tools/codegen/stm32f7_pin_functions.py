#!/usr/bin/env python3
"""
STM32F7 Pin Function Database

Complete pin function mappings for STM32F7 family microcontrollers.
STM32F7 uses the modern GPIO architecture with AF0-AF15.

Architecture: ARM Cortex-M7
GPIO Registers: MODER, OTYPER, OSPEEDR, PUPDR, AFR[2]
Alternate Functions: AF0-AF15 (same as STM32F4)
"""

from typing import List, Dict
from dataclasses import dataclass


@dataclass
class PinFunction:
    """Represents a pin's alternate function"""
    name: str           # e.g., "USART1_TX"
    peripheral: str     # e.g., "USART", "I2C", "SPI"
    af_number: int      # Alternate function number (0-15)


def get_pin_functions(pin_name: str) -> List[PinFunction]:
    """Get all alternate functions for a given pin"""
    return STM32F7_PIN_FUNCTIONS.get(pin_name, [])


# ============================================================================
# STM32F7 Pin Function Database
# Based on STM32F7 datasheets (F722, F723, F730, F745, F746, F750, F765, F767, F769)
# ============================================================================

STM32F7_PIN_FUNCTIONS = {
    # Port A pins
    "PA0": [
        PinFunction("TIM2_CH1", "TIM", 1),
        PinFunction("TIM2_ETR", "TIM", 1),
        PinFunction("TIM5_CH1", "TIM", 2),
        PinFunction("TIM8_ETR", "TIM", 3),
        PinFunction("USART2_CTS", "USART", 7),
        PinFunction("UART4_TX", "UART", 8),
        PinFunction("SAI2_SD_B", "SAI", 10),
        PinFunction("ETH_MII_CRS", "ETH", 11),
        PinFunction("ADC123_IN0", "ADC", 0),
        PinFunction("WKUP", "SYSTEM", 0),
    ],
    "PA1": [
        PinFunction("TIM2_CH2", "TIM", 1),
        PinFunction("TIM5_CH2", "TIM", 2),
        PinFunction("USART2_RTS", "USART", 7),
        PinFunction("UART4_RX", "UART", 8),
        PinFunction("QUADSPI_BK1_IO3", "QUADSPI", 9),
        PinFunction("SAI2_MCLK_B", "SAI", 10),
        PinFunction("ETH_MII_RX_CLK", "ETH", 11),
        PinFunction("ETH_RMII_REF_CLK", "ETH", 11),
        PinFunction("ADC123_IN1", "ADC", 0),
    ],
    "PA2": [
        PinFunction("TIM2_CH3", "TIM", 1),
        PinFunction("TIM5_CH3", "TIM", 2),
        PinFunction("TIM9_CH1", "TIM", 3),
        PinFunction("USART2_TX", "USART", 7),
        PinFunction("SAI2_SCK_B", "SAI", 8),
        PinFunction("ETH_MDIO", "ETH", 11),
        PinFunction("ADC123_IN2", "ADC", 0),
    ],
    "PA3": [
        PinFunction("TIM2_CH4", "TIM", 1),
        PinFunction("TIM5_CH4", "TIM", 2),
        PinFunction("TIM9_CH2", "TIM", 3),
        PinFunction("USART2_RX", "USART", 7),
        PinFunction("OTG_HS_ULPI_D0", "USB", 10),
        PinFunction("ETH_MII_COL", "ETH", 11),
        PinFunction("ADC123_IN3", "ADC", 0),
    ],
    "PA4": [
        PinFunction("SPI1_NSS", "SPI", 5),
        PinFunction("SPI3_NSS", "SPI", 6),
        PinFunction("I2S3_WS", "I2S", 6),
        PinFunction("USART2_CK", "USART", 7),
        PinFunction("DCMI_HSYNC", "DCMI", 13),
        PinFunction("OTG_HS_SOF", "USB", 12),
        PinFunction("ADC12_IN4", "ADC", 0),
        PinFunction("DAC_OUT1", "DAC", 0),
    ],
    "PA5": [
        PinFunction("TIM2_CH1", "TIM", 1),
        PinFunction("TIM2_ETR", "TIM", 1),
        PinFunction("TIM8_CH1N", "TIM", 3),
        PinFunction("SPI1_SCK", "SPI", 5),
        PinFunction("OTG_HS_ULPI_CK", "USB", 10),
        PinFunction("ADC12_IN5", "ADC", 0),
        PinFunction("DAC_OUT2", "DAC", 0),
    ],
    "PA6": [
        PinFunction("TIM1_BKIN", "TIM", 1),
        PinFunction("TIM3_CH1", "TIM", 2),
        PinFunction("TIM8_BKIN", "TIM", 3),
        PinFunction("SPI1_MISO", "SPI", 5),
        PinFunction("TIM13_CH1", "TIM", 9),
        PinFunction("DCMI_PIXCLK", "DCMI", 13),
        PinFunction("ADC12_IN6", "ADC", 0),
    ],
    "PA7": [
        PinFunction("TIM1_CH1N", "TIM", 1),
        PinFunction("TIM3_CH2", "TIM", 2),
        PinFunction("TIM8_CH1N", "TIM", 3),
        PinFunction("SPI1_MOSI", "SPI", 5),
        PinFunction("TIM14_CH1", "TIM", 9),
        PinFunction("ETH_MII_RX_DV", "ETH", 11),
        PinFunction("ETH_RMII_CRS_DV", "ETH", 11),
        PinFunction("ADC12_IN7", "ADC", 0),
    ],
    "PA8": [
        PinFunction("MCO1", "RCC", 0),
        PinFunction("TIM1_CH1", "TIM", 1),
        PinFunction("I2C3_SCL", "I2C", 4),
        PinFunction("USART1_CK", "USART", 7),
        PinFunction("OTG_FS_SOF", "USB", 10),
        PinFunction("UART7_RX", "UART", 12),
    ],
    "PA9": [
        PinFunction("TIM1_CH2", "TIM", 1),
        PinFunction("I2C3_SMBA", "I2C", 4),
        PinFunction("USART1_TX", "USART", 7),
        PinFunction("DCMI_D0", "DCMI", 13),
    ],
    "PA10": [
        PinFunction("TIM1_CH3", "TIM", 1),
        PinFunction("USART1_RX", "USART", 7),
        PinFunction("OTG_FS_ID", "USB", 10),
        PinFunction("DCMI_D1", "DCMI", 13),
    ],
    "PA11": [
        PinFunction("TIM1_CH4", "TIM", 1),
        PinFunction("USART1_CTS", "USART", 7),
        PinFunction("CAN1_RX", "CAN", 9),
        PinFunction("OTG_FS_DM", "USB", 10),
    ],
    "PA12": [
        PinFunction("TIM1_ETR", "TIM", 1),
        PinFunction("USART1_RTS", "USART", 7),
        PinFunction("CAN1_TX", "CAN", 9),
        PinFunction("OTG_FS_DP", "USB", 10),
    ],
    "PA13": [
        PinFunction("JTMS", "JTAG", 0),
        PinFunction("SWDIO", "SWD", 0),
    ],
    "PA14": [
        PinFunction("JTCK", "JTAG", 0),
        PinFunction("SWCLK", "SWD", 0),
    ],
    "PA15": [
        PinFunction("JTDI", "JTAG", 0),
        PinFunction("TIM2_CH1", "TIM", 1),
        PinFunction("TIM2_ETR", "TIM", 1),
        PinFunction("SPI1_NSS", "SPI", 5),
        PinFunction("SPI3_NSS", "SPI", 6),
        PinFunction("I2S3_WS", "I2S", 6),
        PinFunction("UART4_RTS", "UART", 8),
    ],

    # Port B pins
    "PB0": [
        PinFunction("TIM1_CH2N", "TIM", 1),
        PinFunction("TIM3_CH3", "TIM", 2),
        PinFunction("TIM8_CH2N", "TIM", 3),
        PinFunction("UART4_CTS", "UART", 8),
        PinFunction("ETH_MII_RXD2", "ETH", 11),
        PinFunction("OTG_HS_ULPI_D1", "USB", 10),
        PinFunction("ADC12_IN8", "ADC", 0),
    ],
    "PB1": [
        PinFunction("TIM1_CH3N", "TIM", 1),
        PinFunction("TIM3_CH4", "TIM", 2),
        PinFunction("TIM8_CH3N", "TIM", 3),
        PinFunction("ETH_MII_RXD3", "ETH", 11),
        PinFunction("OTG_HS_ULPI_D2", "USB", 10),
        PinFunction("ADC12_IN9", "ADC", 0),
    ],
    "PB2": [
        PinFunction("QUADSPI_CLK", "QUADSPI", 9),
    ],
    "PB3": [
        PinFunction("JTDO", "JTAG", 0),
        PinFunction("TRACESWO", "TRACE", 0),
        PinFunction("TIM2_CH2", "TIM", 1),
        PinFunction("SPI1_SCK", "SPI", 5),
        PinFunction("SPI3_SCK", "SPI", 6),
        PinFunction("I2S3_CK", "I2S", 6),
    ],
    "PB4": [
        PinFunction("NJTRST", "JTAG", 0),
        PinFunction("TIM3_CH1", "TIM", 2),
        PinFunction("SPI1_MISO", "SPI", 5),
        PinFunction("SPI3_MISO", "SPI", 6),
        PinFunction("I2S3ext_SD", "I2S", 7),
    ],
    "PB5": [
        PinFunction("TIM3_CH2", "TIM", 2),
        PinFunction("I2C1_SMBA", "I2C", 4),
        PinFunction("SPI1_MOSI", "SPI", 5),
        PinFunction("SPI3_MOSI", "SPI", 6),
        PinFunction("I2S3_SD", "I2S", 6),
        PinFunction("CAN2_RX", "CAN", 9),
        PinFunction("OTG_HS_ULPI_D7", "USB", 10),
        PinFunction("ETH_PPS_OUT", "ETH", 11),
        PinFunction("DCMI_D10", "DCMI", 13),
    ],
    "PB6": [
        PinFunction("TIM4_CH1", "TIM", 2),
        PinFunction("I2C1_SCL", "I2C", 4),
        PinFunction("USART1_TX", "USART", 7),
        PinFunction("CAN2_TX", "CAN", 9),
        PinFunction("QUADSPI_BK1_NCS", "QUADSPI", 10),
        PinFunction("DCMI_D5", "DCMI", 13),
    ],
    "PB7": [
        PinFunction("TIM4_CH2", "TIM", 2),
        PinFunction("I2C1_SDA", "I2C", 4),
        PinFunction("USART1_RX", "USART", 7),
        PinFunction("DCMI_VSYNC", "DCMI", 13),
    ],
    "PB8": [
        PinFunction("TIM4_CH3", "TIM", 2),
        PinFunction("TIM10_CH1", "TIM", 3),
        PinFunction("I2C1_SCL", "I2C", 4),
        PinFunction("CAN1_RX", "CAN", 9),
        PinFunction("SDMMC1_D4", "SDMMC", 12),
        PinFunction("DCMI_D6", "DCMI", 13),
    ],
    "PB9": [
        PinFunction("TIM4_CH4", "TIM", 2),
        PinFunction("TIM11_CH1", "TIM", 3),
        PinFunction("I2C1_SDA", "I2C", 4),
        PinFunction("SPI2_NSS", "SPI", 5),
        PinFunction("I2S2_WS", "I2S", 5),
        PinFunction("CAN1_TX", "CAN", 9),
        PinFunction("SDMMC1_D5", "SDMMC", 12),
        PinFunction("DCMI_D7", "DCMI", 13),
    ],
    "PB10": [
        PinFunction("TIM2_CH3", "TIM", 1),
        PinFunction("I2C2_SCL", "I2C", 4),
        PinFunction("SPI2_SCK", "SPI", 5),
        PinFunction("I2S2_CK", "I2S", 5),
        PinFunction("USART3_TX", "USART", 7),
        PinFunction("OTG_HS_ULPI_D3", "USB", 10),
        PinFunction("ETH_MII_RX_ER", "ETH", 11),
    ],
    "PB11": [
        PinFunction("TIM2_CH4", "TIM", 1),
        PinFunction("I2C2_SDA", "I2C", 4),
        PinFunction("USART3_RX", "USART", 7),
        PinFunction("OTG_HS_ULPI_D4", "USB", 10),
        PinFunction("ETH_MII_TX_EN", "ETH", 11),
        PinFunction("ETH_RMII_TX_EN", "ETH", 11),
    ],
    "PB12": [
        PinFunction("TIM1_BKIN", "TIM", 1),
        PinFunction("I2C2_SMBA", "I2C", 4),
        PinFunction("SPI2_NSS", "SPI", 5),
        PinFunction("I2S2_WS", "I2S", 5),
        PinFunction("USART3_CK", "USART", 7),
        PinFunction("CAN2_RX", "CAN", 9),
        PinFunction("OTG_HS_ULPI_D5", "USB", 10),
        PinFunction("ETH_MII_TXD0", "ETH", 11),
        PinFunction("ETH_RMII_TXD0", "ETH", 11),
        PinFunction("OTG_HS_ID", "USB", 12),
    ],
    "PB13": [
        PinFunction("TIM1_CH1N", "TIM", 1),
        PinFunction("SPI2_SCK", "SPI", 5),
        PinFunction("I2S2_CK", "I2S", 5),
        PinFunction("USART3_CTS", "USART", 7),
        PinFunction("CAN2_TX", "CAN", 9),
        PinFunction("OTG_HS_ULPI_D6", "USB", 10),
        PinFunction("ETH_MII_TXD1", "ETH", 11),
        PinFunction("ETH_RMII_TXD1", "ETH", 11),
    ],
    "PB14": [
        PinFunction("TIM1_CH2N", "TIM", 1),
        PinFunction("TIM8_CH2N", "TIM", 3),
        PinFunction("SPI2_MISO", "SPI", 5),
        PinFunction("I2S2ext_SD", "I2S", 6),
        PinFunction("USART3_RTS", "USART", 7),
        PinFunction("TIM12_CH1", "TIM", 9),
        PinFunction("OTG_HS_DM", "USB", 12),
    ],
    "PB15": [
        PinFunction("RTC_REFIN", "RTC", 0),
        PinFunction("TIM1_CH3N", "TIM", 1),
        PinFunction("TIM8_CH3N", "TIM", 3),
        PinFunction("SPI2_MOSI", "SPI", 5),
        PinFunction("I2S2_SD", "I2S", 5),
        PinFunction("TIM12_CH2", "TIM", 9),
        PinFunction("OTG_HS_DP", "USB", 12),
    ],

    # Port C pins
    "PC0": [
        PinFunction("OTG_HS_ULPI_STP", "USB", 10),
        PinFunction("FMC_SDNWE", "FMC", 12),
        PinFunction("ADC123_IN10", "ADC", 0),
    ],
    "PC1": [
        PinFunction("ETH_MDC", "ETH", 11),
        PinFunction("ADC123_IN11", "ADC", 0),
    ],
    "PC2": [
        PinFunction("SPI2_MISO", "SPI", 5),
        PinFunction("I2S2ext_SD", "I2S", 6),
        PinFunction("OTG_HS_ULPI_DIR", "USB", 10),
        PinFunction("ETH_MII_TXD2", "ETH", 11),
        PinFunction("FMC_SDNE0", "FMC", 12),
        PinFunction("ADC123_IN12", "ADC", 0),
    ],
    "PC3": [
        PinFunction("SPI2_MOSI", "SPI", 5),
        PinFunction("I2S2_SD", "I2S", 5),
        PinFunction("OTG_HS_ULPI_NXT", "USB", 10),
        PinFunction("ETH_MII_TX_CLK", "ETH", 11),
        PinFunction("FMC_SDCKE0", "FMC", 12),
        PinFunction("ADC123_IN13", "ADC", 0),
    ],
    "PC4": [
        PinFunction("ETH_MII_RXD0", "ETH", 11),
        PinFunction("ETH_RMII_RXD0", "ETH", 11),
        PinFunction("ADC12_IN14", "ADC", 0),
    ],
    "PC5": [
        PinFunction("ETH_MII_RXD1", "ETH", 11),
        PinFunction("ETH_RMII_RXD1", "ETH", 11),
        PinFunction("ADC12_IN15", "ADC", 0),
    ],
    "PC6": [
        PinFunction("TIM3_CH1", "TIM", 2),
        PinFunction("TIM8_CH1", "TIM", 3),
        PinFunction("I2S2_MCK", "I2S", 5),
        PinFunction("USART6_TX", "USART", 8),
        PinFunction("SDMMC1_D6", "SDMMC", 12),
        PinFunction("DCMI_D0", "DCMI", 13),
    ],
    "PC7": [
        PinFunction("TIM3_CH2", "TIM", 2),
        PinFunction("TIM8_CH2", "TIM", 3),
        PinFunction("I2S3_MCK", "I2S", 6),
        PinFunction("USART6_RX", "USART", 8),
        PinFunction("SDMMC1_D7", "SDMMC", 12),
        PinFunction("DCMI_D1", "DCMI", 13),
    ],
    "PC8": [
        PinFunction("TIM3_CH3", "TIM", 2),
        PinFunction("TIM8_CH3", "TIM", 3),
        PinFunction("UART5_RTS", "UART", 7),
        PinFunction("USART6_CK", "USART", 8),
        PinFunction("SDMMC1_D0", "SDMMC", 12),
        PinFunction("DCMI_D2", "DCMI", 13),
    ],
    "PC9": [
        PinFunction("MCO2", "RCC", 0),
        PinFunction("TIM3_CH4", "TIM", 2),
        PinFunction("TIM8_CH4", "TIM", 3),
        PinFunction("I2C3_SDA", "I2C", 4),
        PinFunction("I2S_CKIN", "I2S", 5),
        PinFunction("UART5_CTS", "UART", 7),
        PinFunction("QUADSPI_BK1_IO0", "QUADSPI", 9),
        PinFunction("SDMMC1_D1", "SDMMC", 12),
        PinFunction("DCMI_D3", "DCMI", 13),
    ],
    "PC10": [
        PinFunction("SPI3_SCK", "SPI", 6),
        PinFunction("I2S3_CK", "I2S", 6),
        PinFunction("USART3_TX", "USART", 7),
        PinFunction("UART4_TX", "UART", 8),
        PinFunction("QUADSPI_BK1_IO1", "QUADSPI", 9),
        PinFunction("SDMMC1_D2", "SDMMC", 12),
        PinFunction("DCMI_D8", "DCMI", 13),
    ],
    "PC11": [
        PinFunction("SPI3_MISO", "SPI", 6),
        PinFunction("I2S3ext_SD", "I2S", 5),
        PinFunction("USART3_RX", "USART", 7),
        PinFunction("UART4_RX", "UART", 8),
        PinFunction("QUADSPI_BK2_NCS", "QUADSPI", 9),
        PinFunction("SDMMC1_D3", "SDMMC", 12),
        PinFunction("DCMI_D4", "DCMI", 13),
    ],
    "PC12": [
        PinFunction("SPI3_MOSI", "SPI", 6),
        PinFunction("I2S3_SD", "I2S", 6),
        PinFunction("USART3_CK", "USART", 7),
        PinFunction("UART5_TX", "UART", 8),
        PinFunction("SDMMC1_CK", "SDMMC", 12),
        PinFunction("DCMI_D9", "DCMI", 13),
    ],
    "PC13": [
        PinFunction("RTC_OUT", "RTC", 0),
        PinFunction("RTC_TS", "RTC", 0),
        PinFunction("RTC_TAMP1", "RTC", 0),
    ],
    "PC14": [
        PinFunction("OSC32_IN", "RCC", 0),
    ],
    "PC15": [
        PinFunction("OSC32_OUT", "RCC", 0),
    ],

    # Port D pins
    "PD0": [
        PinFunction("CAN1_RX", "CAN", 9),
        PinFunction("FMC_D2", "FMC", 12),
        PinFunction("FMC_DA2", "FMC", 12),
    ],
    "PD1": [
        PinFunction("CAN1_TX", "CAN", 9),
        PinFunction("FMC_D3", "FMC", 12),
        PinFunction("FMC_DA3", "FMC", 12),
    ],
    "PD2": [
        PinFunction("TIM3_ETR", "TIM", 2),
        PinFunction("UART5_RX", "UART", 8),
        PinFunction("SDMMC1_CMD", "SDMMC", 12),
        PinFunction("DCMI_D11", "DCMI", 13),
    ],
    "PD3": [
        PinFunction("USART2_CTS", "USART", 7),
        PinFunction("FMC_CLK", "FMC", 12),
        PinFunction("DCMI_D5", "DCMI", 13),
    ],
    "PD4": [
        PinFunction("USART2_RTS", "USART", 7),
        PinFunction("FMC_NOE", "FMC", 12),
    ],
    "PD5": [
        PinFunction("USART2_TX", "USART", 7),
        PinFunction("FMC_NWE", "FMC", 12),
    ],
    "PD6": [
        PinFunction("SPI3_MOSI", "SPI", 5),
        PinFunction("I2S3_SD", "I2S", 5),
        PinFunction("SAI1_SD_A", "SAI", 6),
        PinFunction("USART2_RX", "USART", 7),
        PinFunction("FMC_NWAIT", "FMC", 12),
        PinFunction("DCMI_D10", "DCMI", 13),
    ],
    "PD7": [
        PinFunction("USART2_CK", "USART", 7),
        PinFunction("FMC_NE1", "FMC", 12),
    ],
    "PD8": [
        PinFunction("USART3_TX", "USART", 7),
        PinFunction("FMC_D13", "FMC", 12),
        PinFunction("FMC_DA13", "FMC", 12),
    ],
    "PD9": [
        PinFunction("USART3_RX", "USART", 7),
        PinFunction("FMC_D14", "FMC", 12),
        PinFunction("FMC_DA14", "FMC", 12),
    ],
    "PD10": [
        PinFunction("USART3_CK", "USART", 7),
        PinFunction("FMC_D15", "FMC", 12),
        PinFunction("FMC_DA15", "FMC", 12),
    ],
    "PD11": [
        PinFunction("USART3_CTS", "USART", 7),
        PinFunction("QUADSPI_BK1_IO0", "QUADSPI", 9),
        PinFunction("FMC_A16", "FMC", 12),
        PinFunction("FMC_CLE", "FMC", 12),
    ],
    "PD12": [
        PinFunction("TIM4_CH1", "TIM", 2),
        PinFunction("USART3_RTS", "USART", 7),
        PinFunction("QUADSPI_BK1_IO1", "QUADSPI", 9),
        PinFunction("FMC_A17", "FMC", 12),
        PinFunction("FMC_ALE", "FMC", 12),
    ],
    "PD13": [
        PinFunction("TIM4_CH2", "TIM", 2),
        PinFunction("QUADSPI_BK1_IO3", "QUADSPI", 9),
        PinFunction("FMC_A18", "FMC", 12),
    ],
    "PD14": [
        PinFunction("TIM4_CH3", "TIM", 2),
        PinFunction("UART8_CTS", "UART", 8),
        PinFunction("FMC_D0", "FMC", 12),
        PinFunction("FMC_DA0", "FMC", 12),
    ],
    "PD15": [
        PinFunction("TIM4_CH4", "TIM", 2),
        PinFunction("UART8_RTS", "UART", 8),
        PinFunction("FMC_D1", "FMC", 12),
        PinFunction("FMC_DA1", "FMC", 12),
    ],

    # Port E pins
    "PE0": [
        PinFunction("TIM4_ETR", "TIM", 2),
        PinFunction("UART8_RX", "UART", 8),
        PinFunction("FMC_NBL0", "FMC", 12),
        PinFunction("DCMI_D2", "DCMI", 13),
    ],
    "PE1": [
        PinFunction("UART8_TX", "UART", 8),
        PinFunction("FMC_NBL1", "FMC", 12),
        PinFunction("DCMI_D3", "DCMI", 13),
    ],
    "PE2": [
        PinFunction("TRACECLK", "TRACE", 0),
        PinFunction("SPI4_SCK", "SPI", 5),
        PinFunction("SAI1_MCLK_A", "SAI", 6),
        PinFunction("QUADSPI_BK1_IO2", "QUADSPI", 9),
        PinFunction("ETH_MII_TXD3", "ETH", 11),
        PinFunction("FMC_A23", "FMC", 12),
    ],
    "PE3": [
        PinFunction("TRACED0", "TRACE", 0),
        PinFunction("SAI1_SD_B", "SAI", 6),
        PinFunction("FMC_A19", "FMC", 12),
    ],
    "PE4": [
        PinFunction("TRACED1", "TRACE", 0),
        PinFunction("SPI4_NSS", "SPI", 5),
        PinFunction("SAI1_FS_A", "SAI", 6),
        PinFunction("FMC_A20", "FMC", 12),
        PinFunction("DCMI_D4", "DCMI", 13),
    ],
    "PE5": [
        PinFunction("TRACED2", "TRACE", 0),
        PinFunction("TIM9_CH1", "TIM", 3),
        PinFunction("SPI4_MISO", "SPI", 5),
        PinFunction("SAI1_SCK_A", "SAI", 6),
        PinFunction("FMC_A21", "FMC", 12),
        PinFunction("DCMI_D6", "DCMI", 13),
    ],
    "PE6": [
        PinFunction("TRACED3", "TRACE", 0),
        PinFunction("TIM9_CH2", "TIM", 3),
        PinFunction("SPI4_MOSI", "SPI", 5),
        PinFunction("SAI1_SD_A", "SAI", 6),
        PinFunction("FMC_A22", "FMC", 12),
        PinFunction("DCMI_D7", "DCMI", 13),
    ],
    "PE7": [
        PinFunction("TIM1_ETR", "TIM", 1),
        PinFunction("UART7_RX", "UART", 8),
        PinFunction("QUADSPI_BK2_IO0", "QUADSPI", 10),
        PinFunction("FMC_D4", "FMC", 12),
        PinFunction("FMC_DA4", "FMC", 12),
    ],
    "PE8": [
        PinFunction("TIM1_CH1N", "TIM", 1),
        PinFunction("UART7_TX", "UART", 8),
        PinFunction("QUADSPI_BK2_IO1", "QUADSPI", 10),
        PinFunction("FMC_D5", "FMC", 12),
        PinFunction("FMC_DA5", "FMC", 12),
    ],
    "PE9": [
        PinFunction("TIM1_CH1", "TIM", 1),
        PinFunction("UART7_RTS", "UART", 8),
        PinFunction("QUADSPI_BK2_IO2", "QUADSPI", 10),
        PinFunction("FMC_D6", "FMC", 12),
        PinFunction("FMC_DA6", "FMC", 12),
    ],
    "PE10": [
        PinFunction("TIM1_CH2N", "TIM", 1),
        PinFunction("UART7_CTS", "UART", 8),
        PinFunction("QUADSPI_BK2_IO3", "QUADSPI", 10),
        PinFunction("FMC_D7", "FMC", 12),
        PinFunction("FMC_DA7", "FMC", 12),
    ],
    "PE11": [
        PinFunction("TIM1_CH2", "TIM", 1),
        PinFunction("SPI4_NSS", "SPI", 5),
        PinFunction("FMC_D8", "FMC", 12),
        PinFunction("FMC_DA8", "FMC", 12),
    ],
    "PE12": [
        PinFunction("TIM1_CH3N", "TIM", 1),
        PinFunction("SPI4_SCK", "SPI", 5),
        PinFunction("FMC_D9", "FMC", 12),
        PinFunction("FMC_DA9", "FMC", 12),
    ],
    "PE13": [
        PinFunction("TIM1_CH3", "TIM", 1),
        PinFunction("SPI4_MISO", "SPI", 5),
        PinFunction("FMC_D10", "FMC", 12),
        PinFunction("FMC_DA10", "FMC", 12),
    ],
    "PE14": [
        PinFunction("TIM1_CH4", "TIM", 1),
        PinFunction("SPI4_MOSI", "SPI", 5),
        PinFunction("FMC_D11", "FMC", 12),
        PinFunction("FMC_DA11", "FMC", 12),
    ],
    "PE15": [
        PinFunction("TIM1_BKIN", "TIM", 1),
        PinFunction("FMC_D12", "FMC", 12),
        PinFunction("FMC_DA12", "FMC", 12),
    ],

    # Port F pins (for larger packages like LQFP144/LQFP176)
    "PF0": [
        PinFunction("I2C2_SDA", "I2C", 4),
        PinFunction("FMC_A0", "FMC", 12),
    ],
    "PF1": [
        PinFunction("I2C2_SCL", "I2C", 4),
        PinFunction("FMC_A1", "FMC", 12),
    ],
    "PF2": [
        PinFunction("I2C2_SMBA", "I2C", 4),
        PinFunction("FMC_A2", "FMC", 12),
    ],
    "PF3": [
        PinFunction("FMC_A3", "FMC", 12),
        PinFunction("ADC3_IN9", "ADC", 0),
    ],
    "PF4": [
        PinFunction("FMC_A4", "FMC", 12),
        PinFunction("ADC3_IN14", "ADC", 0),
    ],
    "PF5": [
        PinFunction("FMC_A5", "FMC", 12),
        PinFunction("ADC3_IN15", "ADC", 0),
    ],
    "PF6": [
        PinFunction("TIM10_CH1", "TIM", 3),
        PinFunction("SPI5_NSS", "SPI", 5),
        PinFunction("SAI1_SD_B", "SAI", 6),
        PinFunction("UART7_RX", "UART", 8),
        PinFunction("QUADSPI_BK1_IO3", "QUADSPI", 9),
        PinFunction("ADC3_IN4", "ADC", 0),
    ],
    "PF7": [
        PinFunction("TIM11_CH1", "TIM", 3),
        PinFunction("SPI5_SCK", "SPI", 5),
        PinFunction("SAI1_MCLK_B", "SAI", 6),
        PinFunction("UART7_TX", "UART", 8),
        PinFunction("QUADSPI_BK1_IO2", "QUADSPI", 9),
        PinFunction("ADC3_IN5", "ADC", 0),
    ],
    "PF8": [
        PinFunction("SPI5_MISO", "SPI", 5),
        PinFunction("SAI1_SCK_B", "SAI", 6),
        PinFunction("UART7_RTS", "UART", 8),
        PinFunction("TIM13_CH1", "TIM", 9),
        PinFunction("QUADSPI_BK1_IO0", "QUADSPI", 10),
        PinFunction("ADC3_IN6", "ADC", 0),
    ],
    "PF9": [
        PinFunction("SPI5_MOSI", "SPI", 5),
        PinFunction("SAI1_FS_B", "SAI", 6),
        PinFunction("UART7_CTS", "UART", 8),
        PinFunction("TIM14_CH1", "TIM", 9),
        PinFunction("QUADSPI_BK1_IO1", "QUADSPI", 10),
        PinFunction("ADC3_IN7", "ADC", 0),
    ],
    "PF10": [
        PinFunction("DCMI_D11", "DCMI", 13),
        PinFunction("ADC3_IN8", "ADC", 0),
    ],
    "PF11": [
        PinFunction("SPI5_MOSI", "SPI", 5),
        PinFunction("FMC_SDNRAS", "FMC", 12),
        PinFunction("DCMI_D12", "DCMI", 13),
    ],
    "PF12": [
        PinFunction("FMC_A6", "FMC", 12),
    ],
    "PF13": [
        PinFunction("FMC_A7", "FMC", 12),
    ],
    "PF14": [
        PinFunction("FMC_A8", "FMC", 12),
    ],
    "PF15": [
        PinFunction("FMC_A9", "FMC", 12),
    ],

    # Port G pins (for larger packages)
    "PG0": [
        PinFunction("FMC_A10", "FMC", 12),
    ],
    "PG1": [
        PinFunction("FMC_A11", "FMC", 12),
    ],
    "PG2": [
        PinFunction("FMC_A12", "FMC", 12),
    ],
    "PG3": [
        PinFunction("FMC_A13", "FMC", 12),
    ],
    "PG4": [
        PinFunction("FMC_A14", "FMC", 12),
        PinFunction("FMC_BA0", "FMC", 12),
    ],
    "PG5": [
        PinFunction("FMC_A15", "FMC", 12),
        PinFunction("FMC_BA1", "FMC", 12),
    ],
    "PG6": [
        PinFunction("DCMI_D12", "DCMI", 13),
    ],
    "PG7": [
        PinFunction("USART6_CK", "USART", 8),
        PinFunction("FMC_INT", "FMC", 12),
        PinFunction("DCMI_D13", "DCMI", 13),
    ],
    "PG8": [
        PinFunction("SPI6_NSS", "SPI", 5),
        PinFunction("USART6_RTS", "USART", 8),
        PinFunction("FMC_SDCLK", "FMC", 12),
    ],
    "PG9": [
        PinFunction("USART6_RX", "USART", 8),
        PinFunction("QUADSPI_BK2_IO2", "QUADSPI", 9),
        PinFunction("FMC_NE2", "FMC", 12),
        PinFunction("FMC_NCE", "FMC", 12),
        PinFunction("DCMI_VSYNC", "DCMI", 13),
    ],
    "PG10": [
        PinFunction("FMC_NE3", "FMC", 12),
        PinFunction("DCMI_D2", "DCMI", 13),
    ],
    "PG11": [
        PinFunction("ETH_MII_TX_EN", "ETH", 11),
        PinFunction("ETH_RMII_TX_EN", "ETH", 11),
        PinFunction("DCMI_D3", "DCMI", 13),
    ],
    "PG12": [
        PinFunction("USART6_RTS", "USART", 8),
        PinFunction("FMC_NE4", "FMC", 12),
    ],
    "PG13": [
        PinFunction("USART6_CTS", "USART", 8),
        PinFunction("ETH_MII_TXD0", "ETH", 11),
        PinFunction("ETH_RMII_TXD0", "ETH", 11),
        PinFunction("FMC_A24", "FMC", 12),
    ],
    "PG14": [
        PinFunction("USART6_TX", "USART", 8),
        PinFunction("ETH_MII_TXD1", "ETH", 11),
        PinFunction("ETH_RMII_TXD1", "ETH", 11),
        PinFunction("FMC_A25", "FMC", 12),
        PinFunction("QUADSPI_BK2_IO3", "QUADSPI", 9),
    ],
    "PG15": [
        PinFunction("USART6_CTS", "USART", 8),
        PinFunction("FMC_SDNCAS", "FMC", 12),
        PinFunction("DCMI_D13", "DCMI", 13),
    ],

    # Port H pins (for larger packages)
    "PH0": [
        PinFunction("OSC_IN", "RCC", 0),
    ],
    "PH1": [
        PinFunction("OSC_OUT", "RCC", 0),
    ],
}
