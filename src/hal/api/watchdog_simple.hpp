/**
 * @file watchdog_simple.hpp
 * @brief Level 1 Simple API for Watchdog Timer
 *
 * Provides one-liner operations for watchdog timers.
 * Platform-agnostic API that uses Hardware Policies.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Simple Watchdog API
 *
 * Platform-agnostic watchdog control using Hardware Policies.
 * The WatchdogPolicy template parameter handles platform-specific details.
 *
 * Example usage:
 * @code
 * // Disable watchdog using platform-specific policy
 * Watchdog::disable<WatchdogPolicy>();
 *
 * // Feed/kick watchdog
 * Watchdog::feed<WatchdogPolicy>();
 *
 * // Check if enabled
 * if (Watchdog::is_enabled<WatchdogPolicy>()) {
 *     // Watchdog is running
 * }
 * @endcode
 */
class Watchdog {
public:
    /**
     * @brief Disable watchdog timer
     *
     * Uses the platform's Hardware Policy to disable the watchdog.
     *
     * @tparam WatchdogPolicy Platform-specific watchdog hardware policy
     */
    template <typename WatchdogPolicy>
    static void disable() {
        WatchdogPolicy::disable();
    }

    /**
     * @brief Enable watchdog timer with default timeout
     *
     * @tparam WatchdogPolicy Platform-specific watchdog hardware policy
     */
    template <typename WatchdogPolicy>
    static void enable() {
        WatchdogPolicy::enable();
    }

    /**
     * @brief Enable watchdog with custom timeout
     *
     * @tparam WatchdogPolicy Platform-specific watchdog hardware policy
     * @param timeout_ms Timeout in milliseconds
     */
    template <typename WatchdogPolicy>
    static void enable_with_timeout(u16 timeout_ms) {
        WatchdogPolicy::enable_with_timeout(timeout_ms);
    }

    /**
     * @brief Feed/kick the watchdog (reset the timer)
     *
     * Call this periodically to prevent watchdog reset.
     *
     * @tparam WatchdogPolicy Platform-specific watchdog hardware policy
     */
    template <typename WatchdogPolicy>
    static void feed() {
        WatchdogPolicy::feed();
    }

    /**
     * @brief Check if watchdog is enabled
     *
     * @tparam WatchdogPolicy Platform-specific watchdog hardware policy
     * @return true if enabled, false if disabled
     */
    template <typename WatchdogPolicy>
    static bool is_enabled() {
        return WatchdogPolicy::is_enabled();
    }

    /**
     * @brief Get remaining time before watchdog timeout
     *
     * @tparam WatchdogPolicy Platform-specific watchdog hardware policy
     * @return Remaining time in milliseconds (0 if not supported)
     */
    template <typename WatchdogPolicy>
    static u16 get_remaining_time_ms() {
        return WatchdogPolicy::get_remaining_time_ms();
    }
};

}  // namespace alloy::hal
