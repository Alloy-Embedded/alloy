// test_clock_hal.cpp — Task 2.5 (add-clock-management-hal)
//
// Compile-time verification of the clock management HAL API.
// Host-only compile test — no hardware required.

#include "hal/clock/peripheral_frequency.hpp"
#include "hal/clock/kernel_clock.hpp"
#include "hal/clock/clock_profile.hpp"
#include "device/runtime.hpp"
#include "core/result.hpp"
#include "core/error_code.hpp"

using namespace alloy;
using namespace alloy::hal::clock;
using alloy::device::runtime::PeripheralId;

// Verify peripheral_frequency<P>() is callable and returns a Result
void test_peripheral_frequency() {
    // These calls compile regardless of whether the device has clock-tree data.
    // They may return NotSupported on devices without IR clock data.
    auto r1 = peripheral_frequency<PeripheralId::none>();
    (void)r1;

    // Verify return type is core::Result<uint32_t, core::ErrorCode>
    static_assert(
        std::is_same_v<decltype(peripheral_frequency<PeripheralId::none>()),
                       core::Result<std::uint32_t, core::ErrorCode>>
    );
}

// Verify set_kernel_clock<P>(KernelClockSource) is callable
void test_set_kernel_clock() {
    auto r2 = set_kernel_clock<PeripheralId::none>(KernelClockSource::pclk);
    (void)r2;

    static_assert(
        std::is_same_v<decltype(set_kernel_clock<PeripheralId::none>(KernelClockSource::pclk)),
                       core::Result<void, core::ErrorCode>>
    );
}

// Verify switch_to_default_profile() / switch_to_safe_profile() are callable
void test_profile_switch() {
    auto r3 = switch_to_default_profile();
    auto r4 = switch_to_safe_profile();
    (void)r3;
    (void)r4;

    static_assert(
        std::is_same_v<decltype(switch_to_default_profile()),
                       core::Result<void, core::ErrorCode>>
    );
}

// KernelClockSource enum members compile
void test_clock_source_enum() {
    constexpr auto src_pclk   = KernelClockSource::pclk;
    constexpr auto src_hsi16  = KernelClockSource::hsi16;
    constexpr auto src_lse    = KernelClockSource::lse;
    constexpr auto src_sysclk = KernelClockSource::sysclk;
    constexpr auto src_hse    = KernelClockSource::hse;
    (void)src_pclk; (void)src_hsi16; (void)src_lse;
    (void)src_sysclk; (void)src_hse;
}
