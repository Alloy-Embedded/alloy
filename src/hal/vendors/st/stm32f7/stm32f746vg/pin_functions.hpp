#pragma once

#include <cstdint>
#include "pins.hpp"

namespace alloy::hal::stm32f7::stm32f746vg::pin_functions {

// ============================================================================
// Pin Alternate Functions for STM32F746VG
// Auto-generated from STM32F7 datasheet
// ============================================================================


// ADC Functions
namespace adc {

    constexpr uint8_t ADC123_IN0                     = pins::PA0;
    constexpr uint8_t ADC123_IN1                     = pins::PA1;
    constexpr uint8_t ADC123_IN10                    = pins::PC0;
    constexpr uint8_t ADC123_IN11                    = pins::PC1;
    constexpr uint8_t ADC123_IN12                    = pins::PC2;
    constexpr uint8_t ADC123_IN13                    = pins::PC3;
    constexpr uint8_t ADC123_IN2                     = pins::PA2;
    constexpr uint8_t ADC123_IN3                     = pins::PA3;
    constexpr uint8_t ADC12_IN14                     = pins::PC4;
    constexpr uint8_t ADC12_IN15                     = pins::PC5;
    constexpr uint8_t ADC12_IN4                      = pins::PA4;
    constexpr uint8_t ADC12_IN5                      = pins::PA5;
    constexpr uint8_t ADC12_IN6                      = pins::PA6;
    constexpr uint8_t ADC12_IN7                      = pins::PA7;
    constexpr uint8_t ADC12_IN8                      = pins::PB0;
    constexpr uint8_t ADC12_IN9                      = pins::PB1;

}  // namespace adc

// CAN Functions
namespace can {

    // CAN1_RX available on: PA11, PB8, PD0
    constexpr uint8_t CAN1_RX                           = pins::PA11;
    constexpr uint8_t CAN1_RX_REMAP1                    = pins::PB8;
    constexpr uint8_t CAN1_RX_REMAP2                    = pins::PD0;
    // CAN1_TX available on: PA12, PB9, PD1
    constexpr uint8_t CAN1_TX                           = pins::PA12;
    constexpr uint8_t CAN1_TX_REMAP1                    = pins::PB9;
    constexpr uint8_t CAN1_TX_REMAP2                    = pins::PD1;
    // CAN2_RX available on: PB12, PB5
    constexpr uint8_t CAN2_RX                           = pins::PB12;
    constexpr uint8_t CAN2_RX_REMAP1                    = pins::PB5;
    // CAN2_TX available on: PB13, PB6
    constexpr uint8_t CAN2_TX                           = pins::PB13;
    constexpr uint8_t CAN2_TX_REMAP1                    = pins::PB6;

}  // namespace can

// DAC Functions
namespace dac {

    constexpr uint8_t DAC_OUT1                       = pins::PA4;
    constexpr uint8_t DAC_OUT2                       = pins::PA5;

}  // namespace dac

// DCMI Functions
namespace dcmi {

    // DCMI_D0 available on: PA9, PC6
    constexpr uint8_t DCMI_D0                           = pins::PA9;
    constexpr uint8_t DCMI_D0_REMAP1                    = pins::PC6;
    // DCMI_D1 available on: PA10, PC7
    constexpr uint8_t DCMI_D1                           = pins::PA10;
    constexpr uint8_t DCMI_D1_REMAP1                    = pins::PC7;
    // DCMI_D10 available on: PB5, PD6
    constexpr uint8_t DCMI_D10                           = pins::PB5;
    constexpr uint8_t DCMI_D10_REMAP1                    = pins::PD6;
    constexpr uint8_t DCMI_D11                       = pins::PD2;
    // DCMI_D2 available on: PC8, PE0
    constexpr uint8_t DCMI_D2                           = pins::PC8;
    constexpr uint8_t DCMI_D2_REMAP1                    = pins::PE0;
    // DCMI_D3 available on: PC9, PE1
    constexpr uint8_t DCMI_D3                           = pins::PC9;
    constexpr uint8_t DCMI_D3_REMAP1                    = pins::PE1;
    // DCMI_D4 available on: PC11, PE4
    constexpr uint8_t DCMI_D4                           = pins::PC11;
    constexpr uint8_t DCMI_D4_REMAP1                    = pins::PE4;
    // DCMI_D5 available on: PB6, PD3
    constexpr uint8_t DCMI_D5                           = pins::PB6;
    constexpr uint8_t DCMI_D5_REMAP1                    = pins::PD3;
    // DCMI_D6 available on: PB8, PE5
    constexpr uint8_t DCMI_D6                           = pins::PB8;
    constexpr uint8_t DCMI_D6_REMAP1                    = pins::PE5;
    // DCMI_D7 available on: PB9, PE6
    constexpr uint8_t DCMI_D7                           = pins::PB9;
    constexpr uint8_t DCMI_D7_REMAP1                    = pins::PE6;
    constexpr uint8_t DCMI_D8                        = pins::PC10;
    constexpr uint8_t DCMI_D9                        = pins::PC12;
    constexpr uint8_t DCMI_HSYNC                     = pins::PA4;
    constexpr uint8_t DCMI_PIXCLK                    = pins::PA6;
    constexpr uint8_t DCMI_VSYNC                     = pins::PB7;

}  // namespace dcmi

// ETH Functions
namespace eth {

