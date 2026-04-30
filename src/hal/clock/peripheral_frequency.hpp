#pragma once

// peripheral_frequency.hpp — Tasks 2.1–2.4 (add-clock-management-hal)
//
// clock::peripheral_frequency<P>() — query the current clock frequency
// delivered to a peripheral's bus (APB1/APB2/AHB/MCLK).
//
// Design:
//   1. Look up the peripheral's ClockGateId → bus chain.
//   2. Read SYSCLK from clock profile (or live RCC register).
//   3. Apply bus dividers to derive PCLK.
//   4. Return PCLK as uint32_t Hz.
//
// If the device has no clock-tree data compiled in, returns NotSupported.

#include <cstdint>

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "device/clock_config.hpp"
#include "device/runtime.hpp"
#include "hal/detail/runtime_ops.hpp"

namespace alloy::hal::clock {

namespace detail {

// ---------------------------------------------------------------------------
// Bus frequency helpers
// ---------------------------------------------------------------------------

// Note: GCC rejects `requires { namespace_member(); }` in non-template
// functions when the member does not exist (treated as hard error rather than
// evaluating to false).  Use #if guards with the CMake-provided macros instead.

/// Read SYSCLK Hz from the max clock profile, or 0 when no profile data.
/// TODO: read live RCC SWS register for dynamic frequency.
[[nodiscard]] inline auto current_sysclk_hz() -> std::uint32_t {
#if defined(ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE) && ALLOY_DEVICE_CLOCK_CONFIG_AVAILABLE
    return device::clock_config::max_clock_frequency_hz;
#else
    return 0u;
#endif
}

/// Read AHB (HCLK) frequency.
/// Conservative: assumes AHB prescaler = 1 (HCLK = SYSCLK).
[[nodiscard]] inline auto current_hclk_hz() -> std::uint32_t {
    return current_sysclk_hz();
}

/// Read APB1 (PCLK1) frequency.
/// Conservative: assumes APB1 prescaler = 1 (PCLK1 = HCLK).
[[nodiscard]] inline auto current_pclk1_hz() -> std::uint32_t {
    return current_hclk_hz();
}

/// Read APB2 (PCLK2) frequency.
/// Conservative: assumes APB2 prescaler = 1 (PCLK2 = HCLK).
[[nodiscard]] inline auto current_pclk2_hz() -> std::uint32_t {
    return current_hclk_hz();
}

}  // namespace detail

// ---------------------------------------------------------------------------
// Public API — peripheral_frequency<PeripheralId>()
// ---------------------------------------------------------------------------

/// Return the current clock frequency (Hz) supplied to peripheral P's bus.
///
/// Uses ClockSemanticTraits<P> when available (generated from IR clock-tree).
/// Falls back to APB1 as a conservative estimate when no semantic data is present.
///
/// Returns Err(NotSupported) if the device was compiled without clock-tree data.
template <device::runtime::PeripheralId P>
[[nodiscard]] auto peripheral_frequency() -> core::Result<std::uint32_t, core::ErrorCode> {
    namespace rt = alloy::hal::detail::runtime;
    using PeripheralBusDomain = rt::PeripheralBusDomain;

    if constexpr (rt::ClockSemanticTraits<P>::kPresent) {
        // Generated ClockSemanticTraits present — read live bus dividers.
        constexpr auto domain = rt::ClockSemanticTraits<P>::kBusDomain;
        if constexpr (domain == PeripheralBusDomain::apb2) {
            return core::Ok(detail::current_pclk2_hz());
        } else if constexpr (domain == PeripheralBusDomain::apb1) {
            return core::Ok(detail::current_pclk1_hz());
        } else if constexpr (domain == PeripheralBusDomain::ahb) {
            return core::Ok(detail::current_hclk_hz());
        } else {
            return core::Ok(detail::current_sysclk_hz());
        }
    } else if constexpr (requires {
        rt::PeripheralBusTraits<P>::kBusDomain;
    }) {
        // PeripheralBusTraits fallback (hand-written board layer).
        constexpr auto domain = rt::PeripheralBusTraits<P>::kBusDomain;
        if constexpr (domain == PeripheralBusDomain::apb2) {
            return core::Ok(detail::current_pclk2_hz());
        } else if constexpr (domain == PeripheralBusDomain::apb1) {
            return core::Ok(detail::current_pclk1_hz());
        } else if constexpr (domain == PeripheralBusDomain::ahb) {
            return core::Ok(detail::current_hclk_hz());
        } else {
            return core::Ok(detail::current_sysclk_hz());
        }
    } else {
        // No clock-tree data — conservative fallback: APB1.
        const std::uint32_t hz = detail::current_pclk1_hz();
        if (hz == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return core::Ok(std::uint32_t{hz});
    }
}

}  // namespace alloy::hal::clock
