#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-G0B1RE board
 *
 * Defines GPIO pins and hardware constants for the STM32 Nucleo-G0B1RE
 * development board (MB1360).
 */

#include <cstdint>

#include "hal/connect/tags.hpp"

namespace nucleo_g0b1re {

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// Green LED (LD4) on PA5 - Arduino D13 compatible pin
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

struct ClockConfig {
    /// System clock frequency (using HSI 64 MHz oscillator)
    /// Note: STM32G0B1 has HSI16 that can be multiplied to 64MHz
    static constexpr uint32_t system_clock_hz = 64'000'000;

    /// APB bus frequency (same as system clock on Cortex-M0+)
    static constexpr uint32_t apb_clock_hz = system_clock_hz;

    /// PLL input divider (/1)
    static constexpr uint32_t pll_m = 0;

    /// PLL multiplier (x8)
    static constexpr uint32_t pll_n = 8;

    /// PLL output divider (/2)
    static constexpr uint32_t pll_r = 0;

    /// Flash latency for 64 MHz
    static constexpr uint32_t flash_latency = 2;
};

}  // namespace nucleo_g0b1re