    constexpr uint8_t ETH_MDC                        = pins::PC1;
    constexpr uint8_t ETH_MDIO                       = pins::PA2;
    constexpr uint8_t ETH_MII_COL                    = pins::PA3;
    constexpr uint8_t ETH_MII_CRS                    = pins::PA0;
    constexpr uint8_t ETH_MII_RXD0                   = pins::PC4;
    constexpr uint8_t ETH_MII_RXD1                   = pins::PC5;
    constexpr uint8_t ETH_MII_RXD2                   = pins::PB0;
    constexpr uint8_t ETH_MII_RXD3                   = pins::PB1;
    constexpr uint8_t ETH_MII_RX_CLK                 = pins::PA1;
    constexpr uint8_t ETH_MII_RX_DV                  = pins::PA7;
    constexpr uint8_t ETH_MII_RX_ER                  = pins::PB10;
    constexpr uint8_t ETH_MII_TXD0                   = pins::PB12;
    constexpr uint8_t ETH_MII_TXD1                   = pins::PB13;
    constexpr uint8_t ETH_MII_TXD2                   = pins::PC2;
    constexpr uint8_t ETH_MII_TXD3                   = pins::PE2;
    constexpr uint8_t ETH_MII_TX_CLK                 = pins::PC3;
    constexpr uint8_t ETH_MII_TX_EN                  = pins::PB11;
    constexpr uint8_t ETH_PPS_OUT                    = pins::PB5;
    constexpr uint8_t ETH_RMII_CRS_DV                = pins::PA7;
    constexpr uint8_t ETH_RMII_REF_CLK               = pins::PA1;
    constexpr uint8_t ETH_RMII_RXD0                  = pins::PC4;
    constexpr uint8_t ETH_RMII_RXD1                  = pins::PC5;
    constexpr uint8_t ETH_RMII_TXD0                  = pins::PB12;
    constexpr uint8_t ETH_RMII_TXD1                  = pins::PB13;
    constexpr uint8_t ETH_RMII_TX_EN                 = pins::PB11;

}  // namespace eth

// FMC Functions
namespace fmc {

    constexpr uint8_t FMC_A16                        = pins::PD11;
    constexpr uint8_t FMC_A17                        = pins::PD12;
    constexpr uint8_t FMC_A18                        = pins::PD13;
    constexpr uint8_t FMC_A19                        = pins::PE3;
    constexpr uint8_t FMC_A20                        = pins::PE4;
    constexpr uint8_t FMC_A21                        = pins::PE5;
    constexpr uint8_t FMC_A22                        = pins::PE6;
    constexpr uint8_t FMC_A23                        = pins::PE2;
    constexpr uint8_t FMC_ALE                        = pins::PD12;
    constexpr uint8_t FMC_CLE                        = pins::PD11;
    constexpr uint8_t FMC_CLK                        = pins::PD3;
    constexpr uint8_t FMC_D0                         = pins::PD14;
    constexpr uint8_t FMC_D1                         = pins::PD15;
    constexpr uint8_t FMC_D10                        = pins::PE13;
    constexpr uint8_t FMC_D11                        = pins::PE14;
    constexpr uint8_t FMC_D12                        = pins::PE15;
    constexpr uint8_t FMC_D13                        = pins::PD8;
    constexpr uint8_t FMC_D14                        = pins::PD9;
    constexpr uint8_t FMC_D15                        = pins::PD10;
    constexpr uint8_t FMC_D2                         = pins::PD0;
    constexpr uint8_t FMC_D3                         = pins::PD1;
    constexpr uint8_t FMC_D4                         = pins::PE7;
    constexpr uint8_t FMC_D5                         = pins::PE8;
    constexpr uint8_t FMC_D6                         = pins::PE9;
    constexpr uint8_t FMC_D7                         = pins::PE10;
    constexpr uint8_t FMC_D8                         = pins::PE11;
    constexpr uint8_t FMC_D9                         = pins::PE12;
    constexpr uint8_t FMC_DA0                        = pins::PD14;
    constexpr uint8_t FMC_DA1                        = pins::PD15;
    constexpr uint8_t FMC_DA10                       = pins::PE13;
    constexpr uint8_t FMC_DA11                       = pins::PE14;
    constexpr uint8_t FMC_DA12                       = pins::PE15;
    constexpr uint8_t FMC_DA13                       = pins::PD8;
    constexpr uint8_t FMC_DA14                       = pins::PD9;
    constexpr uint8_t FMC_DA15                       = pins::PD10;
    constexpr uint8_t FMC_DA2                        = pins::PD0;
    constexpr uint8_t FMC_DA3                        = pins::PD1;
    constexpr uint8_t FMC_DA4                        = pins::PE7;
    constexpr uint8_t FMC_DA5                        = pins::PE8;
    constexpr uint8_t FMC_DA6                        = pins::PE9;
    constexpr uint8_t FMC_DA7                        = pins::PE10;
    constexpr uint8_t FMC_DA8                        = pins::PE11;
    constexpr uint8_t FMC_DA9                        = pins::PE12;
    constexpr uint8_t FMC_NBL0                       = pins::PE0;
    constexpr uint8_t FMC_NBL1                       = pins::PE1;
    constexpr uint8_t FMC_NE1                        = pins::PD7;
    constexpr uint8_t FMC_NOE                        = pins::PD4;
    constexpr uint8_t FMC_NWAIT                      = pins::PD6;
    constexpr uint8_t FMC_NWE                        = pins::PD5;
    constexpr uint8_t FMC_SDCKE0                     = pins::PC3;
    constexpr uint8_t FMC_SDNE0                      = pins::PC2;
    constexpr uint8_t FMC_SDNWE                      = pins::PC0;

}  // namespace fmc

// I2C Functions
namespace i2c {

