/// Host (PC) Clock Configuration Implementation
///
/// Implements a minimal clock interface for host platforms.
/// Since there's no real hardware to configure, this is mostly a stub
/// that returns reasonable default values for testing purposes.
///
/// Platform Support:
/// - Linux (POSIX)
/// - macOS
/// - Windows
///
/// Note: On host, clock configuration is managed by the OS.
/// This implementation exists only to satisfy the HAL interface.

#ifndef ALLOY_HAL_HOST_CLOCK_HPP
#define ALLOY_HAL_HOST_CLOCK_HPP

#include "hal/interface/clock.hpp"
#include "core/types.hpp"

namespace alloy::hal::host {

/// Host platform System Clock stub implementation
///
/// Returns simulated clock values matching typical embedded targets.
/// No actual hardware configuration occurs.
class SystemClock {
public:
    /// Default frequency: 168MHz (simulating STM32F4)
    static constexpr core::u32 DEFAULT_FREQUENCY = 168000000;

    SystemClock() : system_frequency_(DEFAULT_FREQUENCY) {}

    /// Configure system clock (stub - returns Ok)
    ///
    /// On host platforms, this does nothing as the OS manages clocks.
    /// Always succeeds and stores the requested frequency.
    ///
    /// @param config Clock configuration (stored but not applied)
    /// @return Always returns Ok
    core::Result<void> configure(const ClockConfig& config) {
        system_frequency_ = config.crystal_frequency_hz;
        return core::Result<void>::ok();
    }

    /// Set system frequency (stub)
    ///
    /// Stores the frequency for get_frequency() to return.
    ///
    /// @param frequency_hz Desired frequency in Hz
    /// @return Always returns Ok
    core::Result<void> set_frequency(core::u32 frequency_hz) {
        system_frequency_ = frequency_hz;
        return core::Result<void>::ok();
    }

    /// Get current system frequency
    ///
    /// Returns the stored frequency value (default: 168MHz).
    ///
    /// @return System frequency in Hz
    core::u32 get_frequency() const {
        return system_frequency_;
    }

    /// Get AHB frequency (same as system frequency on host)
    ///
    /// @return AHB frequency in Hz
    core::u32 get_ahb_frequency() const {
        return system_frequency_;
    }

    /// Get APB1 frequency (simulating typical /4 divider)
    ///
    /// @return APB1 frequency in Hz
    core::u32 get_apb1_frequency() const {
        return system_frequency_ / 4;  // Simulate 42MHz @ 168MHz
    }

    /// Get APB2 frequency (simulating typical /2 divider)
    ///
    /// @return APB2 frequency in Hz
    core::u32 get_apb2_frequency() const {
        return system_frequency_ / 2;  // Simulate 84MHz @ 168MHz
    }

    /// Get peripheral frequency (stub)
    ///
    /// Returns APB1 or APB2 frequency based on peripheral type.
    ///
    /// @param periph Peripheral identifier
    /// @return Peripheral clock frequency in Hz
    core::u32 get_peripheral_frequency(Peripheral periph) const {
        // Simulate APB1/APB2 split
        core::u16 periph_val = static_cast<core::u16>(periph);
        if (periph_val >= 0x0100 && periph_val < 0x0200) {
            return get_apb2_frequency();
        }
        return get_apb1_frequency();
    }

    /// Enable peripheral clock (stub - always succeeds)
    ///
    /// On host, there are no peripheral clocks to enable.
    ///
    /// @param periph Peripheral to enable
    /// @return Always returns Ok
    core::Result<void> enable_peripheral(Peripheral periph) {
        (void)periph;  // Unused
        return core::Result<void>::ok();
    }

    /// Disable peripheral clock (stub - always succeeds)
    ///
    /// On host, there are no peripheral clocks to disable.
    ///
    /// @param periph Peripheral to disable
    /// @return Always returns Ok
    core::Result<void> disable_peripheral(Peripheral periph) {
        (void)periph;  // Unused
        return core::Result<void>::ok();
    }

    /// Set flash latency (stub - always succeeds)
    ///
    /// On host, there's no flash to configure.
    ///
    /// @param frequency_hz Target frequency
    /// @return Always returns Ok
    core::Result<void> set_flash_latency(core::u32 frequency_hz) {
        (void)frequency_hz;  // Unused
        return core::Result<void>::ok();
    }

    /// Configure PLL (stub - not supported)
    ///
    /// @param config PLL configuration
    /// @return Always returns NotSupported
    core::Result<void> configure_pll(const PllConfig& config) {
        (void)config;  // Unused
        return core::Result<void>::error(core::ErrorCode::NotSupported);
    }

private:
    core::u32 system_frequency_;  ///< Simulated system frequency
};

// Static assertion to verify concept compliance
static_assert(hal::SystemClock<SystemClock>,
              "Host SystemClock must satisfy SystemClock concept");

} // namespace alloy::hal::host

#endif // ALLOY_HAL_HOST_CLOCK_HPP
