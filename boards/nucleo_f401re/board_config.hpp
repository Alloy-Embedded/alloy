#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-F401RE board
 *
 * Defines GPIO pins and hardware constants for the STM32 Nucleo-F401RE
 * development board (MB1136).
 */

#include <cstdint>

#include "hal/connect/tags.hpp"

namespace nucleo_f401re {

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// Green LED (LD2) on PA5 - Arduino D13 compatible pin
    using led_green = alloy::hal::pin<"PA5">;

    /// LED is active HIGH (turns on when pin is HIGH)
    static constexpr bool led_green_active_high = true;
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    /// User button (B1) on PC13 - Active LOW (pressed = LOW)
    using button_user = alloy::hal::pin<"PC13">;

    /// Button is active LOW
    static constexpr bool button_active_high = false;
};

// =============================================================================
// Clock Configuration
// =============================================================================

/**
 * @brief Clock configuration for Nucleo-F401RE
 *
 * PLL Configuration:
 * - HSE: 8 MHz external crystal
 * - PLL input: HSE / M = 8 MHz / 4 = 2 MHz
 * - VCO: 2 MHz × N = 2 MHz × 168 = 336 MHz
 * - System clock: VCO / P = 336 MHz / 4 = 84 MHz
 * - USB clock: VCO / Q = 336 MHz / 7 = 48 MHz
 *
 * Bus clocks:
 * - AHB: 84 MHz (SYSCLK / 1)
 * - APB1: 42 MHz (AHB / 2, max 42 MHz)
 * - APB2: 84 MHz (AHB / 1, max 84 MHz)
 */
struct ClockConfig {
    /// HSE crystal frequency
    static constexpr uint32_t hse_hz = 8'000'000;

    /// Target system clock frequency
    static constexpr uint32_t system_clock_hz = 84'000'000;

    /// PLL input divider (HSE / pll_m)
    static constexpr uint32_t pll_m = 4;

    /// PLL VCO multiplier (input × pll_n)
    static constexpr uint32_t pll_n = 168;

    /// PLL system clock divider (VCO / pll_p_div)
    static constexpr uint32_t pll_p_div = 4;

    /// PLL USB/SDMMC divider (VCO / pll_q)
    static constexpr uint32_t pll_q = 7;

    /// Flash latency (wait states) for 84 MHz @ 3.3V
    static constexpr uint32_t flash_latency = 2;

    /// AHB prescaler (SYSCLK / ahb_prescaler)
    static constexpr uint32_t ahb_prescaler = 1;

    /// APB1 prescaler (AHB / apb1_prescaler)
    static constexpr uint32_t apb1_prescaler = 2;

    /// APB2 prescaler (AHB / apb2_prescaler)
    static constexpr uint32_t apb2_prescaler = 1;
};

}  // namespace nucleo_f401re