    // I2C1_SCL available on: PB6, PB8
    constexpr uint8_t I2C1_SCL                           = pins::PB6;
    constexpr uint8_t I2C1_SCL_REMAP1                    = pins::PB8;
    // I2C1_SDA available on: PB7, PB9
    constexpr uint8_t I2C1_SDA                           = pins::PB7;
    constexpr uint8_t I2C1_SDA_REMAP1                    = pins::PB9;
    constexpr uint8_t I2C1_SMBA                      = pins::PB5;
    constexpr uint8_t I2C2_SCL                       = pins::PB10;
    constexpr uint8_t I2C2_SDA                       = pins::PB11;
    constexpr uint8_t I2C2_SMBA                      = pins::PB12;
    constexpr uint8_t I2C3_SCL                       = pins::PA8;
    constexpr uint8_t I2C3_SDA                       = pins::PC9;
    constexpr uint8_t I2C3_SMBA                      = pins::PA9;

}  // namespace i2c

// I2S Functions
namespace i2s {

    // I2S2_CK available on: PB10, PB13
    constexpr uint8_t I2S2_CK                           = pins::PB10;
    constexpr uint8_t I2S2_CK_REMAP1                    = pins::PB13;
    constexpr uint8_t I2S2_MCK                       = pins::PC6;
    // I2S2_SD available on: PB15, PC3
    constexpr uint8_t I2S2_SD                           = pins::PB15;
    constexpr uint8_t I2S2_SD_REMAP1                    = pins::PC3;
    // I2S2_WS available on: PB12, PB9
    constexpr uint8_t I2S2_WS                           = pins::PB12;
    constexpr uint8_t I2S2_WS_REMAP1                    = pins::PB9;
    // I2S2ext_SD available on: PB14, PC2
    constexpr uint8_t I2S2ext_SD                           = pins::PB14;
    constexpr uint8_t I2S2ext_SD_REMAP1                    = pins::PC2;
    // I2S3_CK available on: PB3, PC10
    constexpr uint8_t I2S3_CK                           = pins::PB3;
    constexpr uint8_t I2S3_CK_REMAP1                    = pins::PC10;
    constexpr uint8_t I2S3_MCK                       = pins::PC7;
    // I2S3_SD available on: PB5, PC12, PD6
    constexpr uint8_t I2S3_SD                           = pins::PB5;
    constexpr uint8_t I2S3_SD_REMAP1                    = pins::PC12;
    constexpr uint8_t I2S3_SD_REMAP2                    = pins::PD6;
    // I2S3_WS available on: PA15, PA4
    constexpr uint8_t I2S3_WS                           = pins::PA15;
    constexpr uint8_t I2S3_WS_REMAP1                    = pins::PA4;
    // I2S3ext_SD available on: PB4, PC11
    constexpr uint8_t I2S3ext_SD                           = pins::PB4;
    constexpr uint8_t I2S3ext_SD_REMAP1                    = pins::PC11;
    constexpr uint8_t I2S_CKIN                       = pins::PC9;

}  // namespace i2s

// JTAG Functions
namespace jtag {

