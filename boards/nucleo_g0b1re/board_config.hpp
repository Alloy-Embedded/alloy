#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-G0B1RE board
 *
 * Defines GPIO pins and hardware constants for the STM32 Nucleo-G0B1RE
 * development board (MB1360).
 */

#include "hal/platform/st/stm32g0/gpio.hpp"
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"
#include <cstdint>

namespace nucleo_g0b1re {

using namespace alloy::hal::st::stm32g0;
using namespace alloy::generated::stm32g0b1;

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// Green LED (LD4) on PA5 - Arduino D13 compatible pin
    using led_green = GpioPin<peripherals::GPIOA, 5>;

    /// LED is active HIGH (turns on when pin is HIGH)
    static constexpr bool led_green_active_high = true;
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    /// User button (B1) on PC13 - Active LOW (pressed = LOW)
    using button_user = GpioPin<peripherals::GPIOC, 13>;

    /// Button is active LOW
    static constexpr bool button_active_high = false;
};

// =============================================================================
// Clock Configuration
// =============================================================================

struct ClockConfig {
    /// System clock frequency (using HSI 64 MHz oscillator)
    /// Note: STM32G0B1 has HSI16 that can be multiplied to 64MHz
    static constexpr uint32_t system_clock_hz = 64'000'000;

    /// APB bus frequency (same as system clock on Cortex-M0+)
    static constexpr uint32_t apb_clock_hz = system_clock_hz;
};

// =============================================================================
// UART Configuration (ST-Link Virtual COM Port)
// =============================================================================

struct UartConfig {
    /// USART2 TX pin - PA2 (ST-Link VCP TX)
    using usart2_tx = GpioPin<peripherals::GPIOA, 2>;

    /// USART2 RX pin - PA3 (ST-Link VCP RX)
    using usart2_rx = GpioPin<peripherals::GPIOA, 3>;

    /// Default baud rate for debug UART
    static constexpr uint32_t default_baud_rate = 115200;
};

} // namespace nucleo_g0b1re
