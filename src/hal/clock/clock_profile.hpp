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
template <device::ClockProfileId Id>
[[nodiscard]] inline auto switch_profile() -> core::Result<void, core::ErrorCode> {
    if constexpr (requires { device::apply_clock_profile<Id>(); }) {
        const bool ok = device::apply_clock_profile<Id>();
        return ok ? core::Ok() : core::Err(core::ErrorCode::IoError);
    } else {
        return core::Err(core::ErrorCode::NotSupported);
    }
}

/// Switch to the device default clock profile (fastest profile available).
[[nodiscard]] inline auto switch_to_default_profile() -> core::Result<void, core::ErrorCode> {
    if constexpr (requires { device::apply_default_clock_profile(); }) {
        const bool ok = device::apply_default_clock_profile();
        return ok ? core::Ok() : core::Err(core::ErrorCode::IoError);
    } else {
        return core::Err(core::ErrorCode::NotSupported);
    }
}

/// Switch to the safe (lowest-power, always-works) clock profile.
[[nodiscard]] inline auto switch_to_safe_profile() -> core::Result<void, core::ErrorCode> {
    if constexpr (requires { device::apply_safe_clock_profile(); }) {
        const bool ok = device::apply_safe_clock_profile();
        return ok ? core::Ok() : core::Err(core::ErrorCode::IoError);
    } else {
        return core::Err(core::ErrorCode::NotSupported);
    }
}

}  // namespace alloy::hal::clock
