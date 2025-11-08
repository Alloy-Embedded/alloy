#pragma once

#include <cstdint>

#include "pins.hpp"

namespace alloy::hal::stm32f1::stm32f103re::pin_functions {

// ============================================================================
// Pin Alternate Functions for STM32F103RE
// Auto-generated from STM32F1 datasheet
// ============================================================================

// Pin function types (STM32F1 specific)
enum class PinMode : uint8_t {
    Input = 0,
    Output_10MHz = 1,
    Output_2MHz = 2,
    Output_50MHz = 3,
};

enum class PinConfig : uint8_t {
    // Input modes (when PinMode = Input)
    Analog = 0,
    Floating = 1,
    PullUpDown = 2,

    // Output modes (when PinMode = Output_*)
    PushPull = 0,
    OpenDrain = 1,
    AltFunctionPushPull = 2,
    AltFunctionOpenDrain = 3,
};


// ADC Functions
namespace adc {

constexpr uint8_t ADC12_IN0 = pins::PA0;
constexpr uint8_t ADC12_IN1 = pins::PA1;
constexpr uint8_t ADC12_IN10 = pins::PC0;
constexpr uint8_t ADC12_IN11 = pins::PC1;
constexpr uint8_t ADC12_IN12 = pins::PC2;
constexpr uint8_t ADC12_IN13 = pins::PC3;
constexpr uint8_t ADC12_IN14 = pins::PC4;
constexpr uint8_t ADC12_IN15 = pins::PC5;
constexpr uint8_t ADC12_IN2 = pins::PA2;
constexpr uint8_t ADC12_IN3 = pins::PA3;
constexpr uint8_t ADC12_IN4 = pins::PA4;
constexpr uint8_t ADC12_IN5 = pins::PA5;
constexpr uint8_t ADC12_IN6 = pins::PA6;
constexpr uint8_t ADC12_IN7 = pins::PA7;
constexpr uint8_t ADC12_IN8 = pins::PB0;
constexpr uint8_t ADC12_IN9 = pins::PB1;

}  // namespace adc

// CAN Functions
namespace can {

// CAN_RX available on: PA11, PB8, PD0
constexpr uint8_t CAN_RX = pins::PA11;
constexpr uint8_t CAN_RX_REMAP1 = pins::PB8;
constexpr uint8_t CAN_RX_REMAP2 = pins::PD0;
// CAN_TX available on: PA12, PB9, PD1
constexpr uint8_t CAN_TX = pins::PA12;
constexpr uint8_t CAN_TX_REMAP1 = pins::PB9;
constexpr uint8_t CAN_TX_REMAP2 = pins::PD1;

}  // namespace can

// DEBUG Functions
namespace debug {

constexpr uint8_t JTCK_SWCLK = pins::PA14;
constexpr uint8_t JTDI = pins::PA15;
constexpr uint8_t JTDO = pins::PB3;
constexpr uint8_t JTMS_SWDIO = pins::PA13;
constexpr uint8_t NJTRST = pins::PB4;
constexpr uint8_t TRACESWO = pins::PB3;

}  // namespace debug

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

}  // namespace i2c

// RTC Functions
namespace rtc {

constexpr uint8_t OSC32_IN = pins::PC14;
constexpr uint8_t OSC32_OUT = pins::PC15;
constexpr uint8_t TAMPER_RTC = pins::PC13;

}  // namespace rtc

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
constexpr uint8_t SPI2_NSS = pins::PB12;
constexpr uint8_t SPI2_SCK = pins::PB13;

}  // namespace spi

// SYSTEM Functions
namespace system {

constexpr uint8_t BOOT1 = pins::PB2;
constexpr uint8_t MCO = pins::PA8;
constexpr uint8_t OSC_IN = pins::PD0;
constexpr uint8_t OSC_OUT = pins::PD1;
constexpr uint8_t WKUP = pins::PA0;

}  // namespace system

