#pragma once

// clock_profile.hpp — Task 4.1 (add-clock-management-hal)
//
// ClockProfile struct and switch_profile() API.

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/clock_config.hpp"

namespace alloy::hal::clock {

/// Describes a complete clock configuration (mirrors the IR ClockProfile).
struct ClockProfile {
    std::uint32_t sysclk_hz  = 0u;
    std::uint32_t hclk_hz    = 0u;
    std::uint32_t pclk1_hz   = 0u;
    std::uint32_t pclk2_hz   = 0u;
    std::uint8_t  flash_latency = 0u;
    const char*   name        = nullptr;
};

/// Switch to a statically-selected clock profile by profile ID.
///
/// Sequence (increase → decrease):
///   increase: raise flash wait-states → ramp PLL → switch SYSCLK → (lower wait-states after)
///   decrease: switch SYSCLK → lower PLL → reduce flash wait-states
///
/// Returns Ok on success, Err(NotSupported) if the profile is not compiled in.
template <device::clock_config::ProfileId Id>
[[nodiscard]] inline auto switch_profile() -> core::Result<void, core::ErrorCode> {
#if defined(ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE) && ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE
    if (device::clock_config::apply_profile<Id>()) { return core::Ok(); }
    return core::Err(core::ErrorCode::HardwareError);
#else
    return core::Err(core::ErrorCode::NotSupported);
#endif
}

/// Switch to the device default clock profile (fastest profile available).
[[nodiscard]] inline auto switch_to_default_profile() -> core::Result<void, core::ErrorCode> {
#if defined(ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE) && ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE
    if (device::clock_config::apply_default()) { return core::Ok(); }
    return core::Err(core::ErrorCode::HardwareError);
#else
    return core::Err(core::ErrorCode::NotSupported);
#endif
}

/// Switch to the safe (lowest-power, always-works) clock profile.
[[nodiscard]] inline auto switch_to_safe_profile() -> core::Result<void, core::ErrorCode> {
#if defined(ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE) && ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE
    if (device::clock_config::apply_safe()) { return core::Ok(); }
    return core::Err(core::ErrorCode::HardwareError);
#else
    return core::Err(core::ErrorCode::NotSupported);
#endif
}

}  // namespace alloy::hal::clock
