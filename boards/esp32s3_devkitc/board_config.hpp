#pragma once

#include <cstdint>

#include "device/system_clock.hpp"
#include "device/runtime.hpp"

namespace esp32s3_devkitc {

struct LedConfig {
    // ESP32-S3-DevKitC-1: RGB WS2812 on GPIO48 (not yet in bootstrap descriptor).
    // GPIO8 used as placeholder until the pin bootstrap is extended.
    using led_green = alloy::device::pin<alloy::device::PinId::GPIO8>;
    static constexpr bool led_green_active_high = true;
};

struct ClockConfig {
    static_assert(alloy::device::SelectedSystemClockProfiles::available);

    static constexpr auto system_clock_profile =
        alloy::device::system_clock::ProfileId::default_pll_240mhz;
    using SystemClockProfile =
        alloy::device::system_clock::ProfileTraits<system_clock_profile>;

    static_assert(SystemClockProfile::kPresent);

    static constexpr std::uint32_t cpu_freq_hz  = SystemClockProfile::kSysclkHz;
    static constexpr std::uint32_t pclk_freq_hz = SystemClockProfile::kPclkHz;
};

}  // namespace esp32s3_devkitc