// TIM Functions
namespace tim {

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
// TIM2_CH1_ETR available on: PA0, PA15
constexpr uint8_t TIM2_CH1_ETR = pins::PA0;
constexpr uint8_t TIM2_CH1_ETR_REMAP1 = pins::PA15;
// TIM2_CH2 available on: PA1, PB3
constexpr uint8_t TIM2_CH2 = pins::PA1;
constexpr uint8_t TIM2_CH2_REMAP1 = pins::PB3;
// TIM2_CH3 available on: PA2, PB10
constexpr uint8_t TIM2_CH3 = pins::PA2;
constexpr uint8_t TIM2_CH3_REMAP1 = pins::PB10;
// TIM2_CH4 available on: PA3, PB11
constexpr uint8_t TIM2_CH4 = pins::PA3;
constexpr uint8_t TIM2_CH4_REMAP1 = pins::PB11;
// TIM3_CH1 available on: PA6, PB4, PC6
constexpr uint8_t TIM3_CH1 = pins::PA6;
constexpr uint8_t TIM3_CH1_REMAP1 = pins::PB4;
constexpr uint8_t TIM3_CH1_REMAP2 = pins::PC6;
// TIM3_CH2 available on: PA7, PB5, PC7
constexpr uint8_t TIM3_CH2 = pins::PA7;
constexpr uint8_t TIM3_CH2_REMAP1 = pins::PB5;
constexpr uint8_t TIM3_CH2_REMAP2 = pins::PC7;
// TIM3_CH3 available on: PB0, PC8
constexpr uint8_t TIM3_CH3 = pins::PB0;
constexpr uint8_t TIM3_CH3_REMAP1 = pins::PC8;
// TIM3_CH4 available on: PB1, PC9
constexpr uint8_t TIM3_CH4 = pins::PB1;
constexpr uint8_t TIM3_CH4_REMAP1 = pins::PC9;
constexpr uint8_t TIM3_ETR = pins::PD2;
constexpr uint8_t TIM4_CH1 = pins::PB6;
constexpr uint8_t TIM4_CH2 = pins::PB7;
constexpr uint8_t TIM4_CH3 = pins::PB8;
constexpr uint8_t TIM4_CH4 = pins::PB9;

}  // namespace tim

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
// USART3_CK available on: PB12, PC12
constexpr uint8_t USART3_CK = pins::PB12;
constexpr uint8_t USART3_CK_REMAP1 = pins::PC12;
constexpr uint8_t USART3_CTS = pins::PB13;
constexpr uint8_t USART3_RTS = pins::PB14;
// USART3_RX available on: PB11, PC11
constexpr uint8_t USART3_RX = pins::PB11;
constexpr uint8_t USART3_RX_REMAP1 = pins::PC11;
// USART3_TX available on: PB10, PC10
constexpr uint8_t USART3_TX = pins::PB10;
constexpr uint8_t USART3_TX_REMAP1 = pins::PC10;

}  // namespace usart

// USB Functions
namespace usb {

constexpr uint8_t USB_DM = pins::PA11;
constexpr uint8_t USB_DP = pins::PA12;

}  // namespace usb

// ============================================================================
// Pin Configuration Helpers (STM32F1 specific)
// ============================================================================

// ADC pin configuration helper
template <uint8_t Pin>
struct ADCPinConfig {
    static constexpr PinMode mode = PinMode::Input;
    static constexpr PinConfig config = PinConfig::Analog;
};

// PWM (Timer) output pin configuration helper
template <uint8_t Pin>
struct PWMPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionPushPull;
};

// UART TX pin configuration helper
template <uint8_t Pin>
struct UARTTxPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionPushPull;
};

// UART RX pin configuration helper
template <uint8_t Pin>
struct UARTRxPinConfig {
    static constexpr PinMode mode = PinMode::Input;
    static constexpr PinConfig config = PinConfig::Floating;
};

// I2C pin configuration helper (both SCL and SDA)
template <uint8_t Pin>
struct I2CPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionOpenDrain;
};

// SPI Master pin configuration helpers
template <uint8_t Pin>
struct SPIMOSIPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionPushPull;
};

template <uint8_t Pin>
struct SPIMISOPinConfig {
    static constexpr PinMode mode = PinMode::Input;
    static constexpr PinConfig config = PinConfig::Floating;
};

template <uint8_t Pin>
struct SPISCKPinConfig {
    static constexpr PinMode mode = PinMode::Output_50MHz;
    static constexpr PinConfig config = PinConfig::AltFunctionPushPull;
};

}  // namespace alloy::hal::stm32f1::stm32f103re::pin_functions
