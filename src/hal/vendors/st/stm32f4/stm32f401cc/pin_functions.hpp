#pragma once

#include <cstdint>

#include "pins.hpp"

namespace alloy::hal::stm32f4::stm32f401cc::pin_functions {

// ============================================================================
// Pin Alternate Functions for STM32F401CC
// Auto-generated from STM32F4 datasheet
// ============================================================================


// ADC Functions
namespace adc {

constexpr uint8_t ADC123_IN0 = pins::PA0;
constexpr uint8_t ADC123_IN1 = pins::PA1;
constexpr uint8_t ADC123_IN2 = pins::PA2;
constexpr uint8_t ADC123_IN3 = pins::PA3;
constexpr uint8_t ADC12_IN4 = pins::PA4;
constexpr uint8_t ADC12_IN5 = pins::PA5;
constexpr uint8_t ADC12_IN6 = pins::PA6;
constexpr uint8_t ADC12_IN7 = pins::PA7;
constexpr uint8_t ADC12_IN8 = pins::PB0;
constexpr uint8_t ADC12_IN9 = pins::PB1;

}  // namespace adc

// CAN Functions
namespace can {

// CAN1_RX available on: PA11, PB8
constexpr uint8_t CAN1_RX = pins::PA11;
constexpr uint8_t CAN1_RX_REMAP1 = pins::PB8;
// CAN1_TX available on: PA12, PB9
constexpr uint8_t CAN1_TX = pins::PA12;
constexpr uint8_t CAN1_TX_REMAP1 = pins::PB9;
// CAN2_RX available on: PB12, PB5
constexpr uint8_t CAN2_RX = pins::PB12;
constexpr uint8_t CAN2_RX_REMAP1 = pins::PB5;
// CAN2_TX available on: PB13, PB6
constexpr uint8_t CAN2_TX = pins::PB13;
constexpr uint8_t CAN2_TX_REMAP1 = pins::PB6;

}  // namespace can

// DAC Functions
namespace dac {

constexpr uint8_t DAC_OUT1 = pins::PA4;
constexpr uint8_t DAC_OUT2 = pins::PA5;

}  // namespace dac

// DCMI Functions
namespace dcmi {

constexpr uint8_t DCMI_D0 = pins::PA9;
constexpr uint8_t DCMI_D1 = pins::PA10;
constexpr uint8_t DCMI_D10 = pins::PB5;
constexpr uint8_t DCMI_D5 = pins::PB6;
constexpr uint8_t DCMI_D6 = pins::PB8;
constexpr uint8_t DCMI_D7 = pins::PB9;
constexpr uint8_t DCMI_HSYNC = pins::PA4;
constexpr uint8_t DCMI_PIXCLK = pins::PA6;
constexpr uint8_t DCMI_VSYNC = pins::PB7;

}  // namespace dcmi

// DEBUG Functions
namespace debug {

constexpr uint8_t JTCK = pins::PA14;
constexpr uint8_t JTDI = pins::PA15;
constexpr uint8_t JTDO = pins::PB3;
constexpr uint8_t JTMS = pins::PA13;
constexpr uint8_t NJTRST = pins::PB4;
constexpr uint8_t SWCLK = pins::PA14;
constexpr uint8_t SWDIO = pins::PA13;
constexpr uint8_t TRACESWO = pins::PB3;

}  // namespace debug

// ETH Functions
namespace eth {

constexpr uint8_t ETH_MDIO = pins::PA2;
constexpr uint8_t ETH_MII_COL = pins::PA3;
constexpr uint8_t ETH_MII_CRS = pins::PA0;
constexpr uint8_t ETH_MII_RXD2 = pins::PB0;
constexpr uint8_t ETH_MII_RXD3 = pins::PB1;
constexpr uint8_t ETH_MII_RX_CLK = pins::PA1;
constexpr uint8_t ETH_MII_RX_DV = pins::PA7;
constexpr uint8_t ETH_MII_RX_ER = pins::PB10;
constexpr uint8_t ETH_MII_TXD0 = pins::PB12;
constexpr uint8_t ETH_MII_TXD1 = pins::PB13;
constexpr uint8_t ETH_MII_TXD3 = pins::PB8;
constexpr uint8_t ETH_MII_TX_EN = pins::PB11;
constexpr uint8_t ETH_PPS_OUT = pins::PB5;
constexpr uint8_t ETH_RMII_CRS_DV = pins::PA7;
constexpr uint8_t ETH_RMII_REF_CLK = pins::PA1;
constexpr uint8_t ETH_RMII_TXD0 = pins::PB12;
constexpr uint8_t ETH_RMII_TXD1 = pins::PB13;
constexpr uint8_t ETH_RMII_TX_EN = pins::PB11;

}  // namespace eth

// I2C Functions
namespace i2c {

// I2C1_SCL available on: PB6, PB8
constexpr uint8_t I2C1_SCL = pins::PB6;
constexpr uint8_t I2C1_SCL_REMAP1 = pins::PB8;
// I2C1_SDA available on: PB7, PB9
constexpr uint8_t I2C1_SDA = pins::PB7;
constexpr uint8_t I2C1_SDA_REMAP1 = pins::PB9;
constexpr uint8_t I2C1_SMBA = pins::PB5;
constexpr uint8_t I2C2_SCL = pins::PB10;
constexpr uint8_t I2C2_SDA = pins::PB11;
constexpr uint8_t I2C2_SMBA = pins::PB12;
constexpr uint8_t I2C3_SCL = pins::PA8;
constexpr uint8_t I2C3_SMBA = pins::PA9;

}  // namespace i2c

// I2S Functions
namespace i2s {

constexpr uint8_t I2S2EXT_SD = pins::PB14;
// I2S2_CK available on: PB10, PB13
constexpr uint8_t I2S2_CK = pins::PB10;
constexpr uint8_t I2S2_CK_REMAP1 = pins::PB13;
constexpr uint8_t I2S2_SD = pins::PB15;
// I2S2_WS available on: PB12, PB9
constexpr uint8_t I2S2_WS = pins::PB12;
constexpr uint8_t I2S2_WS_REMAP1 = pins::PB9;
constexpr uint8_t I2S3EXT_SD = pins::PB4;
constexpr uint8_t I2S3_CK = pins::PB3;
constexpr uint8_t I2S3_SD = pins::PB5;
constexpr uint8_t I2S3_WS = pins::PA4;

}  // namespace i2s

// RTC Functions
namespace rtc {

constexpr uint8_t OSC32_IN = pins::PC14;
constexpr uint8_t OSC32_OUT = pins::PC15;
constexpr uint8_t RTC_AF1 = pins::PC13;
constexpr uint8_t RTC_REFIN = pins::PB15;

}  // namespace rtc

// SDIO Functions
namespace sdio {

constexpr uint8_t SDIO_D4 = pins::PB8;
constexpr uint8_t SDIO_D5 = pins::PB9;

}  // namespace sdio

// SPI Functions
namespace spi {

// SPI1_MISO available on: PA6, PB4
constexpr uint8_t SPI1_MISO = pins::PA6;
constexpr uint8_t SPI1_MISO_REMAP1 = pins::PB4;
// SPI1_MOSI available on: PA7, PB5
constexpr uint8_t SPI1_MOSI = pins::PA7;
constexpr uint8_t SPI1_MOSI_REMAP1 = pins::PB5;
// SPI1_NSS available on: PA15, PA4
constexpr uint8_t SPI1_NSS = pins::PA15;
constexpr uint8_t SPI1_NSS_REMAP1 = pins::PA4;
// SPI1_SCK available on: PA5, PB3
constexpr uint8_t SPI1_SCK = pins::PA5;
constexpr uint8_t SPI1_SCK_REMAP1 = pins::PB3;
constexpr uint8_t SPI2_MISO = pins::PB14;
constexpr uint8_t SPI2_MOSI = pins::PB15;
// SPI2_NSS available on: PB12, PB9
constexpr uint8_t SPI2_NSS = pins::PB12;
constexpr uint8_t SPI2_NSS_REMAP1 = pins::PB9;
// SPI2_SCK available on: PB10, PB13
constexpr uint8_t SPI2_SCK = pins::PB10;
constexpr uint8_t SPI2_SCK_REMAP1 = pins::PB13;
constexpr uint8_t SPI3_MISO = pins::PB4;
constexpr uint8_t SPI3_MOSI = pins::PB5;
// SPI3_NSS available on: PA15, PA4
constexpr uint8_t SPI3_NSS = pins::PA15;
constexpr uint8_t SPI3_NSS_REMAP1 = pins::PA4;
constexpr uint8_t SPI3_SCK = pins::PB3;

}  // namespace spi

// SYSTEM Functions
namespace system {

constexpr uint8_t BOOT1 = pins::PB2;
constexpr uint8_t MCO1 = pins::PA8;
constexpr uint8_t WKUP = pins::PA0;

}  // namespace system

// TIM Functions
namespace tim {

constexpr uint8_t TIM10_CH1 = pins::PB8;
constexpr uint8_t TIM11_CH1 = pins::PB9;
constexpr uint8_t TIM12_CH1 = pins::PB14;
constexpr uint8_t TIM12_CH2 = pins::PB15;
constexpr uint8_t TIM13_CH1 = pins::PA6;
constexpr uint8_t TIM14_CH1 = pins::PA7;
// TIM1_BKIN available on: PA6, PB12
constexpr uint8_t TIM1_BKIN = pins::PA6;
constexpr uint8_t TIM1_BKIN_REMAP1 = pins::PB12;
constexpr uint8_t TIM1_CH1 = pins::PA8;
// TIM1_CH1N available on: PA7, PB13
constexpr uint8_t TIM1_CH1N = pins::PA7;
constexpr uint8_t TIM1_CH1N_REMAP1 = pins::PB13;
constexpr uint8_t TIM1_CH2 = pins::PA9;
// TIM1_CH2N available on: PB0, PB14
constexpr uint8_t TIM1_CH2N = pins::PB0;
constexpr uint8_t TIM1_CH2N_REMAP1 = pins::PB14;
constexpr uint8_t TIM1_CH3 = pins::PA10;
// TIM1_CH3N available on: PB1, PB15
constexpr uint8_t TIM1_CH3N = pins::PB1;
constexpr uint8_t TIM1_CH3N_REMAP1 = pins::PB15;
constexpr uint8_t TIM1_CH4 = pins::PA11;
constexpr uint8_t TIM1_ETR = pins::PA12;
// TIM2_CH1 available on: PA0, PA15, PA5
constexpr uint8_t TIM2_CH1 = pins::PA0;
constexpr uint8_t TIM2_CH1_REMAP1 = pins::PA15;
constexpr uint8_t TIM2_CH1_REMAP2 = pins::PA5;
// TIM2_CH2 available on: PA1, PB3
constexpr uint8_t TIM2_CH2 = pins::PA1;
constexpr uint8_t TIM2_CH2_REMAP1 = pins::PB3;
// TIM2_CH3 available on: PA2, PB10
constexpr uint8_t TIM2_CH3 = pins::PA2;
constexpr uint8_t TIM2_CH3_REMAP1 = pins::PB10;
// TIM2_CH4 available on: PA3, PB11
constexpr uint8_t TIM2_CH4 = pins::PA3;
constexpr uint8_t TIM2_CH4_REMAP1 = pins::PB11;
// TIM2_ETR available on: PA0, PA15, PA5
constexpr uint8_t TIM2_ETR = pins::PA0;
constexpr uint8_t TIM2_ETR_REMAP1 = pins::PA15;
constexpr uint8_t TIM2_ETR_REMAP2 = pins::PA5;
// TIM3_CH1 available on: PA6, PB4
constexpr uint8_t TIM3_CH1 = pins::PA6;
constexpr uint8_t TIM3_CH1_REMAP1 = pins::PB4;
// TIM3_CH2 available on: PA7, PB5
constexpr uint8_t TIM3_CH2 = pins::PA7;
constexpr uint8_t TIM3_CH2_REMAP1 = pins::PB5;
constexpr uint8_t TIM3_CH3 = pins::PB0;
constexpr uint8_t TIM3_CH4 = pins::PB1;
constexpr uint8_t TIM4_CH1 = pins::PB6;
constexpr uint8_t TIM4_CH2 = pins::PB7;
constexpr uint8_t TIM4_CH3 = pins::PB8;
constexpr uint8_t TIM4_CH4 = pins::PB9;
constexpr uint8_t TIM5_CH1 = pins::PA0;
constexpr uint8_t TIM5_CH2 = pins::PA1;
constexpr uint8_t TIM5_CH3 = pins::PA2;
constexpr uint8_t TIM5_CH4 = pins::PA3;
constexpr uint8_t TIM8_BKIN = pins::PA6;
// TIM8_CH1N available on: PA5, PA7
constexpr uint8_t TIM8_CH1N = pins::PA5;
constexpr uint8_t TIM8_CH1N_REMAP1 = pins::PA7;
// TIM8_CH2N available on: PB0, PB14
constexpr uint8_t TIM8_CH2N = pins::PB0;
constexpr uint8_t TIM8_CH2N_REMAP1 = pins::PB14;
// TIM8_CH3N available on: PB1, PB15
constexpr uint8_t TIM8_CH3N = pins::PB1;
constexpr uint8_t TIM8_CH3N_REMAP1 = pins::PB15;
constexpr uint8_t TIM8_ETR = pins::PA0;
constexpr uint8_t TIM9_CH1 = pins::PA2;
constexpr uint8_t TIM9_CH2 = pins::PA3;

}  // namespace tim

// UART Functions
namespace uart {

constexpr uint8_t UART4_RX = pins::PA1;
constexpr uint8_t UART4_TX = pins::PA0;

}  // namespace uart

// USART Functions
namespace usart {

constexpr uint8_t USART1_CK = pins::PA8;
constexpr uint8_t USART1_CTS = pins::PA11;
constexpr uint8_t USART1_RTS = pins::PA12;
// USART1_RX available on: PA10, PB7
constexpr uint8_t USART1_RX = pins::PA10;
constexpr uint8_t USART1_RX_REMAP1 = pins::PB7;
// USART1_TX available on: PA9, PB6
constexpr uint8_t USART1_TX = pins::PA9;
constexpr uint8_t USART1_TX_REMAP1 = pins::PB6;
constexpr uint8_t USART2_CK = pins::PA4;
constexpr uint8_t USART2_CTS = pins::PA0;
constexpr uint8_t USART2_RTS = pins::PA1;
constexpr uint8_t USART2_RX = pins::PA3;
constexpr uint8_t USART2_TX = pins::PA2;
constexpr uint8_t USART3_CK = pins::PB12;
constexpr uint8_t USART3_CTS = pins::PB13;
constexpr uint8_t USART3_RTS = pins::PB14;
constexpr uint8_t USART3_RX = pins::PB11;
constexpr uint8_t USART3_TX = pins::PB10;

}  // namespace usart

// USB Functions
namespace usb {

constexpr uint8_t OTG_FS_DM = pins::PA11;
constexpr uint8_t OTG_FS_DP = pins::PA12;
constexpr uint8_t OTG_FS_ID = pins::PA10;
constexpr uint8_t OTG_FS_SOF = pins::PA8;
constexpr uint8_t OTG_FS_VBUS = pins::PA9;
constexpr uint8_t OTG_HS_DM = pins::PB14;
constexpr uint8_t OTG_HS_DP = pins::PB15;
constexpr uint8_t OTG_HS_ID = pins::PB12;
constexpr uint8_t OTG_HS_SOF = pins::PA4;
constexpr uint8_t OTG_HS_ULPI_CK = pins::PA5;
constexpr uint8_t OTG_HS_ULPI_D0 = pins::PA3;
constexpr uint8_t OTG_HS_ULPI_D1 = pins::PB0;
constexpr uint8_t OTG_HS_ULPI_D2 = pins::PB1;
constexpr uint8_t OTG_HS_ULPI_D3 = pins::PB10;
constexpr uint8_t OTG_HS_ULPI_D4 = pins::PB11;
constexpr uint8_t OTG_HS_ULPI_D5 = pins::PB12;
constexpr uint8_t OTG_HS_ULPI_D6 = pins::PB13;
constexpr uint8_t OTG_HS_ULPI_D7 = pins::PB5;

}  // namespace usb
}  // namespace alloy::hal::stm32f4::stm32f401cc::pin_functions
