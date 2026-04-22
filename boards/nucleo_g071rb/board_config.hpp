#pragma once

/**
 * @file board_config.hpp
 * @brief Hardware configuration for Nucleo-G071RB board
 *
 * Defines GPIO pins and hardware constants for the STM32 Nucleo-G071RB
 * development board (MB1360).
 */

#include <cstdint>

#include "device/system_clock.hpp"
#include "device/runtime.hpp"

namespace nucleo_g071rb {

// =============================================================================
// LED Configuration
// =============================================================================

struct LedConfig {
    /// Green LED (LD4) on PA5 - Arduino D13 compatible pin
    using led_green = alloy::device::pin<alloy::device::PinId::PA5>;

    /// LED is active HIGH (turns on when pin is HIGH)
    static constexpr bool led_green_active_high = true;
};

// =============================================================================
// Button Configuration
// =============================================================================

struct ButtonConfig {
    /// User button (B1) on PC13 - Active LOW (pressed = LOW)
    using button_user = alloy::device::pin<alloy::device::PinId::PC13>;

    /// Button is active LOW
    static constexpr bool button_active_high = false;
};

// =============================================================================
// Clock Configuration
// =============================================================================

struct ClockConfig {
    static_assert(alloy::device::SelectedSystemClockProfiles::available);

    static constexpr auto system_clock_profile =
        alloy::device::system_clock::ProfileId::default_pll_64mhz;
    using SystemClockProfile = alloy::device::system_clock::ProfileTraits<system_clock_profile>;

    static_assert(SystemClockProfile::kPresent);

    static constexpr uint32_t system_clock_hz = SystemClockProfile::kSysclkHz;
    static constexpr uint32_t apb_clock_hz = SystemClockProfile::kPclkHz;
    static constexpr uint32_t pll_m = SystemClockProfile::kPllM;
    static constexpr uint32_t pll_n = SystemClockProfile::kPllN;
    static constexpr uint32_t pll_r = SystemClockProfile::kPllR;
    static constexpr uint32_t flash_latency = SystemClockProfile::kFlashLatency;
};

}  // namespace nucleo_g071rb
