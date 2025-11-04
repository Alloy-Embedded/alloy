/// RP2040 SysTick Timer Implementation
///
/// Uses RP2040's hardware microsecond timer (64-bit at 1MHz).
/// This is the simplest implementation - hardware already provides
/// perfect 1MHz timer, we just read it directly.
///
/// Hardware Resources:
/// - Timer peripheral (base address 0x40054000)
/// - TIMELR register (lower 32 bits at 0x40054028)
///
/// Configuration:
/// - No configuration needed - timer always running at 1MHz
/// - No interrupts needed - direct hardware read
/// - Perfect 1us resolution
///
/// Memory Usage:
/// - 0 bytes RAM (no software counter needed)
/// - ~50 bytes code

#ifndef ALLOY_HAL_RP2040_SYSTICK_HPP
#define ALLOY_HAL_RP2040_SYSTICK_HPP

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::hal::raspberrypi::rp2040 {

/// RP2040 SysTick implementation using hardware timer
///
/// @note This is the ideal SysTick implementation - hardware does everything
class SystemTick {
public:
    /// Initialize SysTick timer
    ///
    /// On RP2040, the timer is always running, so this is basically a no-op.
    /// We just mark as initialized.
    ///
    /// @return Ok on success
    static core::Result<void> init() {
        initialized_ = true;
        return core::Result<void>::ok();
    }

    /// Get current time in microseconds
    ///
    /// Reads lower 32 bits of hardware 64-bit microsecond timer.
    /// Timer runs at 1MHz, so each tick is exactly 1 microsecond.
    ///
    /// Thread-safe: Yes (single memory load)
    ///
    /// @return Microseconds since boot
    static core::u32 micros() {
        // Read TIMELR register (lower 32 bits of 64-bit timer)
        // Address: 0x40054028
        constexpr core::u32 TIMER_TIMELR_ADDR = 0x40054028;
        return *reinterpret_cast<volatile core::u32*>(TIMER_TIMELR_ADDR);
    }

    /// Reset counter to zero
    ///
    /// On RP2040, we can write to TIMEHR/TIMELR to reset the timer.
    ///
    /// @return Ok on success
    static core::Result<void> reset() {
        constexpr core::u32 TIMER_TIMEHR_ADDR = 0x4005402C;
        constexpr core::u32 TIMER_TIMELR_ADDR = 0x40054028;

        // Write 0 to both high and low registers
        *reinterpret_cast<volatile core::u32*>(TIMER_TIMEHR_ADDR) = 0;
        *reinterpret_cast<volatile core::u32*>(TIMER_TIMELR_ADDR) = 0;

        return core::Result<void>::ok();
    }

    /// Check if initialized
    ///
    /// @return true if init() has been called
    static bool is_initialized() {
        return initialized_;
    }

private:
    /// Initialization flag
    static inline bool initialized_ = false;
};

// Validate concept compliance at compile time
static_assert(alloy::hal::SystemTick<SystemTick>,
              "RP2040 SystemTick must satisfy SystemTick concept");

} // namespace alloy::hal::raspberrypi::rp2040

/// Global namespace implementation for RP2040
namespace alloy::systick::detail {
    inline core::u32 get_micros() {
        return alloy::hal::raspberrypi::rp2040::SystemTick::micros();
    }
}

#endif // ALLOY_HAL_RP2040_SYSTICK_HPP