    constexpr uint8_t JTCK                           = pins::PA14;
    constexpr uint8_t JTDI                           = pins::PA15;
    constexpr uint8_t JTDO                           = pins::PB3;
    constexpr uint8_t JTMS                           = pins::PA13;
    constexpr uint8_t NJTRST                         = pins::PB4;

}  // namespace jtag

// QUADSPI Functions
namespace quadspi {

    // QUADSPI_BK1_IO0 available on: PC9, PD11
    constexpr uint8_t QUADSPI_BK1_IO0                           = pins::PC9;
    constexpr uint8_t QUADSPI_BK1_IO0_REMAP1                    = pins::PD11;
    // QUADSPI_BK1_IO1 available on: PC10, PD12
    constexpr uint8_t QUADSPI_BK1_IO1                           = pins::PC10;
    constexpr uint8_t QUADSPI_BK1_IO1_REMAP1                    = pins::PD12;
    constexpr uint8_t QUADSPI_BK1_IO2                = pins::PE2;
    // QUADSPI_BK1_IO3 available on: PA1, PD13
    constexpr uint8_t QUADSPI_BK1_IO3                           = pins::PA1;
    constexpr uint8_t QUADSPI_BK1_IO3_REMAP1                    = pins::PD13;
    constexpr uint8_t QUADSPI_BK1_NCS                = pins::PB6;
    constexpr uint8_t QUADSPI_BK2_IO0                = pins::PE7;
    constexpr uint8_t QUADSPI_BK2_IO1                = pins::PE8;
    constexpr uint8_t QUADSPI_BK2_IO2                = pins::PE9;
    constexpr uint8_t QUADSPI_BK2_IO3                = pins::PE10;
    constexpr uint8_t QUADSPI_BK2_NCS                = pins::PC11;
    constexpr uint8_t QUADSPI_CLK                    = pins::PB2;

}  // namespace quadspi

// RCC Functions
namespace rcc {

    constexpr uint8_t MCO1                           = pins::PA8;
    constexpr uint8_t MCO2                           = pins::PC9;
    constexpr uint8_t OSC32_IN                       = pins::PC14;
    constexpr uint8_t OSC32_OUT                      = pins::PC15;

}  // namespace rcc

// RTC Functions
namespace rtc {

    constexpr uint8_t RTC_OUT                        = pins::PC13;
    constexpr uint8_t RTC_REFIN                      = pins::PB15;
    constexpr uint8_t RTC_TAMP1                      = pins::PC13;
    constexpr uint8_t RTC_TS                         = pins::PC13;

}  // namespace rtc

// SAI Functions
namespace sai {

    constexpr uint8_t SAI1_FS_A                      = pins::PE4;
    constexpr uint8_t SAI1_MCLK_A                    = pins::PE2;
    constexpr uint8_t SAI1_SCK_A                     = pins::PE5;
    // SAI1_SD_A available on: PD6, PE6
    constexpr uint8_t SAI1_SD_A                           = pins::PD6;
    constexpr uint8_t SAI1_SD_A_REMAP1                    = pins::PE6;
    constexpr uint8_t SAI1_SD_B                      = pins::PE3;
    constexpr uint8_t SAI2_MCLK_B                    = pins::PA1;
    constexpr uint8_t SAI2_SCK_B                     = pins::PA2;
    constexpr uint8_t SAI2_SD_B                      = pins::PA0;

}  // namespace sai

// SDMMC Functions
namespace sdmmc {

    constexpr uint8_t SDMMC1_CK                      = pins::PC12;
    constexpr uint8_t SDMMC1_CMD                     = pins::PD2;
    constexpr uint8_t SDMMC1_D0                      = pins::PC8;
    constexpr uint8_t SDMMC1_D1                      = pins::PC9;
    constexpr uint8_t SDMMC1_D2                      = pins::PC10;
    constexpr uint8_t SDMMC1_D3                      = pins::PC11;
    constexpr uint8_t SDMMC1_D4                      = pins::PB8;
    constexpr uint8_t SDMMC1_D5                      = pins::PB9;
    constexpr uint8_t SDMMC1_D6                      = pins::PC6;
    constexpr uint8_t SDMMC1_D7                      = pins::PC7;

}  // namespace sdmmc

// SPI Functions
namespace spi {

