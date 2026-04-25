#pragma once

#include <cstdint>

#include "device/system_clock.hpp"
#include "device/runtime.hpp"

namespace raspberry_pi_pico {

struct LedConfig {
    // GP25 = onboard LED on Raspberry Pi Pico (active HIGH)
    using led_green = alloy::device::pin<alloy::device::PinId::GP25>;
    static constexpr bool led_green_active_high = true;
};

struct ButtonConfig {
    // No dedicated user button on standard Pico; placeholder unused.
};

struct ClockConfig {
    static_assert(alloy::device::SelectedSystemClockProfiles::available);

    static constexpr auto system_clock_profile =
        alloy::device::system_clock::ProfileId::default_pll_125mhz;
    using SystemClockProfile =
        alloy::device::system_clock::ProfileTraits<system_clock_profile>;

    static_assert(SystemClockProfile::kPresent);

    static constexpr std::uint32_t cpu_freq_hz  = SystemClockProfile::kSysclkHz;
    static constexpr std::uint32_t pclk_freq_hz = SystemClockProfile::kPclkHz;
};

}  // namespace raspberry_pi_pico
