#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-F722ZE board
 *
 * Defines GPIO pins and hardware constants for the STM32 Nucleo-F722ZE
 * development board (MB1136).
 */

#include "hal/vendors/st/stm32f7/gpio.hpp"
#include "hal/vendors/st/stm32f7/stm32f722/peripherals.hpp"
#include <cstdint>

namespace nucleo_f722ze {

using namespace alloy::hal::st::stm32f7;
using namespace alloy::generated::stm32f722;

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// Green LED (LD1) on PB7 - Nucleo-144 user LED
    using led_green = GpioPin<peripherals::GPIOB, 7>;

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

/**
 * @brief Clock configuration for Nucleo-F722ZE
 *
 * PLL Configuration:
 * - HSE: 8 MHz external crystal
 * - PLL input: HSE / M = 8 MHz / 4 = 2 MHz
 * - VCO: 2 MHz × N = 2 MHz × 180 = 360 MHz
 * - System clock: VCO / P = 360 MHz / 2 = 180 MHz
 * - USB clock: VCO / Q = 360 MHz / 8 = 45 MHz
 *
 * Bus clocks:
 * - AHB: 180 MHz (SYSCLK / 1)
 * - APB1: 45 MHz (AHB / 4, max 54 MHz)
 * - APB2: 90 MHz (AHB / 2, max 108 MHz)
 */
struct ClockConfig {
    /// HSE crystal frequency
    static constexpr uint32_t hse_hz = 8'000'000;

    /// Target system clock frequency
    static constexpr uint32_t system_clock_hz = 180'000'000;

    /// PLL input divider (HSE / pll_m)
    static constexpr uint32_t pll_m = 4;

    /// PLL VCO multiplier (input × pll_n)
    static constexpr uint32_t pll_n = 180;

    /// PLL system clock divider (VCO / pll_p_div)
    static constexpr uint32_t pll_p_div = 2;

    /// PLL USB/SDMMC divider (VCO / pll_q)
    static constexpr uint32_t pll_q = 8;

    /// Flash latency (wait states) for 180 MHz @ 3.3V
    static constexpr uint32_t flash_latency = 5;

    /// AHB prescaler (SYSCLK / ahb_prescaler)
    static constexpr uint32_t ahb_prescaler = 1;

    /// APB1 prescaler (AHB / apb1_prescaler)
    static constexpr uint32_t apb1_prescaler = 4;

    /// APB2 prescaler (AHB / apb2_prescaler)
    static constexpr uint32_t apb2_prescaler = 2;
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