    // SPI1_MISO available on: PA6, PB4
    constexpr uint8_t SPI1_MISO                           = pins::PA6;
    constexpr uint8_t SPI1_MISO_REMAP1                    = pins::PB4;
    // SPI1_MOSI available on: PA7, PB5
    constexpr uint8_t SPI1_MOSI                           = pins::PA7;
    constexpr uint8_t SPI1_MOSI_REMAP1                    = pins::PB5;
    // SPI1_NSS available on: PA15, PA4
    constexpr uint8_t SPI1_NSS                           = pins::PA15;
    constexpr uint8_t SPI1_NSS_REMAP1                    = pins::PA4;
    // SPI1_SCK available on: PA5, PB3
    constexpr uint8_t SPI1_SCK                           = pins::PA5;
    constexpr uint8_t SPI1_SCK_REMAP1                    = pins::PB3;
    // SPI2_MISO available on: PB14, PC2
    constexpr uint8_t SPI2_MISO                           = pins::PB14;
    constexpr uint8_t SPI2_MISO_REMAP1                    = pins::PC2;
    // SPI2_MOSI available on: PB15, PC3
    constexpr uint8_t SPI2_MOSI                           = pins::PB15;
    constexpr uint8_t SPI2_MOSI_REMAP1                    = pins::PC3;
    // SPI2_NSS available on: PB12, PB9
    constexpr uint8_t SPI2_NSS                           = pins::PB12;
    constexpr uint8_t SPI2_NSS_REMAP1                    = pins::PB9;
    // SPI2_SCK available on: PB10, PB13
    constexpr uint8_t SPI2_SCK                           = pins::PB10;
    constexpr uint8_t SPI2_SCK_REMAP1                    = pins::PB13;
    // SPI3_MISO available on: PB4, PC11
    constexpr uint8_t SPI3_MISO                           = pins::PB4;
    constexpr uint8_t SPI3_MISO_REMAP1                    = pins::PC11;
    // SPI3_MOSI available on: PB5, PC12, PD6
    constexpr uint8_t SPI3_MOSI                           = pins::PB5;
    constexpr uint8_t SPI3_MOSI_REMAP1                    = pins::PC12;
    constexpr uint8_t SPI3_MOSI_REMAP2                    = pins::PD6;
    // SPI3_NSS available on: PA15, PA4
    constexpr uint8_t SPI3_NSS                           = pins::PA15;
    constexpr uint8_t SPI3_NSS_REMAP1                    = pins::PA4;
    // SPI3_SCK available on: PB3, PC10
    constexpr uint8_t SPI3_SCK                           = pins::PB3;
    constexpr uint8_t SPI3_SCK_REMAP1                    = pins::PC10;
    // SPI4_MISO available on: PE13, PE5
    constexpr uint8_t SPI4_MISO                           = pins::PE13;
    constexpr uint8_t SPI4_MISO_REMAP1                    = pins::PE5;
    // SPI4_MOSI available on: PE14, PE6
    constexpr uint8_t SPI4_MOSI                           = pins::PE14;
    constexpr uint8_t SPI4_MOSI_REMAP1                    = pins::PE6;
    // SPI4_NSS available on: PE11, PE4
    constexpr uint8_t SPI4_NSS                           = pins::PE11;
    constexpr uint8_t SPI4_NSS_REMAP1                    = pins::PE4;
    // SPI4_SCK available on: PE12, PE2
    constexpr uint8_t SPI4_SCK                           = pins::PE12;
    constexpr uint8_t SPI4_SCK_REMAP1                    = pins::PE2;

}  // namespace spi

// SWD Functions
namespace swd {

    constexpr uint8_t SWCLK                          = pins::PA14;
    constexpr uint8_t SWDIO                          = pins::PA13;

}  // namespace swd

// SYSTEM Functions
namespace system {

    constexpr uint8_t WKUP                           = pins::PA0;

}  // namespace system

// TIM Functions
namespace tim {

