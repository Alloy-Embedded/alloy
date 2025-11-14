/**
 * @file watchdog_fluent.hpp
 * @brief Level 2 Fluent API for Watchdog Timer
 *
 * Provides chainable builder pattern for watchdog configuration.
 * Platform-agnostic API using Hardware Policies.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/api/watchdog_simple.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Fluent watchdog configuration builder
 *
 * Platform-agnostic watchdog configuration.
 *
 * Example:
 * @code
 * WatchdogBuilder<WatchdogPolicy>()
 *     .with_timeout(1000)  // 1 second
 *     .enable_reset()
 *     .apply();
 * @endcode
 */
template <typename WatchdogPolicy>
class WatchdogBuilder {
public:
    constexpr WatchdogBuilder()
        : timeout_ms_(1000), enable_reset_(true), enable_interrupt_(false) {}

    /**
     * @brief Set watchdog timeout
     */
    constexpr WatchdogBuilder& with_timeout(u16 timeout_ms) {
        timeout_ms_ = timeout_ms;
        return *this;
    }

    /**
     * @brief Enable system reset on timeout
     */
    constexpr WatchdogBuilder& enable_reset() {
        enable_reset_ = true;
        return *this;
    }

    /**
     * @brief Disable system reset on timeout
     */
    constexpr WatchdogBuilder& disable_reset() {
        enable_reset_ = false;
        return *this;
    }

    /**
     * @brief Enable interrupt on timeout
     */
    constexpr WatchdogBuilder& enable_interrupt() {
        enable_interrupt_ = true;
        return *this;
    }

    /**
     * @brief Disable interrupt on timeout
     */
    constexpr WatchdogBuilder& disable_interrupt() {
        enable_interrupt_ = false;
        return *this;
    }

    /**
     * @brief Apply configuration and enable watchdog
     */
    void apply() const {
        WatchdogPolicy::configure(timeout_ms_, enable_reset_, enable_interrupt_);
        WatchdogPolicy::enable();
    }

    /**
     * @brief Apply configuration but keep watchdog disabled
     */
    void configure_only() const {
        WatchdogPolicy::configure(timeout_ms_, enable_reset_, enable_interrupt_);
    }

private:
    u16 timeout_ms_;
    bool enable_reset_;
    bool enable_interrupt_;
};

}  // namespace alloy::hal
