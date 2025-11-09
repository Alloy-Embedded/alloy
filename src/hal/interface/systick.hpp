/// Platform-agnostic SysTick Timer interface
///
/// Provides global microsecond time tracking for all platforms.
/// This serves as the foundation for RTOS scheduling and timing operations.

#ifndef ALLOY_HAL_INTERFACE_SYSTICK_HPP
#define ALLOY_HAL_INTERFACE_SYSTICK_HPP

#include <concepts>

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

/// SysTick configuration parameters
///
/// Reserved for future use. Currently SysTick auto-configures
/// during Board::initialize() with platform-specific defaults.
struct SysTickConfig {
    core::u32 tick_frequency_hz;  ///< Desired tick frequency (typically 1MHz for 1us resolution)

    /// Constructor with default 1MHz (microsecond precision)
    constexpr SysTickConfig(core::u32 freq_hz = 1000000) : tick_frequency_hz(freq_hz) {}
};

/// SystemTick device concept
///
/// Defines the interface that all SysTick implementations must satisfy.
/// Provides microsecond-precision time tracking with 32-bit counter.
///
/// Counter overflow:
/// - 32-bit microsecond counter overflows after ~71 minutes
/// - Use micros_since() helper to handle overflow correctly
///
/// Thread safety:
/// - micros() reads are atomic on ARM Cortex-M (single LDR instruction)
/// - Other platforms use critical sections as needed
///
/// Example usage:
/// @code
/// // Get current time
/// uint32_t now = alloy::systick::micros();
///
/// // Measure elapsed time
/// uint32_t start = alloy::systick::micros();
/// do_work();
/// uint32_t elapsed = alloy::systick::micros_since(start);
///
/// // Check for timeout
/// if (alloy::systick::is_timeout(start, 1000)) {
///     // 1ms timeout expired
/// }
/// @endcode
template <typename T>
concept SystemTick = requires(T device, const T const_device) {
    /// Initialize SysTick timer
    ///
    /// Configures hardware timer for microsecond counting.
    /// Called automatically during Board::initialize().
    ///
    /// @return Ok on success, error code on failure
    { T::init() } -> std::convertible_to<core::Result<void, core::ErrorCode>>;

    /// Get current time in microseconds
    ///
    /// Returns monotonically increasing 32-bit counter.
    /// Overflows after ~71 minutes.
    ///
    /// Thread-safe: Yes (atomic read on ARM, critical section on others)
    ///
    /// @return Current time in microseconds since init
    { T::micros() } -> std::convertible_to<core::u32>;

    /// Reset counter to zero
    ///
    /// Resets the microsecond counter. Useful for testing or
    /// when overflow needs to be managed explicitly.
    ///
    /// @return Ok on success, error code on failure
    { T::reset() } -> std::convertible_to<core::Result<void, core::ErrorCode>>;

    /// Check if SysTick is initialized
    ///
    /// @return true if init() has been called successfully
    { T::is_initialized() } -> std::convertible_to<bool>;
};

}  // namespace alloy::hal

/// Global namespace API for convenient access
///
/// These functions delegate to the platform-specific SystemTick implementation.
/// Recommended for most users due to simplicity.
///
/// Example:
/// @code
/// #include "hal/interface/systick.hpp"
///
/// uint32_t start = alloy::systick::micros();
/// delay(10);
/// uint32_t elapsed = alloy::systick::micros_since(start);
/// @endcode
namespace alloy::systick {

// Forward declaration - implemented by platform-specific code
namespace detail {
core::u32 get_micros();
}

/// Get current time in microseconds
///
/// @return Microseconds since SysTick initialization
inline core::u32 micros() {
    return detail::get_micros();
}

/// Calculate elapsed time handling overflow
///
/// Correctly handles 32-bit counter wraparound using unsigned arithmetic.
///
/// @param start_time Starting time from micros()
/// @return Elapsed microseconds (handles overflow correctly)
///
/// Example:
/// @code
/// uint32_t start = alloy::systick::micros();
/// // ... time passes, possibly with overflow ...
/// uint32_t elapsed = alloy::systick::micros_since(start);
/// @endcode
inline core::u32 micros_since(core::u32 start_time) {
    // Unsigned arithmetic handles wraparound correctly
    // Example: start=0xFFFFFF00, now=0x00000100
    // elapsed = 0x00000100 - 0xFFFFFF00 = 0x00000200 (512us) ✓
    return micros() - start_time;
}

/// Check if timeout has occurred
///
/// Correctly handles overflow. Use for implementing timeouts.
///
/// @param start_time Starting time from micros()
/// @param timeout_us Timeout duration in microseconds
/// @return true if timeout has expired
///
/// Example:
/// @code
/// uint32_t start = alloy::systick::micros();
/// while (!alloy::systick::is_timeout(start, 1000)) {
///     // Wait up to 1ms
///     if (data_ready()) break;
/// }
/// @endcode
inline bool is_timeout(core::u32 start_time, core::u32 timeout_us) {
    return micros_since(start_time) >= timeout_us;
}

/// Delay for specified microseconds
///
/// Busy-wait delay using SysTick. For precise short delays.
/// For longer delays, prefer platform delay_ms() or RTOS sleep.
///
/// @param delay_us Microseconds to delay
///
/// Example:
/// @code
/// alloy::systick::delay_us(100);  // 100us delay
/// @endcode
inline void delay_us(core::u32 delay_us) {
    core::u32 start = micros();
    while (!is_timeout(start, delay_us)) {
        // Busy wait
    }
}

}  // namespace alloy::systick

#endif  // ALLOY_HAL_INTERFACE_SYSTICK_HPP