    constexpr uint8_t TIM10_CH1                      = pins::PB8;
    constexpr uint8_t TIM11_CH1                      = pins::PB9;
    constexpr uint8_t TIM12_CH1                      = pins::PB14;
    constexpr uint8_t TIM12_CH2                      = pins::PB15;
    constexpr uint8_t TIM13_CH1                      = pins::PA6;
    constexpr uint8_t TIM14_CH1                      = pins::PA7;
    // TIM1_BKIN available on: PA6, PB12, PE15
    constexpr uint8_t TIM1_BKIN                           = pins::PA6;
    constexpr uint8_t TIM1_BKIN_REMAP1                    = pins::PB12;
    constexpr uint8_t TIM1_BKIN_REMAP2                    = pins::PE15;
    // TIM1_CH1 available on: PA8, PE9
    constexpr uint8_t TIM1_CH1                           = pins::PA8;
    constexpr uint8_t TIM1_CH1_REMAP1                    = pins::PE9;
    // TIM1_CH1N available on: PA7, PB13, PE8
    constexpr uint8_t TIM1_CH1N                           = pins::PA7;
    constexpr uint8_t TIM1_CH1N_REMAP1                    = pins::PB13;
    constexpr uint8_t TIM1_CH1N_REMAP2                    = pins::PE8;
    // TIM1_CH2 available on: PA9, PE11
    constexpr uint8_t TIM1_CH2                           = pins::PA9;
    constexpr uint8_t TIM1_CH2_REMAP1                    = pins::PE11;
    // TIM1_CH2N available on: PB0, PB14, PE10
    constexpr uint8_t TIM1_CH2N                           = pins::PB0;
    constexpr uint8_t TIM1_CH2N_REMAP1                    = pins::PB14;
    constexpr uint8_t TIM1_CH2N_REMAP2                    = pins::PE10;
    // TIM1_CH3 available on: PA10, PE13
    constexpr uint8_t TIM1_CH3                           = pins::PA10;
    constexpr uint8_t TIM1_CH3_REMAP1                    = pins::PE13;
    // TIM1_CH3N available on: PB1, PB15, PE12
    constexpr uint8_t TIM1_CH3N                           = pins::PB1;
    constexpr uint8_t TIM1_CH3N_REMAP1                    = pins::PB15;
    constexpr uint8_t TIM1_CH3N_REMAP2                    = pins::PE12;
    // TIM1_CH4 available on: PA11, PE14
    constexpr uint8_t TIM1_CH4                           = pins::PA11;
    constexpr uint8_t TIM1_CH4_REMAP1                    = pins::PE14;
    // TIM1_ETR available on: PA12, PE7
    constexpr uint8_t TIM1_ETR                           = pins::PA12;
    constexpr uint8_t TIM1_ETR_REMAP1                    = pins::PE7;
    // TIM2_CH1 available on: PA0, PA15, PA5
    constexpr uint8_t TIM2_CH1                           = pins::PA0;
    constexpr uint8_t TIM2_CH1_REMAP1                    = pins::PA15;
    constexpr uint8_t TIM2_CH1_REMAP2                    = pins::PA5;
    // TIM2_CH2 available on: PA1, PB3
    constexpr uint8_t TIM2_CH2                           = pins::PA1;
    constexpr uint8_t TIM2_CH2_REMAP1                    = pins::PB3;
    // TIM2_CH3 available on: PA2, PB10
    constexpr uint8_t TIM2_CH3                           = pins::PA2;
    constexpr uint8_t TIM2_CH3_REMAP1                    = pins::PB10;
    // TIM2_CH4 available on: PA3, PB11
    constexpr uint8_t TIM2_CH4                           = pins::PA3;
    constexpr uint8_t TIM2_CH4_REMAP1                    = pins::PB11;
    // TIM2_ETR available on: PA0, PA15, PA5
    constexpr uint8_t TIM2_ETR                           = pins::PA0;
    constexpr uint8_t TIM2_ETR_REMAP1                    = pins::PA15;
    constexpr uint8_t TIM2_ETR_REMAP2                    = pins::PA5;
    // TIM3_CH1 available on: PA6, PB4, PC6
    constexpr uint8_t TIM3_CH1                           = pins::PA6;
    constexpr uint8_t TIM3_CH1_REMAP1                    = pins::PB4;
    constexpr uint8_t TIM3_CH1_REMAP2                    = pins::PC6;
    // TIM3_CH2 available on: PA7, PB5, PC7
    constexpr uint8_t TIM3_CH2                           = pins::PA7;
    constexpr uint8_t TIM3_CH2_REMAP1                    = pins::PB5;
    constexpr uint8_t TIM3_CH2_REMAP2                    = pins::PC7;
    // TIM3_CH3 available on: PB0, PC8
    constexpr uint8_t TIM3_CH3                           = pins::PB0;
    constexpr uint8_t TIM3_CH3_REMAP1                    = pins::PC8;
    // TIM3_CH4 available on: PB1, PC9
    constexpr uint8_t TIM3_CH4                           = pins::PB1;
    constexpr uint8_t TIM3_CH4_REMAP1                    = pins::PC9;
    constexpr uint8_t TIM3_ETR                       = pins::PD2;
    // TIM4_CH1 available on: PB6, PD12
    constexpr uint8_t TIM4_CH1                           = pins::PB6;
    constexpr uint8_t TIM4_CH1_REMAP1                    = pins::PD12;
    // TIM4_CH2 available on: PB7, PD13
    constexpr uint8_t TIM4_CH2                           = pins::PB7;
    constexpr uint8_t TIM4_CH2_REMAP1                    = pins::PD13;
    // TIM4_CH3 available on: PB8, PD14
    constexpr uint8_t TIM4_CH3                           = pins::PB8;
    constexpr uint8_t TIM4_CH3_REMAP1                    = pins::PD14;
    // TIM4_CH4 available on: PB9, PD15
    constexpr uint8_t TIM4_CH4                           = pins::PB9;
    constexpr uint8_t TIM4_CH4_REMAP1                    = pins::PD15;
    constexpr uint8_t TIM4_ETR                       = pins::PE0;
    constexpr uint8_t TIM5_CH1                       = pins::PA0;
    constexpr uint8_t TIM5_CH2                       = pins::PA1;
    constexpr uint8_t TIM5_CH3                       = pins::PA2;
    constexpr uint8_t TIM5_CH4                       = pins::PA3;
    constexpr uint8_t TIM8_BKIN                      = pins::PA6;
    constexpr uint8_t TIM8_CH1                       = pins::PC6;
    // TIM8_CH1N available on: PA5, PA7
    constexpr uint8_t TIM8_CH1N                           = pins::PA5;
    constexpr uint8_t TIM8_CH1N_REMAP1                    = pins::PA7;
    constexpr uint8_t TIM8_CH2                       = pins::PC7;
    // TIM8_CH2N available on: PB0, PB14
    constexpr uint8_t TIM8_CH2N                           = pins::PB0;
    constexpr uint8_t TIM8_CH2N_REMAP1                    = pins::PB14;
    constexpr uint8_t TIM8_CH3                       = pins::PC8;
    // TIM8_CH3N available on: PB1, PB15
    constexpr uint8_t TIM8_CH3N                           = pins::PB1;
    constexpr uint8_t TIM8_CH3N_REMAP1                    = pins::PB15;
    constexpr uint8_t TIM8_CH4                       = pins::PC9;
    constexpr uint8_t TIM8_ETR                       = pins::PA0;
    // TIM9_CH1 available on: PA2, PE5
    constexpr uint8_t TIM9_CH1                           = pins::PA2;
    constexpr uint8_t TIM9_CH1_REMAP1                    = pins::PE5;
    // TIM9_CH2 available on: PA3, PE6
    constexpr uint8_t TIM9_CH2                           = pins::PA3;
    constexpr uint8_t TIM9_CH2_REMAP1                    = pins::PE6;

}  // namespace tim

// TRACE Functions
namespace trace {

