#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-F401RE board
 *
 * Defines GPIO pins and hardware constants for the STM32 Nucleo-F401RE
 * development board (MB1136).
 */

#include <cstdint>

#include "device/system_clock.hpp"
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
    static_assert(alloy::device::SelectedSystemClockProfiles::available);

    static constexpr auto system_clock_profile =
        alloy::device::system_clock::ProfileId::default_hse_pll_84mhz;
    using SystemClockProfile =
        alloy::device::system_clock::ProfileTraits<system_clock_profile>;

    static_assert(SystemClockProfile::kPresent);

    static constexpr uint32_t hse_hz = SystemClockProfile::kSourceHz;
    static constexpr uint32_t system_clock_hz = SystemClockProfile::kSysclkHz;
    static constexpr uint32_t ahb_clock_hz = SystemClockProfile::kHclkHz;
    static constexpr uint32_t apb1_hz = SystemClockProfile::kApb1Hz;
    static constexpr uint32_t apb2_hz = SystemClockProfile::kApb2Hz;
    static constexpr uint32_t pll_m = SystemClockProfile::kPllM;
    static constexpr uint32_t pll_n = SystemClockProfile::kPllN;
    static constexpr uint32_t pll_p_div = SystemClockProfile::kPllP;
    static constexpr uint32_t pll_q = SystemClockProfile::kPllQ;
    static constexpr uint32_t flash_latency = SystemClockProfile::kFlashLatency;
    static constexpr uint32_t ahb_prescaler = SystemClockProfile::kAhbPrescaler;
    static constexpr uint32_t apb1_prescaler = SystemClockProfile::kApb1Prescaler;
    static constexpr uint32_t apb2_prescaler = SystemClockProfile::kApb2Prescaler;
};

}  // namespace nucleo_f401re
