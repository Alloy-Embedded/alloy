/// Platform-agnostic Timer interface
///
/// Defines Timer concepts and configuration types for all platforms.

#ifndef ALLOY_HAL_INTERFACE_TIMER_HPP
#define ALLOY_HAL_INTERFACE_TIMER_HPP

#include <concepts>
#include <functional>

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

/// Timer operating mode
enum class TimerMode : core::u8 {
    OneShot,        ///< Fire once and stop
    Periodic,       ///< Fire repeatedly at fixed interval
    InputCapture,   ///< Capture timer value on external event
    OutputCompare,  ///< Trigger event when counter reaches compare value
    Counter         ///< Free-running counter mode
};

/// Input capture edge selection
enum class CaptureEdge : core::u8 {
    Rising,   ///< Capture on rising edge
    Falling,  ///< Capture on falling edge
    Both      ///< Capture on both edges
};

/// Timer configuration parameters
///
/// Contains all parameters needed to configure a timer peripheral.
struct TimerConfig {
    TimerMode mode;            ///< Timer operating mode
    core::u32 period_us;       ///< Period in microseconds (for periodic/one-shot)
    core::u32 prescaler;       ///< Clock prescaler value
    CaptureEdge capture_edge;  ///< Edge selection for input capture mode
    core::u32 compare_value;   ///< Compare value for output compare mode

    /// Constructor with default configuration (1ms periodic)
    constexpr TimerConfig(TimerMode m = TimerMode::Periodic, core::u32 period = 1000,
                          core::u32 presc = 1, CaptureEdge edge = CaptureEdge::Rising,
                          core::u32 compare = 0)
        : mode(m),
          period_us(period),
          prescaler(presc),
          capture_edge(edge),
          compare_value(compare) {}
};

/// Timer device concept
///
/// Defines the interface that all Timer implementations must satisfy.
/// Uses Result<T, ErrorCode> for all operations that can fail.
///
/// Error codes specific to Timer:
/// - ErrorCode::InvalidParameter: Invalid period, prescaler, or mode
/// - ErrorCode::NotSupported: Feature not supported by hardware
/// - ErrorCode::Busy: Timer already running
template <typename T>
concept TimerDevice =
    requires(T device, const T const_device, TimerConfig config, core::u32 counter_value,
             core::u32 period_us, std::function<void()> callback) {
        /// Start timer
        ///
        /// Begins timer operation according to configured mode.
        ///
        /// @return Ok on success, error code on failure
        { device.start() } -> std::same_as<core::Result<void>>;

        /// Stop timer
        ///
        /// Halts timer operation immediately.
        ///
        /// @return Ok on success, error code on failure
        { device.stop() } -> std::same_as<core::Result<void>>;

        /// Get current counter value
        ///
        /// Reads the current timer counter. Useful for measuring elapsed time.
        ///
        /// @return Current counter value in timer ticks
        { const_device.get_counter() } -> std::same_as<core::u32>;

        /// Set counter value
        ///
        /// Manually sets the timer counter value.
        ///
        /// @param value New counter value
        /// @return Ok on success, error code on failure
        { device.set_counter(counter_value) } -> std::same_as<core::Result<void>>;

        /// Set timer period
        ///
        /// Changes the timer period (interval between events).
        ///
        /// @param period_us Period in microseconds
        /// @return Ok on success, error code on failure
        { device.set_period(period_us) } -> std::same_as<core::Result<void>>;

        /// Set timer callback
        ///
        /// Sets function to be called on timer events (overflow, compare match, etc).
        /// Callback executes in interrupt context.
        ///
        /// @param callback Function to call on timer event
        /// @return Ok on success, error code on failure
        { device.set_callback(callback) } -> std::same_as<core::Result<void>>;

        /// Get captured value (input capture mode)
        ///
        /// Returns the timer value captured on the configured edge event.
        /// Useful for measuring frequency or pulse width.
        ///
        /// @return Captured timer value, or error code
        { const_device.get_captured_value() } -> std::same_as<core::Result<core::u32>>;

        /// Configure input capture mode
        ///
        /// Sets up timer to capture counter value on external pin events.
        ///
        /// @param edge Edge to trigger capture (rising, falling, or both)
        /// @return Ok on success, error code on failure
        { device.configure_capture(config.capture_edge) } -> std::same_as<core::Result<void>>;

        /// Configure output compare mode
        ///
        /// Sets up timer to trigger event when counter matches compare value.
        /// Useful for precise timing events.
        ///
        /// @param compare_value Value to compare against counter
        /// @return Ok on success, error code on failure
        { device.configure_compare(counter_value) } -> std::same_as<core::Result<void>>;

        /// Configure timer parameters
        ///
        /// @param config Timer configuration (mode, period, prescaler, etc)
        /// @return Ok on success, error code on failure
        { device.configure(config) } -> std::same_as<core::Result<void>>;
    };

/// Helper function to calculate prescaler for desired timer frequency
///
/// @param clock_hz Timer input clock frequency in Hz
/// @param desired_freq_hz Desired timer frequency in Hz
/// @return Prescaler value
inline constexpr core::u32 calculate_prescaler(core::u32 clock_hz, core::u32 desired_freq_hz) {
    return (clock_hz / desired_freq_hz) - 1;
}

/// Helper function to calculate period in timer ticks
///
/// @param clock_hz Timer clock frequency in Hz
/// @param period_us Desired period in microseconds
/// @return Period in timer ticks
inline constexpr core::u32 period_us_to_ticks(core::u32 clock_hz, core::u32 period_us) {
    return (clock_hz / 1000000u) * period_us;
}

/// Helper function to convert timer ticks to microseconds
///
/// @param ticks Timer ticks
/// @param clock_hz Timer clock frequency in Hz
/// @return Time in microseconds
inline constexpr core::u32 ticks_to_us(core::u32 ticks, core::u32 clock_hz) {
    return (ticks * 1000000u) / clock_hz;
}

/// Helper function to calculate frequency from captured timer values
///
/// Useful for input capture mode to measure signal frequency.
///
/// @param timer_clock_hz Timer clock frequency in Hz
/// @param captured_ticks Difference between two captures
/// @return Measured frequency in Hz
inline constexpr core::u32 calculate_frequency(core::u32 timer_clock_hz, core::u32 captured_ticks) {
    if (captured_ticks == 0)
        return 0;
    return timer_clock_hz / captured_ticks;
}

}  // namespace alloy::hal

#endif  // ALLOY_HAL_INTERFACE_TIMER_HPP
