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

/// Read SYSCLK Hz from the first active clock profile, or from a fallback.
[[nodiscard]] inline auto current_sysclk_hz() -> std::uint32_t {
    if constexpr (requires { device::current_system_clock_hz(); }) {
        return device::current_system_clock_hz();
    } else if constexpr (requires { device::ClockProfileTraits<device::default_clock_profile_id>::kSysclkHz; }) {
        return device::ClockProfileTraits<device::default_clock_profile_id>::kSysclkHz;
    } else {
        return 0u;
    }
}

/// Read AHB (HCLK) frequency.
[[nodiscard]] inline auto current_hclk_hz() -> std::uint32_t {
    if constexpr (requires { device::current_ahb_clock_hz(); }) {
        return device::current_ahb_clock_hz();
    } else {
        return current_sysclk_hz();  // AHB divider = 1 (safe fallback)
    }
}

/// Read APB1 (PCLK1) frequency.
[[nodiscard]] inline auto current_pclk1_hz() -> std::uint32_t {
    if constexpr (requires { device::current_apb1_clock_hz(); }) {
        return device::current_apb1_clock_hz();
    } else {
        return current_hclk_hz();  // APB1 divider = 1 (safe fallback)
    }
}

/// Read APB2 (PCLK2) frequency.
[[nodiscard]] inline auto current_pclk2_hz() -> std::uint32_t {
    if constexpr (requires { device::current_apb2_clock_hz(); }) {
        return device::current_apb2_clock_hz();
    } else {
        return current_hclk_hz();
    }
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
    using rt = alloy::hal::detail::runtime;

    if constexpr (requires {
        rt::ClockSemanticTraits<P>::kBusHz;
    }) {
        // Generated ClockSemanticTraits present — return compile-time bus Hz
        constexpr auto hz = rt::ClockSemanticTraits<P>::kBusHz;
        if (hz == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return core::Ok(hz);
    } else if constexpr (requires {
        rt::PeripheralBusTraits<P>::kBusDomain;
    }) {
        // Peripheral bus domain known — read live dividers
        constexpr auto domain = rt::PeripheralBusTraits<P>::kBusDomain;
        using BusDomain = decltype(domain);
        if constexpr (domain == BusDomain::apb2) {
            return core::Ok(detail::current_pclk2_hz());
        } else if constexpr (domain == BusDomain::apb1) {
            return core::Ok(detail::current_pclk1_hz());
        } else if constexpr (domain == BusDomain::ahb) {
            return core::Ok(detail::current_hclk_hz());
        } else {
            return core::Ok(detail::current_sysclk_hz());
        }
    } else {
        // No clock-tree data — conservative fallback: APB1
        const auto hz = detail::current_pclk1_hz();
        if (hz == 0u) {
            return core::Err(core::ErrorCode::NotSupported);
        }
        return core::Ok(hz);
    }
}

// ---------------------------------------------------------------------------
// Task 3.1 — set_kernel_clock<P>(KernelClockSource)
// ---------------------------------------------------------------------------

/// Set the kernel clock MUX for peripheral P.
/// Writes the selector value to the RCC kernel-clock field for P.
/// No-op (returns NotSupported) if P has no kernel clock selector.
template <device::runtime::PeripheralId P>
[[nodiscard]] auto set_kernel_clock(KernelClockSource) -> core::Result<void, core::ErrorCode> {
    // Forward declaration — actual implementation in kernel_clock.hpp
    // This overload is the "not supported" fallback.
    return core::Err(core::ErrorCode::NotSupported);
}

}  // namespace alloy::hal::clock