    constexpr uint8_t TRACECLK                       = pins::PE2;
    constexpr uint8_t TRACED0                        = pins::PE3;
    constexpr uint8_t TRACED1                        = pins::PE4;
    constexpr uint8_t TRACED2                        = pins::PE5;
    constexpr uint8_t TRACED3                        = pins::PE6;
    constexpr uint8_t TRACESWO                       = pins::PB3;

}  // namespace trace

// UART Functions
namespace uart {

    constexpr uint8_t UART4_CTS                      = pins::PB0;
    constexpr uint8_t UART4_RTS                      = pins::PA15;
    // UART4_RX available on: PA1, PC11
    constexpr uint8_t UART4_RX                           = pins::PA1;
    constexpr uint8_t UART4_RX_REMAP1                    = pins::PC11;
    // UART4_TX available on: PA0, PC10
    constexpr uint8_t UART4_TX                           = pins::PA0;
    constexpr uint8_t UART4_TX_REMAP1                    = pins::PC10;
    constexpr uint8_t UART5_CTS                      = pins::PC9;
    constexpr uint8_t UART5_RTS                      = pins::PC8;
    constexpr uint8_t UART5_RX                       = pins::PD2;
    constexpr uint8_t UART5_TX                       = pins::PC12;
    constexpr uint8_t UART7_CTS                      = pins::PE10;
    constexpr uint8_t UART7_RTS                      = pins::PE9;
    // UART7_RX available on: PA8, PE7
    constexpr uint8_t UART7_RX                           = pins::PA8;
    constexpr uint8_t UART7_RX_REMAP1                    = pins::PE7;
    constexpr uint8_t UART7_TX                       = pins::PE8;
    constexpr uint8_t UART8_CTS                      = pins::PD14;
    constexpr uint8_t UART8_RTS                      = pins::PD15;
    constexpr uint8_t UART8_RX                       = pins::PE0;
    constexpr uint8_t UART8_TX                       = pins::PE1;

}  // namespace uart

// USART Functions
namespace usart {

