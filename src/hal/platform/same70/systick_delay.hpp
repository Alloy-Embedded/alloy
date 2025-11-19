/// SAME70 SysTick Delay Functions
///
/// Provides precise delay functions using SysTick timer.
/// These functions use the configured SysTick for accurate timing
/// independent of clock speed changes.
///
/// Features:
/// - Microsecond and millisecond delays
/// - Overflow-safe timing
/// - Non-blocking timeout checks
/// - Runtime monitoring functions
///
/// Requirements:
/// - Clock::initialize() must be called first
/// - SystemTick::init() must be called before using delays
///
/// Example:
/// @code
/// #include "hal/platform/same70/systick_delay.hpp"
///
/// // Blocking delays
/// delay_us(100);   // 100 microsecond delay
/// delay_ms(10);    // 10 millisecond delay
///
/// // Non-blocking timeout
/// uint32_t start = micros();
/// while (!data_ready()) {
///     if (is_timeout(start, 1000)) {  // 1ms timeout
///         return Err(ErrorCode::Timeout);
///     }
/// }
///
/// // Runtime monitoring
/// uint32_t uptime_ms = get_uptime_ms();
/// uint32_t uptime_sec = get_uptime_seconds();
/// @endcode

#ifndef ALLOY_HAL_SAME70_SYSTICK_DELAY_HPP
#define ALLOY_HAL_SAME70_SYSTICK_DELAY_HPP

#include "hal/platform/same70/systick.hpp"

#include "core/types.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

/// Delay for specified microseconds
///
/// Blocking delay with microsecond precision.
/// Uses SysTick timer for accurate timing.
///
/// @param us Microseconds to delay
///
/// @note Requires SystemTick::init() to be called first
/// @note For delays > 1ms, prefer delay_ms() for clarity
inline void delay_us(u32 us) {
    u32 start = SystemTick::micros();
    while ((SystemTick::micros() - start) < us) {
        // Busy wait - unsigned arithmetic handles overflow
    }
}

/// Delay for specified milliseconds
///
/// Blocking delay with millisecond precision.
/// Implemented using delay_us() for consistency.
///
/// @param ms Milliseconds to delay
///
/// @note Requires SystemTick::init() to be called first
inline void delay_ms(u32 ms) {
    delay_us(ms * 1000);
}

/// Get current time in microseconds
///
/// Returns the current SysTick microsecond counter.
/// Counter overflows after ~71 minutes.
///
/// @return Microseconds since SystemTick initialization
inline u32 micros() {
    return SystemTick::micros();
}

/// Get current time in milliseconds
///
/// Returns milliseconds derived from microsecond counter.
/// Counter overflows after ~49 days.
///
/// @return Milliseconds since SystemTick initialization
inline u32 millis() {
    return SystemTick::micros() / 1000;
}

/// Calculate elapsed microseconds from start time
///
/// Handles counter overflow correctly using unsigned arithmetic.
///
/// @param start_us Start time from micros()
/// @return Elapsed microseconds
///
/// Example:
/// @code
/// uint32_t start = micros();
/// do_work();
/// uint32_t elapsed = elapsed_us(start);
/// @endcode
inline u32 elapsed_us(u32 start_us) {
    // Unsigned arithmetic handles overflow correctly
    return SystemTick::micros() - start_us;
}

/// Calculate elapsed milliseconds from start time
///
/// Handles counter overflow correctly.
///
/// @param start_ms Start time from millis()
/// @return Elapsed milliseconds
inline u32 elapsed_ms(u32 start_ms) {
    return millis() - start_ms;
}

/// Check if timeout has occurred
///
/// Correctly handles counter overflow.
/// Use for implementing timeouts.
///
/// @param start_us Start time from micros()
/// @param timeout_us Timeout duration in microseconds
/// @return true if timeout has expired
///
/// Example:
/// @code
/// uint32_t start = micros();
/// while (!data_ready()) {
///     if (is_timeout(start, 1000)) {  // 1ms timeout
///         return Err(ErrorCode::Timeout);
///     }
/// }
/// @endcode
inline bool is_timeout(u32 start_us, u32 timeout_us) {
    return elapsed_us(start_us) >= timeout_us;
}

/// Check if timeout has occurred (millisecond version)
///
/// @param start_ms Start time from millis()
/// @param timeout_ms Timeout duration in milliseconds
/// @return true if timeout has expired
inline bool is_timeout_ms(u32 start_ms, u32 timeout_ms) {
    return elapsed_ms(start_ms) >= timeout_ms;
}

/// Get system uptime in milliseconds
///
/// Returns total time since SystemTick initialization.
/// Overflows after ~49 days.
///
/// @return Uptime in milliseconds
inline u32 get_uptime_ms() {
    return millis();
}

/// Get system uptime in seconds
///
/// Returns total time since SystemTick initialization.
/// Overflows after ~136 years.
///
/// @return Uptime in seconds
inline u32 get_uptime_seconds() {
    return SystemTick::micros() / 1000000;
}

/// Get system uptime in minutes
///
/// Returns total time since SystemTick initialization.
/// Overflows after ~8000 years.
///
/// @return Uptime in minutes
inline u32 get_uptime_minutes() {
    return get_uptime_seconds() / 60;
}

/// Get system uptime in hours
///
/// Returns total time since SystemTick initialization.
///
/// @return Uptime in hours
inline u32 get_uptime_hours() {
    return get_uptime_minutes() / 60;
}

}  // namespace alloy::hal::same70

// Export to alloy namespace for convenience
namespace alloy {
using alloy::hal::same70::delay_ms;
using alloy::hal::same70::delay_us;
using alloy::hal::same70::elapsed_ms;
using alloy::hal::same70::elapsed_us;
using alloy::hal::same70::get_uptime_hours;
using alloy::hal::same70::get_uptime_minutes;
using alloy::hal::same70::get_uptime_ms;
using alloy::hal::same70::get_uptime_seconds;
using alloy::hal::same70::is_timeout;
using alloy::hal::same70::is_timeout_ms;
using alloy::hal::same70::micros;
using alloy::hal::same70::millis;
}  // namespace alloy

#endif  // ALLOY_HAL_SAME70_SYSTICK_DELAY_HPP
