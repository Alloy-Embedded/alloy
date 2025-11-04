"""
STM32F4 Pin Function Database

Complete mapping of STM32F4 pins to their alternate functions.
Based on STM32F4 datasheets and reference manuals.

Key differences from STM32F1:
- Uses AF0-AF15 (Alternate Function) instead of remap levels
- More systematic alternate function assignment
- Each pin can have up to 16 alternate functions (AF0-AF15)

Common AF assignments across STM32F4 family:
- AF0: System (EVENTOUT, etc)
- AF1: TIM1/TIM2
- AF2: TIM3/TIM4/TIM5
- AF3: TIM8/TIM9/TIM10/TIM11
- AF4: I2C1/I2C2/I2C3
- AF5: SPI1/SPI2/SPI3/SPI4/SPI5/SPI6
- AF6: SPI2/SPI3/SAI1
- AF7: USART1/USART2/USART3
- AF8: USART4/USART5/USART6/UART7/UART8
- AF9: CAN1/CAN2/TIM12/TIM13/TIM14/LCD
- AF10: OTG_FS/OTG_HS
- AF11: ETH
- AF12: FMC/SDIO/OTG_HS
- AF13: DCMI
- AF14: LCD-TFT
- AF15: EVENTOUT
"""

from dataclasses import dataclass
from typing import List, Dict

@dataclass
class PinFunction:
    """Represents a pin's alternate function"""
    function_name: str      # e.g., "USART1_TX", "TIM2_CH1"
    peripheral_type: str    # e.g., "USART", "TIM", "I2C", "SPI"
    af_number: int         # Alternate Function number (0-15)
    description: str = ""   # Optional description

