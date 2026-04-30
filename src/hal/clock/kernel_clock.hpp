#pragma once

// kernel_clock.hpp — Tasks 3.1–3.3 (add-clock-management-hal)
//
// set_kernel_clock<P>(KernelClockSource) — write the RCC kernel-clock MUX for
// peripheral P.
//
// For each device, the generated ClockSemanticTraits<P> provides:
//   kKernelClockMuxField  — RuntimeFieldRef into RCC_CCIPR/RCC_D2CCIP2R/…
//   kKernelClockMap[]     — array of {KernelClockSource, field_value} pairs
//
// If the peripheral has no kernel clock selector (e.g. GPIO, Timer on APB),
// the call is a no-op returning NotSupported.

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/clock/kernel_clock_source.hpp"
#include "hal/detail/runtime_ops.hpp"
#include "device/runtime.hpp"

namespace alloy::hal::clock {

namespace detail {

namespace rt = alloy::hal::detail::runtime;

/// Lookup the hardware field value for a KernelClockSource in a map array.
template <typename Map, std::size_t N>
[[nodiscard]] constexpr auto find_field_value(const Map (&map)[N],
                                               KernelClockSource src,
                                               std::uint32_t not_found) -> std::uint32_t {
    for (std::size_t i = 0; i < N; ++i) {
        if (map[i].source == src) {
            return map[i].field_value;
        }
    }
    return not_found;
}

}  // namespace detail

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Set the kernel clock MUX for peripheral P.
///
/// Requires generated ClockSemanticTraits<P> with:
///   kKernelClockMuxField   — RuntimeFieldRef into RCC clock-selector register
///   kKernelClockMap        — array of {source, field_value} pairs
///
/// Returns:
///   Ok()             — field written successfully
///   Err(InvalidParameter) — KernelClockSource not in this peripheral's map
///   Err(NotSupported)     — peripheral has no kernel clock selector
template <device::runtime::PeripheralId P>
[[nodiscard]] auto set_kernel_clock(KernelClockSource source)
    -> core::Result<void, core::ErrorCode> {
    namespace rt = alloy::hal::detail::runtime;

    if constexpr (requires { rt::ClockSemanticTraits<P>::kKernelClockMuxField; }) {
        constexpr auto field = rt::ClockSemanticTraits<P>::kKernelClockMuxField;
        if constexpr (!field.valid) {
            return core::Err(core::ErrorCode::NotSupported);
        }

        if constexpr (requires { rt::ClockSemanticTraits<P>::kKernelClockMap; }) {
            constexpr auto& clock_map = rt::ClockSemanticTraits<P>::kKernelClockMap;
            constexpr std::uint32_t kNotFound = 0xFFFF'FFFFu;
            const auto hw_value = detail::find_field_value(clock_map, source, kNotFound);
            if (hw_value == kNotFound) {
                return core::Err(core::ErrorCode::InvalidParameter);
            }
            return rt::modify_field(field, hw_value);
        } else {
            // Map not generated — write source enum value directly
            return rt::modify_field(field, static_cast<std::uint32_t>(source));
        }
    } else {
        return core::Err(core::ErrorCode::NotSupported);
    }
}

}  // namespace alloy::hal::clock
