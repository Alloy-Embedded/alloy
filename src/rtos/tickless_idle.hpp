/// Alloy RTOS - Tickless Idle Mode
///
/// Power management hooks for tickless idle operation.
///
/// Features:
/// - Automatic sleep when idle
/// - Precise wake-up timing
/// - Low-power mode integration
/// - Minimal wake-up latency
/// - C++23 consteval validation
///
/// Power savings:
/// - Idle current: <1µA (in STOP mode)
/// - Active current: ~10mA (running)
/// - Wake-up time: <10µs
///
/// Use cases:
/// - Battery-powered devices
/// - Energy harvesting applications
/// - Always-on IoT sensors
/// - Low-duty-cycle systems
///
/// Usage:
/// ```cpp
/// // Enable tickless idle
/// TicklessIdle::enable();
///
/// // Configure sleep mode
/// TicklessIdle::configure(SleepMode::Stop, 1000);  // Min 1ms sleep
///
/// // RTOS will automatically enter low-power mode when idle
/// RTOS::start();
/// ```

#ifndef ALLOY_RTOS_TICKLESS_IDLE_HPP
#define ALLOY_RTOS_TICKLESS_IDLE_HPP

#include <cstddef>

#include "rtos/error.hpp"
#include "core/types.hpp"
#include "core/result.hpp"

namespace alloy::rtos {

// ============================================================================
// Tickless Idle Configuration
// ============================================================================

/// Sleep mode selection
enum class SleepMode : core::u8 {
    /// Light sleep (WFI) - fastest wake-up (<1µs)
    /// CPU stopped, peripherals running
    /// Current: ~1-5mA
    Light = 0,

    /// Deep sleep (STOP mode) - moderate wake-up (~10µs)
    /// CPU + most peripherals stopped
    /// Current: ~10-100µA
    Deep = 1,

    /// Standby mode - slowest wake-up (~100µs)
    /// Only RTC and wake-up pins active
    /// Current: <1µA
    Standby = 2,
};

/// Tickless idle configuration
struct TicklessConfig {
    /// Sleep mode to use
    SleepMode mode{SleepMode::Light};

    /// Minimum sleep duration (µs)
    /// Don't sleep if next wake-up is sooner
    core::u32 min_sleep_duration_us{1000};  // 1ms default

    /// Maximum sleep duration (µs)
    /// Limit to prevent missed wake-ups
    core::u32 max_sleep_duration_us{1000000};  // 1s default

    /// Wake-up latency compensation (µs)
    /// Account for time to wake up from sleep
    core::u32 wakeup_latency_us{10};  // 10µs default

    /// Enable/disable tickless idle
    bool enabled{false};
};

// ============================================================================
// Tickless Idle Hooks (User-Implemented)
// ============================================================================

/// User hook: Enter low-power mode
///
/// Called by RTOS when going to sleep.
/// User must implement this function for their platform.
///
/// @param mode Sleep mode
/// @param duration_us Expected sleep duration in microseconds
///
/// Example (STM32):
/// ```cpp
/// extern "C" void tickless_enter_sleep(SleepMode mode, u32 duration_us) {
///     // Configure wake-up timer
///     RTC_SetWakeUpTimer(duration_us);
///
///     // Enter sleep mode
///     switch (mode) {
///         case SleepMode::Light:
///             HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
///             break;
///         case SleepMode::Deep:
///             HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
///             SystemClock_Config();  // Restore clocks
///             break;
///         case SleepMode::Standby:
///             HAL_PWR_EnterSTANDBYMode();
///             break;
///     }
/// }
/// ```
extern "C" void tickless_enter_sleep(SleepMode mode, core::u32 duration_us) __attribute__((weak));

/// User hook: Exit low-power mode
///
/// Called by RTOS after waking from sleep.
/// User may implement this to restore system state.
///
/// @param actual_sleep_us Actual sleep duration in microseconds
///
/// Example:
/// ```cpp
/// extern "C" void tickless_exit_sleep(u32 actual_sleep_us) {
///     // Restore system state if needed
///     // Update tick count to account for sleep time
/// }
/// ```
extern "C" void tickless_exit_sleep(core::u32 actual_sleep_us) __attribute__((weak));

/// User hook: Calculate next wake-up time
///
/// Called by RTOS to determine how long to sleep.
/// Default implementation checks task wake times.
///
/// @return Microseconds until next wake-up event
extern "C" core::u32 tickless_get_next_wakeup_us() __attribute__((weak));

// ============================================================================
// Tickless Idle API
// ============================================================================

/// Tickless Idle Manager
///
/// Manages automatic low-power mode entry when RTOS is idle.
///
/// Example:
/// ```cpp
/// // Enable tickless idle with deep sleep
/// TicklessIdle::enable();
/// TicklessIdle::configure(SleepMode::Deep, 5000);  // Min 5ms sleep
///
/// // RTOS will automatically sleep when idle
/// RTOS::start();
/// ```
class TicklessIdle {
public:
    /// Enable tickless idle mode
    ///
    /// @return Ok(void) on success
    [[nodiscard]] static core::Result<void, RTOSError> enable() noexcept;

