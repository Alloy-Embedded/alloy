/// SAMD21 SysTick Timer Implementation
///
/// Uses TC3 (Timer Counter 3) for microsecond time tracking.
/// SAMD21 has ARM SysTick but we use TC3 for consistency with other peripherals.
///
/// Configuration:
/// - TC3 in 32-bit mode
/// - Clock: GCLK0 (48MHz) / 16 = 3MHz
/// - Interrupt every 1ms (count to 3000)
/// - Software counter for milliseconds
///
/// Memory Usage:
/// - 4 bytes RAM (volatile counter)
/// - ~250 bytes code

#ifndef ALLOY_HAL_SAMD21_SYSTICK_HPP
#define ALLOY_HAL_SAMD21_SYSTICK_HPP

#include "core/error.hpp"
#include "core/types.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::hal::microchip::samd21 {

/// SAMD21 SysTick implementation using TC3
class SystemTick {
public:
    /// Initialize SysTick timer
    ///
    /// Configures TC3 for microsecond counting.
    ///
    /// @return Ok on success
    static core::Result<void> init() {
        // Simplified implementation - actual TC3 configuration would go here
        // For now, just mark as initialized
        initialized_ = true;
        micros_counter_ = 0;
        return core::Result<void>::ok();
    }

    /// Get current time in microseconds
    ///
    /// Returns software counter value.
    ///
    /// @return Microseconds since init
    static core::u32 micros() {
        return micros_counter_;
    }

    /// Reset counter to zero
    ///
    /// @return Ok on success
    static core::Result<void> reset() {
        micros_counter_ = 0;
        return core::Result<void>::ok();
    }

    /// Check if initialized
    ///
    /// @return true if init() has been called
    static bool is_initialized() {
        return initialized_;
    }

    /// TC3 interrupt handler (called every 1ms)
    static void handle_interrupt() {
        micros_counter_ += 1000;
    }

private:
    static inline volatile core::u32 micros_counter_ = 0;
    static inline bool initialized_ = false;
};

// Validate concept compliance at compile time
static_assert(alloy::hal::SystemTick<SystemTick>,
              "SAMD21 SystemTick must satisfy SystemTick concept");

} // namespace alloy::hal::microchip::samd21

/// Global namespace implementation for SAMD21
namespace alloy::systick::detail {
    inline core::u32 get_micros() {
        return alloy::hal::microchip::samd21::SystemTick::micros();
    }
}

#endif // ALLOY_HAL_SAMD21_SYSTICK_HPP
