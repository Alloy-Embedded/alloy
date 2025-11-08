/// Host (PC) SysTick Timer Implementation
///
/// Uses std::chrono for microsecond time tracking on host platforms.
/// This allows testing embedded code on a PC without hardware.
///
/// Platform Support:
/// - Linux (POSIX)
/// - macOS
/// - Windows
///
/// Configuration:
/// - Uses std::chrono::steady_clock (monotonic, never goes backwards)
/// - Microsecond precision (or better, depending on platform)
/// - No interrupts (polling-based)
///
/// Memory Usage:
/// - ~24 bytes RAM (timepoint storage)
/// - ~300 bytes code

#ifndef ALLOY_HAL_HOST_SYSTICK_HPP
#define ALLOY_HAL_HOST_SYSTICK_HPP

#include <chrono>

#include "hal/interface/systick.hpp"

#include "core/error.hpp"
#include "core/types.hpp"

namespace alloy::hal::host {

/// Host platform SysTick implementation using std::chrono
///
/// @note This class is stateless - all state is in static variables
class SystemTick {
   public:
    /// Initialize SysTick timer
    ///
    /// Captures the start time using steady_clock.
    /// This clock is monotonic and never goes backwards.
    ///
    /// @return Ok on success
    static core::Result<void> init() {
        start_time_ = std::chrono::steady_clock::now();
        initialized_ = true;
        return core::Result<void>::ok();
    }

    /// Get current time in microseconds
    ///
    /// Returns microseconds elapsed since init().
    /// Uses std::chrono::steady_clock for monotonic time.
    ///
    /// Thread-safe: Yes (std::chrono is thread-safe)
    ///
    /// @return Microseconds since init
    static core::u32 micros() {
        if (!initialized_) {
            return 0;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_);

        // Convert to u32 (will overflow after ~71 minutes, same as hardware)
        return static_cast<core::u32>(elapsed.count());
    }

    /// Reset counter to zero
    ///
    /// Resets the start time to current time.
    ///
    /// @return Ok on success
    static core::Result<void> reset() {
        start_time_ = std::chrono::steady_clock::now();
        return core::Result<void>::ok();
    }

    /// Check if initialized
    ///
    /// @return true if init() has been called
    static bool is_initialized() { return initialized_; }

   private:
    /// Start time reference point
    static inline std::chrono::steady_clock::time_point start_time_{};

    /// Initialization flag
    static inline bool initialized_ = false;
};

// Validate concept compliance at compile time
static_assert(alloy::hal::SystemTick<SystemTick>,
              "Host SystemTick must satisfy SystemTick concept");

}  // namespace alloy::hal::host

/// Global namespace implementation for host
namespace alloy::systick::detail {
inline core::u32 get_micros() {
    return alloy::hal::host::SystemTick::micros();
}
}  // namespace alloy::systick::detail

#endif  // ALLOY_HAL_HOST_SYSTICK_HPP
