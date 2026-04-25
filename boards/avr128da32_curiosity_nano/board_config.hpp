#pragma once

#include <cstdint>

#include "device/system_clock.hpp"
#include "device/runtime.hpp"

namespace avr128da32_curiosity_nano {

struct LedConfig {
    // AVR128DA32 Curiosity Nano: LED on PC6 (active LOW).
    // NOTE: PC6 is not yet in the bootstrap pin descriptor (only PA0-PA7,
    // PC0-PC1 are included). PA5 is used as a placeholder until the pin
    // bootstrap is extended to include port C fully.
    using led_green = alloy::device::pin<alloy::device::PinId::PA5>;
    static constexpr bool led_green_active_high = false;
};

struct ClockConfig {
    static_assert(alloy::device::SelectedSystemClockProfiles::available);

    static constexpr auto system_clock_profile =
        alloy::device::system_clock::ProfileId::default_osc20m_24mhz;
    using SystemClockProfile =
        alloy::device::system_clock::ProfileTraits<system_clock_profile>;

    static_assert(SystemClockProfile::kPresent);

    static constexpr std::uint32_t cpu_freq_hz  = SystemClockProfile::kSysclkHz;
    static constexpr std::uint32_t pclk_freq_hz = SystemClockProfile::kPclkHz;
};

}  // namespace avr128da32_curiosity_nano
