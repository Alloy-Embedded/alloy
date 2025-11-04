#pragma once

#include <cstdint>

namespace alloy::hal::espressif::esp32 {

// ============================================================================
// GPIO Matrix Configuration for ESP32
// ============================================================================
//
// ESP32 has a flexible GPIO Matrix that allows routing any peripheral signal
// to any GPIO pin. This is different from traditional microcontrollers where
// alternate functions are fixed per pin.
//
// Signal routing is controlled by:
// - GPIO_FUNCx_OUT_SEL_CFG (output signal selection)  
// - GPIO_FUNCy_IN_SEL_CFG (input signal selection)
//
// This header provides helper constants and types for GPIO Matrix configuration.
// ============================================================================

// GPIO Matrix signal indices (selected common ones)
namespace signal {

// UART signals
constexpr uint32_t UART0_TXD = 14;
constexpr uint32_t UART0_RXD = 14;
constexpr uint32_t UART1_TXD = 15;
constexpr uint32_t UART1_RXD = 15;
constexpr uint32_t UART2_TXD = 16;
constexpr uint32_t UART2_RXD = 16;

// SPI signals (HSPI)
constexpr uint32_t HSPICLK  = 71;
constexpr uint32_t HSPIQ    = 72;  // MISO
constexpr uint32_t HSPID    = 73;  // MOSI
constexpr uint32_t HSPICS0  = 74;

// SPI signals (VSPI)
constexpr uint32_t VSPICLK  = 63;
constexpr uint32_t VSPIQ    = 64;  // MISO
constexpr uint32_t VSPID    = 65;  // MOSI
constexpr uint32_t VSPICS0  = 66;

// I2C signals
constexpr uint32_t I2C0_SCL_OUT = 29;
constexpr uint32_t I2C0_SDA_OUT = 30;
constexpr uint32_t I2C0_SCL_IN  = 29;
constexpr uint32_t I2C0_SDA_IN  = 30;

constexpr uint32_t I2C1_SCL_OUT = 31;
constexpr uint32_t I2C1_SDA_OUT = 32;
constexpr uint32_t I2C1_SCL_IN  = 31;
constexpr uint32_t I2C1_SDA_IN  = 32;

// PWM/LEDC signals
constexpr uint32_t LEDC_HS_SIG0 = 45;
constexpr uint32_t LEDC_HS_SIG1 = 46;
constexpr uint32_t LEDC_HS_SIG2 = 47;
constexpr uint32_t LEDC_HS_SIG3 = 48;
constexpr uint32_t LEDC_HS_SIG4 = 49;
constexpr uint32_t LEDC_HS_SIG5 = 50;
constexpr uint32_t LEDC_HS_SIG6 = 51;
constexpr uint32_t LEDC_HS_SIG7 = 52;

constexpr uint32_t LEDC_LS_SIG0 = 53;
constexpr uint32_t LEDC_LS_SIG1 = 54;
constexpr uint32_t LEDC_LS_SIG2 = 55;
constexpr uint32_t LEDC_LS_SIG3 = 56;
constexpr uint32_t LEDC_LS_SIG4 = 57;
constexpr uint32_t LEDC_LS_SIG5 = 58;
constexpr uint32_t LEDC_LS_SIG6 = 59;
constexpr uint32_t LEDC_LS_SIG7 = 60;

// Special signals
constexpr uint32_t SIG_GPIO = 0x100;  // Direct GPIO mode

}  // namespace signal

// GPIO capabilities
struct GpioCapabilities {
    bool can_output;
    bool can_input;
    bool has_pullup;
    bool has_pulldown;
    bool is_strapping;  // Used for boot mode selection
};

// Get capabilities for a GPIO pin
constexpr GpioCapabilities get_gpio_capabilities(uint8_t gpio_num) {
    // GPIO 34-39 are input-only
    if (gpio_num >= 34 && gpio_num <= 39) {
        return GpioCapabilities{
            .can_output = false,
            .can_input = true,
            .has_pullup = false,
            .has_pulldown = false,
            .is_strapping = false,
        };
    }
    
    // Strapping pins: 0, 2, 5, 12, 15
    bool is_strapping = (gpio_num == 0 || gpio_num == 2 || gpio_num == 5 || 
                        gpio_num == 12 || gpio_num == 15);
    
    // All other pins have full capabilities
    return GpioCapabilities{
        .can_output = true,
        .can_input = true,
        .has_pullup = true,
        .has_pulldown = true,
        .is_strapping = is_strapping,
    };
}

}  // namespace alloy::hal::espressif::esp32
