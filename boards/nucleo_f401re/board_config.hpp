#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-F401RE board
 *
 * Defines GPIO pins and hardware constants for the STM32 Nucleo-F401RE
 * development board (MB1136).
 */

#include "hal/platform/st/stm32f4/gpio.hpp"
#include "hal/vendors/st/stm32f4/stm32f401/peripherals.hpp"
#include <cstdint>

namespace nucleo_f401re {

using namespace alloy::hal::st::stm32f4;
using namespace alloy::generated::stm32f401;

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// Green LED (LD2) on PA5 - Arduino D13 compatible pin
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
    /// HSE crystal frequency
    static constexpr uint32_t hse_hz = 8'000'000;  // 8 MHz external crystal on Nucleo

    /// System clock frequency (using HSE + PLL)
    /// HSE (8 MHz) / M (4) × N (168) / P (2) = 84 MHz
    static constexpr uint32_t system_clock_hz = 84'000'000;

    /// AHB bus frequency (same as system clock)
    static constexpr uint32_t ahb_clock_hz = system_clock_hz;

    /// APB1 bus frequency (max 42 MHz for STM32F401)
    static constexpr uint32_t apb1_clock_hz = 42'000'000;  // AHB / 2

    /// APB2 bus frequency (max 84 MHz for STM32F401)
    static constexpr uint32_t apb2_clock_hz = 84'000'000;  // AHB / 1
};

// =============================================================================
// UART Configuration (ST-Link Virtual COM Port)
// =============================================================================

struct UartConfig {
    /// USART2 TX pin - PA2 (ST-Link VCP TX, Arduino D1)
    using usart2_tx = GpioPin<peripherals::GPIOA, 2>;

    /// USART2 RX pin - PA3 (ST-Link VCP RX, Arduino D0)
    using usart2_rx = GpioPin<peripherals::GPIOA, 3>;

    /// Default baud rate for debug UART
    static constexpr uint32_t default_baud_rate = 115200;
};

} // namespace nucleo_f401re