    constexpr uint8_t USART1_CK                      = pins::PA8;
    constexpr uint8_t USART1_CTS                     = pins::PA11;
    constexpr uint8_t USART1_RTS                     = pins::PA12;
    // USART1_RX available on: PA10, PB7
    constexpr uint8_t USART1_RX                           = pins::PA10;
    constexpr uint8_t USART1_RX_REMAP1                    = pins::PB7;
    // USART1_TX available on: PA9, PB6
    constexpr uint8_t USART1_TX                           = pins::PA9;
    constexpr uint8_t USART1_TX_REMAP1                    = pins::PB6;
    // USART2_CK available on: PA4, PD7
    constexpr uint8_t USART2_CK                           = pins::PA4;
    constexpr uint8_t USART2_CK_REMAP1                    = pins::PD7;
    // USART2_CTS available on: PA0, PD3
    constexpr uint8_t USART2_CTS                           = pins::PA0;
    constexpr uint8_t USART2_CTS_REMAP1                    = pins::PD3;
    // USART2_RTS available on: PA1, PD4
    constexpr uint8_t USART2_RTS                           = pins::PA1;
    constexpr uint8_t USART2_RTS_REMAP1                    = pins::PD4;
    // USART2_RX available on: PA3, PD6
    constexpr uint8_t USART2_RX                           = pins::PA3;
    constexpr uint8_t USART2_RX_REMAP1                    = pins::PD6;
    // USART2_TX available on: PA2, PD5
    constexpr uint8_t USART2_TX                           = pins::PA2;
    constexpr uint8_t USART2_TX_REMAP1                    = pins::PD5;
    // USART3_CK available on: PB12, PC12, PD10
    constexpr uint8_t USART3_CK                           = pins::PB12;
    constexpr uint8_t USART3_CK_REMAP1                    = pins::PC12;
    constexpr uint8_t USART3_CK_REMAP2                    = pins::PD10;
    // USART3_CTS available on: PB13, PD11
    constexpr uint8_t USART3_CTS                           = pins::PB13;
    constexpr uint8_t USART3_CTS_REMAP1                    = pins::PD11;
    // USART3_RTS available on: PB14, PD12
    constexpr uint8_t USART3_RTS                           = pins::PB14;
    constexpr uint8_t USART3_RTS_REMAP1                    = pins::PD12;
    // USART3_RX available on: PB11, PC11, PD9
    constexpr uint8_t USART3_RX                           = pins::PB11;
    constexpr uint8_t USART3_RX_REMAP1                    = pins::PC11;
    constexpr uint8_t USART3_RX_REMAP2                    = pins::PD9;
    // USART3_TX available on: PB10, PC10, PD8
    constexpr uint8_t USART3_TX                           = pins::PB10;
    constexpr uint8_t USART3_TX_REMAP1                    = pins::PC10;
    constexpr uint8_t USART3_TX_REMAP2                    = pins::PD8;
    constexpr uint8_t USART6_CK                      = pins::PC8;
    constexpr uint8_t USART6_RX                      = pins::PC7;
    constexpr uint8_t USART6_TX                      = pins::PC6;

}  // namespace usart

// USB Functions
namespace usb {

    constexpr uint8_t OTG_FS_DM                      = pins::PA11;
    constexpr uint8_t OTG_FS_DP                      = pins::PA12;
    constexpr uint8_t OTG_FS_ID                      = pins::PA10;
    constexpr uint8_t OTG_FS_SOF                     = pins::PA8;
    constexpr uint8_t OTG_HS_DM                      = pins::PB14;
    constexpr uint8_t OTG_HS_DP                      = pins::PB15;
    constexpr uint8_t OTG_HS_ID                      = pins::PB12;
    constexpr uint8_t OTG_HS_SOF                     = pins::PA4;
    constexpr uint8_t OTG_HS_ULPI_CK                 = pins::PA5;
    constexpr uint8_t OTG_HS_ULPI_D0                 = pins::PA3;
    constexpr uint8_t OTG_HS_ULPI_D1                 = pins::PB0;
    constexpr uint8_t OTG_HS_ULPI_D2                 = pins::PB1;
    constexpr uint8_t OTG_HS_ULPI_D3                 = pins::PB10;
    constexpr uint8_t OTG_HS_ULPI_D4                 = pins::PB11;
    constexpr uint8_t OTG_HS_ULPI_D5                 = pins::PB12;
    constexpr uint8_t OTG_HS_ULPI_D6                 = pins::PB13;
    constexpr uint8_t OTG_HS_ULPI_D7                 = pins::PB5;
    constexpr uint8_t OTG_HS_ULPI_DIR                = pins::PC2;
    constexpr uint8_t OTG_HS_ULPI_NXT                = pins::PC3;
    constexpr uint8_t OTG_HS_ULPI_STP                = pins::PC0;

}  // namespace usb
}  // namespace alloy::hal::stm32f7::stm32f746vg::pin_functions