# STM32F4 Pin Function Database
# Format: "PIN": [PinFunction(...), ...]
STM32F4_PIN_FUNCTIONS = {
    # PORT A
    "PA0": [
        PinFunction("TIM2_CH1", "TIM", 1),
        PinFunction("TIM2_ETR", "TIM", 1),
        PinFunction("TIM5_CH1", "TIM", 2),
        PinFunction("TIM8_ETR", "TIM", 3),
        PinFunction("USART2_CTS", "USART", 7),
        PinFunction("UART4_TX", "UART", 8),
        PinFunction("ETH_MII_CRS", "ETH", 11),
        PinFunction("ADC123_IN0", "ADC", 0, "ADC1/2/3 Channel 0"),
        PinFunction("WKUP", "SYSTEM", 0, "Wakeup pin"),
    ],

    "PA1": [
        PinFunction("TIM2_CH2", "TIM", 1),
        PinFunction("TIM5_CH2", "TIM", 2),
        PinFunction("USART2_RTS", "USART", 7),
        PinFunction("UART4_RX", "UART", 8),
        PinFunction("ETH_MII_RX_CLK", "ETH", 11),
        PinFunction("ETH_RMII_REF_CLK", "ETH", 11),
        PinFunction("ADC123_IN1", "ADC", 0, "ADC1/2/3 Channel 1"),
    ],

    "PA2": [
        PinFunction("TIM2_CH3", "TIM", 1),
        PinFunction("TIM5_CH3", "TIM", 2),
        PinFunction("TIM9_CH1", "TIM", 3),
        PinFunction("USART2_TX", "USART", 7),
        PinFunction("ETH_MDIO", "ETH", 11),
        PinFunction("ADC123_IN2", "ADC", 0, "ADC1/2/3 Channel 2"),
    ],

    "PA3": [
        PinFunction("TIM2_CH4", "TIM", 1),
        PinFunction("TIM5_CH4", "TIM", 2),
        PinFunction("TIM9_CH2", "TIM", 3),
        PinFunction("USART2_RX", "USART", 7),
        PinFunction("OTG_HS_ULPI_D0", "USB", 10),
        PinFunction("ETH_MII_COL", "ETH", 11),
        PinFunction("ADC123_IN3", "ADC", 0, "ADC1/2/3 Channel 3"),
    ],

    "PA4": [
        PinFunction("SPI1_NSS", "SPI", 5),
        PinFunction("SPI3_NSS", "SPI", 6),
        PinFunction("USART2_CK", "USART", 7),
        PinFunction("I2S3_WS", "I2S", 6),
        PinFunction("OTG_HS_SOF", "USB", 12),
        PinFunction("DCMI_HSYNC", "DCMI", 13),
        PinFunction("ADC12_IN4", "ADC", 0, "ADC1/2 Channel 4"),
        PinFunction("DAC_OUT1", "DAC", 0, "DAC Channel 1"),
    ],

    "PA5": [
        PinFunction("TIM2_CH1", "TIM", 1),
        PinFunction("TIM2_ETR", "TIM", 1),
        PinFunction("TIM8_CH1N", "TIM", 3),
        PinFunction("SPI1_SCK", "SPI", 5),
        PinFunction("OTG_HS_ULPI_CK", "USB", 10),
        PinFunction("ADC12_IN5", "ADC", 0, "ADC1/2 Channel 5"),
        PinFunction("DAC_OUT2", "DAC", 0, "DAC Channel 2"),
    ],

    "PA6": [
        PinFunction("TIM1_BKIN", "TIM", 1),
        PinFunction("TIM3_CH1", "TIM", 2),
        PinFunction("TIM8_BKIN", "TIM", 3),
        PinFunction("SPI1_MISO", "SPI", 5),
        PinFunction("TIM13_CH1", "TIM", 9),
        PinFunction("DCMI_PIXCLK", "DCMI", 13),
        PinFunction("ADC12_IN6", "ADC", 0, "ADC1/2 Channel 6"),
    ],

    "PA7": [
        PinFunction("TIM1_CH1N", "TIM", 1),
        PinFunction("TIM3_CH2", "TIM", 2),
        PinFunction("TIM8_CH1N", "TIM", 3),
        PinFunction("SPI1_MOSI", "SPI", 5),
        PinFunction("TIM14_CH1", "TIM", 9),
        PinFunction("ETH_MII_RX_DV", "ETH", 11),
        PinFunction("ETH_RMII_CRS_DV", "ETH", 11),
        PinFunction("ADC12_IN7", "ADC", 0, "ADC1/2 Channel 7"),
    ],

    "PA8": [
        PinFunction("MCO1", "SYSTEM", 0, "Microcontroller Clock Output 1"),
        PinFunction("TIM1_CH1", "TIM", 1),
        PinFunction("I2C3_SCL", "I2C", 4),
        PinFunction("USART1_CK", "USART", 7),
        PinFunction("OTG_FS_SOF", "USB", 10),
    ],

    "PA9": [
        PinFunction("TIM1_CH2", "TIM", 1),
        PinFunction("I2C3_SMBA", "I2C", 4),
        PinFunction("USART1_TX", "USART", 7),
        PinFunction("DCMI_D0", "DCMI", 13),
        PinFunction("OTG_FS_VBUS", "USB", 0),
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
        PinFunction("JTMS", "DEBUG", 0, "JTAG Test Mode State"),
        PinFunction("SWDIO", "DEBUG", 0, "Serial Wire Debug I/O"),
    ],

    "PA14": [
        PinFunction("JTCK", "DEBUG", 0, "JTAG Clock"),
        PinFunction("SWCLK", "DEBUG", 0, "Serial Wire Clock"),
    ],

    "PA15": [
        PinFunction("JTDI", "DEBUG", 0, "JTAG Data In"),
        PinFunction("TIM2_CH1", "TIM", 1),
        PinFunction("TIM2_ETR", "TIM", 1),
        PinFunction("SPI1_NSS", "SPI", 5),
        PinFunction("SPI3_NSS", "SPI", 6),
    ],

    # PORT B
    "PB0": [
        PinFunction("TIM1_CH2N", "TIM", 1),
        PinFunction("TIM3_CH3", "TIM", 2),
        PinFunction("TIM8_CH2N", "TIM", 3),
        PinFunction("OTG_HS_ULPI_D1", "USB", 10),
        PinFunction("ETH_MII_RXD2", "ETH", 11),
        PinFunction("ADC12_IN8", "ADC", 0, "ADC1/2 Channel 8"),
    ],

    "PB1": [
        PinFunction("TIM1_CH3N", "TIM", 1),
        PinFunction("TIM3_CH4", "TIM", 2),
        PinFunction("TIM8_CH3N", "TIM", 3),
        PinFunction("OTG_HS_ULPI_D2", "USB", 10),
        PinFunction("ETH_MII_RXD3", "ETH", 11),
        PinFunction("ADC12_IN9", "ADC", 0, "ADC1/2 Channel 9"),
    ],

    "PB2": [
        PinFunction("BOOT1", "SYSTEM", 0),
    ],

    "PB3": [
        PinFunction("JTDO", "DEBUG", 0, "JTAG Data Out"),
        PinFunction("TRACESWO", "DEBUG", 0, "Trace SWO"),
        PinFunction("TIM2_CH2", "TIM", 1),
        PinFunction("SPI1_SCK", "SPI", 5),
        PinFunction("SPI3_SCK", "SPI", 6),
        PinFunction("I2S3_CK", "I2S", 6),
    ],

    "PB4": [
        PinFunction("NJTRST", "DEBUG", 0, "JTAG Reset"),
        PinFunction("TIM3_CH1", "TIM", 2),
        PinFunction("SPI1_MISO", "SPI", 5),
        PinFunction("SPI3_MISO", "SPI", 6),
        PinFunction("I2S3EXT_SD", "I2S", 7),
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
        PinFunction("ETH_MII_TXD3", "ETH", 11),
        PinFunction("SDIO_D4", "SDIO", 12),
        PinFunction("DCMI_D6", "DCMI", 13),
    ],

    "PB9": [
        PinFunction("TIM4_CH4", "TIM", 2),
        PinFunction("TIM11_CH1", "TIM", 3),
        PinFunction("I2C1_SDA", "I2C", 4),
        PinFunction("SPI2_NSS", "SPI", 5),
        PinFunction("I2S2_WS", "I2S", 5),
        PinFunction("CAN1_TX", "CAN", 9),
        PinFunction("SDIO_D5", "SDIO", 12),
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
        PinFunction("I2S2EXT_SD", "I2S", 6),
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

    # PORT C
    "PC0": [
        PinFunction("OTG_HS_ULPI_STP", "USB", 10),
        PinFunction("ADC123_IN10", "ADC", 0, "ADC1/2/3 Channel 10"),
    ],

    "PC1": [
        PinFunction("ETH_MDC", "ETH", 11),
        PinFunction("ADC123_IN11", "ADC", 0, "ADC1/2/3 Channel 11"),
    ],

    "PC2": [
        PinFunction("SPI2_MISO", "SPI", 5),
        PinFunction("I2S2EXT_SD", "I2S", 6),
        PinFunction("OTG_HS_ULPI_DIR", "USB", 10),
        PinFunction("ETH_MII_TXD2", "ETH", 11),
        PinFunction("ADC123_IN12", "ADC", 0, "ADC1/2/3 Channel 12"),
    ],

    "PC3": [
        PinFunction("SPI2_MOSI", "SPI", 5),
        PinFunction("I2S2_SD", "I2S", 5),
        PinFunction("OTG_HS_ULPI_NXT", "USB", 10),
        PinFunction("ETH_MII_TX_CLK", "ETH", 11),
        PinFunction("ADC123_IN13", "ADC", 0, "ADC1/2/3 Channel 13"),
    ],

    "PC4": [
        PinFunction("ETH_MII_RXD0", "ETH", 11),
        PinFunction("ETH_RMII_RXD0", "ETH", 11),
        PinFunction("ADC12_IN14", "ADC", 0, "ADC1/2 Channel 14"),
    ],

    "PC5": [
        PinFunction("ETH_MII_RXD1", "ETH", 11),
        PinFunction("ETH_RMII_RXD1", "ETH", 11),
        PinFunction("ADC12_IN15", "ADC", 0, "ADC1/2 Channel 15"),
    ],

    "PC6": [
        PinFunction("TIM3_CH1", "TIM", 2),
        PinFunction("TIM8_CH1", "TIM", 3),
        PinFunction("I2S2_MCK", "I2S", 5),
        PinFunction("USART6_TX", "USART", 8),
        PinFunction("SDIO_D6", "SDIO", 12),
        PinFunction("DCMI_D0", "DCMI", 13),
    ],

    "PC7": [
        PinFunction("TIM3_CH2", "TIM", 2),
        PinFunction("TIM8_CH2", "TIM", 3),
        PinFunction("I2S3_MCK", "I2S", 6),
        PinFunction("USART6_RX", "USART", 8),
        PinFunction("SDIO_D7", "SDIO", 12),
        PinFunction("DCMI_D1", "DCMI", 13),
    ],

    "PC8": [
        PinFunction("TIM3_CH3", "TIM", 2),
        PinFunction("TIM8_CH3", "TIM", 3),
        PinFunction("USART6_CK", "USART", 8),
        PinFunction("SDIO_D0", "SDIO", 12),
        PinFunction("DCMI_D2", "DCMI", 13),
    ],

    "PC9": [
        PinFunction("MCO2", "SYSTEM", 0, "Microcontroller Clock Output 2"),
        PinFunction("TIM3_CH4", "TIM", 2),
        PinFunction("TIM8_CH4", "TIM", 3),
        PinFunction("I2C3_SDA", "I2C", 4),
        PinFunction("I2S_CKIN", "I2S", 5),
        PinFunction("SDIO_D1", "SDIO", 12),
        PinFunction("DCMI_D3", "DCMI", 13),
    ],

    "PC10": [
        PinFunction("SPI3_SCK", "SPI", 6),
        PinFunction("I2S3_CK", "I2S", 6),
        PinFunction("USART3_TX", "USART", 7),
        PinFunction("UART4_TX", "UART", 8),
        PinFunction("SDIO_D2", "SDIO", 12),
        PinFunction("DCMI_D8", "DCMI", 13),
    ],

    "PC11": [
        PinFunction("SPI3_MISO", "SPI", 6),
        PinFunction("I2S3EXT_SD", "I2S", 5),
        PinFunction("USART3_RX", "USART", 7),
        PinFunction("UART4_RX", "UART", 8),
        PinFunction("SDIO_D3", "SDIO", 12),
        PinFunction("DCMI_D4", "DCMI", 13),
    ],

    "PC12": [
        PinFunction("SPI3_MOSI", "SPI", 6),
        PinFunction("I2S3_SD", "I2S", 6),
        PinFunction("USART3_CK", "USART", 7),
        PinFunction("UART5_TX", "UART", 8),
        PinFunction("SDIO_CK", "SDIO", 12),
        PinFunction("DCMI_D9", "DCMI", 13),
    ],

    "PC13": [
        PinFunction("RTC_AF1", "RTC", 0),
    ],

    "PC14": [
        PinFunction("OSC32_IN", "RTC", 0),
    ],

    "PC15": [
        PinFunction("OSC32_OUT", "RTC", 0),
    ],

    # PORT D
    "PD0": [
        PinFunction("CAN1_RX", "CAN", 9),
        PinFunction("FMC_D2", "FMC", 12),
    ],

    "PD1": [
        PinFunction("CAN1_TX", "CAN", 9),
        PinFunction("FMC_D3", "FMC", 12),
    ],

    "PD2": [
        PinFunction("TIM3_ETR", "TIM", 2),
        PinFunction("UART5_RX", "UART", 8),
        PinFunction("SDIO_CMD", "SDIO", 12),
        PinFunction("DCMI_D11", "DCMI", 13),
    ],

    "PD3": [
        PinFunction("SPI2_SCK", "SPI", 5),
        PinFunction("I2S2_CK", "I2S", 5),
        PinFunction("USART2_CTS", "USART", 7),
        PinFunction("FMC_CLK", "FMC", 12),
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
    ],

    "PD9": [
        PinFunction("USART3_RX", "USART", 7),
        PinFunction("FMC_D14", "FMC", 12),
    ],

    "PD10": [
        PinFunction("USART3_CK", "USART", 7),
        PinFunction("FMC_D15", "FMC", 12),
    ],

    "PD11": [
        PinFunction("USART3_CTS", "USART", 7),
        PinFunction("FMC_A16", "FMC", 12),
    ],

    "PD12": [
        PinFunction("TIM4_CH1", "TIM", 2),
        PinFunction("USART3_RTS", "USART", 7),
        PinFunction("FMC_A17", "FMC", 12),
    ],

    "PD13": [
        PinFunction("TIM4_CH2", "TIM", 2),
        PinFunction("FMC_A18", "FMC", 12),
    ],

    "PD14": [
        PinFunction("TIM4_CH3", "TIM", 2),
        PinFunction("FMC_D0", "FMC", 12),
    ],

    "PD15": [
        PinFunction("TIM4_CH4", "TIM", 2),
        PinFunction("FMC_D1", "FMC", 12),
    ],

    # PORT E
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
        PinFunction("TRACECLK", "DEBUG", 0),
        PinFunction("SPI4_SCK", "SPI", 5),
        PinFunction("ETH_MII_TXD3", "ETH", 11),
        PinFunction("FMC_A23", "FMC", 12),
    ],

    "PE3": [
        PinFunction("TRACED0", "DEBUG", 0),
        PinFunction("FMC_A19", "FMC", 12),
    ],

    "PE4": [
        PinFunction("TRACED1", "DEBUG", 0),
        PinFunction("SPI4_NSS", "SPI", 5),
        PinFunction("FMC_A20", "FMC", 12),
        PinFunction("DCMI_D4", "DCMI", 13),
    ],

    "PE5": [
        PinFunction("TRACED2", "DEBUG", 0),
        PinFunction("TIM9_CH1", "TIM", 3),
        PinFunction("SPI4_MISO", "SPI", 5),
        PinFunction("FMC_A21", "FMC", 12),
        PinFunction("DCMI_D6", "DCMI", 13),
    ],

    "PE6": [
        PinFunction("TRACED3", "DEBUG", 0),
        PinFunction("TIM9_CH2", "TIM", 3),
        PinFunction("SPI4_MOSI", "SPI", 5),
        PinFunction("FMC_A22", "FMC", 12),
        PinFunction("DCMI_D7", "DCMI", 13),
    ],

    "PE7": [
        PinFunction("TIM1_ETR", "TIM", 1),
        PinFunction("UART7_RX", "UART", 8),
        PinFunction("FMC_D4", "FMC", 12),
    ],

    "PE8": [
        PinFunction("TIM1_CH1N", "TIM", 1),
        PinFunction("UART7_TX", "UART", 8),
        PinFunction("FMC_D5", "FMC", 12),
    ],

    "PE9": [
        PinFunction("TIM1_CH1", "TIM", 1),
        PinFunction("FMC_D6", "FMC", 12),
    ],

    "PE10": [
        PinFunction("TIM1_CH2N", "TIM", 1),
        PinFunction("FMC_D7", "FMC", 12),
    ],

    "PE11": [
        PinFunction("TIM1_CH2", "TIM", 1),
        PinFunction("SPI4_NSS", "SPI", 5),
        PinFunction("FMC_D8", "FMC", 12),
    ],

    "PE12": [
        PinFunction("TIM1_CH3N", "TIM", 1),
        PinFunction("SPI4_SCK", "SPI", 5),
        PinFunction("FMC_D9", "FMC", 12),
    ],

    "PE13": [
        PinFunction("TIM1_CH3", "TIM", 1),
        PinFunction("SPI4_MISO", "SPI", 5),
        PinFunction("FMC_D10", "FMC", 12),
    ],

    "PE14": [
        PinFunction("TIM1_CH4", "TIM", 1),
        PinFunction("SPI4_MOSI", "SPI", 5),
        PinFunction("FMC_D11", "FMC", 12),
    ],

    "PE15": [
        PinFunction("TIM1_BKIN", "TIM", 1),
        PinFunction("FMC_D12", "FMC", 12),
    ],

    # Additional ports F, G, H, I for larger packages can be added here
}

def get_pin_functions(pin: str) -> List[PinFunction]:
    """Get all alternate functions for a pin"""
    return STM32F4_PIN_FUNCTIONS.get(pin, [])

def get_functions_by_peripheral(peripheral_type: str) -> Dict[str, List[PinFunction]]:
    """Get all pins for a specific peripheral type"""
    result = {}
    for pin, functions in STM32F4_PIN_FUNCTIONS.items():
        matching = [f for f in functions if f.peripheral_type == peripheral_type]
        if matching:
            result[pin] = matching
    return result