    /// Disable tickless idle mode
    ///
    /// @return Ok(void) on success
    [[nodiscard]] static core::Result<void, RTOSError> disable() noexcept;

    /// Check if tickless idle is enabled
    ///
    /// @return true if enabled
    [[nodiscard]] static bool is_enabled() noexcept;

    /// Configure tickless idle parameters
    ///
    /// @param mode Sleep mode
    /// @param min_sleep_us Minimum sleep duration (microseconds)
    /// @return Ok(void) on success
    ///
    /// Example:
    /// ```cpp
    /// // Configure for deep sleep, minimum 10ms
    /// TicklessIdle::configure(SleepMode::Deep, 10000);
    /// ```
    [[nodiscard]] static core::Result<void, RTOSError> configure(
        SleepMode mode,
        core::u32 min_sleep_us
    ) noexcept;

    /// Set maximum sleep duration
    ///
    /// @param max_sleep_us Maximum sleep duration (microseconds)
    /// @return Ok(void) on success
    [[nodiscard]] static core::Result<void, RTOSError> set_max_sleep_duration(
        core::u32 max_sleep_us
    ) noexcept;

    /// Set wake-up latency compensation
    ///
    /// @param latency_us Wake-up latency in microseconds
    /// @return Ok(void) on success
    [[nodiscard]] static core::Result<void, RTOSError> set_wakeup_latency(
        core::u32 latency_us
    ) noexcept;

    /// Get current configuration
    ///
    /// @return Current tickless configuration
    [[nodiscard]] static const TicklessConfig& get_config() noexcept;

    /// Called by RTOS idle task to check if should sleep
    ///
    /// @note Internal function, do not call directly
    /// @return true if should enter sleep
    [[nodiscard]] static bool should_sleep() noexcept;

    /// Enter tickless idle sleep
    ///
    /// @note Internal function, called by idle task
    /// @return Actual sleep duration in microseconds
    [[nodiscard]] static core::u32 enter_sleep() noexcept;

private:
    static TicklessConfig config_;
};

// ============================================================================
// Power Management Statistics
// ============================================================================

/// Power management statistics
struct PowerStats {
    core::u32 total_sleep_time_us{0};    ///< Total time spent sleeping
    core::u32 total_active_time_us{0};   ///< Total time spent active
    core::u32 sleep_count{0};             ///< Number of sleep events
    core::u32 wakeup_count{0};            ///< Number of wake-ups
    core::u32 min_sleep_duration_us{0};   ///< Shortest sleep
    core::u32 max_sleep_duration_us{0};   ///< Longest sleep
    core::u32 avg_sleep_duration_us{0};   ///< Average sleep duration

    /// Calculate power efficiency (% time sleeping)
    [[nodiscard]] constexpr core::u8 efficiency_percent() const noexcept {
        core::u32 total = total_sleep_time_us + total_active_time_us;
        if (total == 0) return 0;
        return static_cast<core::u8>((total_sleep_time_us * 100) / total);
    }

    /// Reset statistics
    constexpr void reset() noexcept {
        *this = PowerStats{};
    }
};

/// Get power management statistics
///
/// @return Power statistics
[[nodiscard]] PowerStats& get_power_stats() noexcept;

// ============================================================================
// C++23 Compile-Time Validation
// ============================================================================

/// Validate sleep mode
///
/// @tparam Mode Sleep mode
/// @return true if valid
template <SleepMode Mode>
consteval bool is_valid_sleep_mode() {
    return Mode >= SleepMode::Light && Mode <= SleepMode::Standby;
}

/// Validate sleep duration
///
/// @tparam DurationUs Duration in microseconds
/// @return true if valid
template <core::u32 DurationUs>
consteval bool is_valid_sleep_duration() {
    return DurationUs >= 100 &&        // Min 100µs
           DurationUs <= 10000000;     // Max 10s
}

/// Calculate power savings estimate
///
/// @tparam IdlePercent Percentage of time idle (0-100)
/// @tparam ActiveCurrentMa Active current in mA
/// @tparam SleepCurrentUa Sleep current in µA
/// @return Power savings in µW
template <core::u8 IdlePercent, core::u32 ActiveCurrentMa, core::u32 SleepCurrentUa>
consteval core::u32 estimated_power_savings_uw() {
    static_assert(IdlePercent <= 100, "Idle percentage must be 0-100");

    // Assume 3.3V supply
    constexpr core::u32 voltage_mv = 3300;

    // Active power (µW)
    constexpr core::u32 active_power = ActiveCurrentMa * voltage_mv;

    // Sleep power (µW)
    constexpr core::u32 sleep_power = (SleepCurrentUa * voltage_mv) / 1000;

    // Average power without tickless idle (µW)
    constexpr core::u32 without_tickless = active_power;

    // Average power with tickless idle (µW)
    constexpr core::u32 with_tickless =
        ((100 - IdlePercent) * active_power + IdlePercent * sleep_power) / 100;

    // Power savings (µW)
    return without_tickless - with_tickless;
}

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_TICKLESS_IDLE_HPP
