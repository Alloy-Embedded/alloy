/**
 * @file watchdog_expert.hpp
 * @brief Level 3 Expert API for Watchdog Timer
 *
 * Provides full control over watchdog configuration.
 * Platform-agnostic API using Hardware Policies.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Expert watchdog configuration
 *
 * Platform-agnostic configuration structure.
 * Actual implementation details handled by Hardware Policy.
 */
struct WatchdogExpertConfig {
    bool enable;               ///< Enable watchdog
    u16 timeout_ms;            ///< Timeout in milliseconds
    bool reset_enable;         ///< Enable reset on timeout
    bool interrupt_enable;     ///< Enable interrupt on timeout
    u8 priority;               ///< Interrupt priority (if interrupts enabled)

    constexpr bool is_valid() const {
        // Timeout must be reasonable (1ms - 60 seconds)
        if (enable && (timeout_ms == 0 || timeout_ms > 60000)) {
            return false;
        }
        return true;
    }

    constexpr const char* error_message() const {
        if (enable && timeout_ms == 0) return "Timeout cannot be zero";
        if (enable && timeout_ms > 60000) return "Timeout max is 60 seconds";
        return "Valid";
    }

    /**
     * @brief Disabled watchdog (typical for development)
     */
    static constexpr WatchdogExpertConfig disabled() {
        return WatchdogExpertConfig{
            .enable = false,
            .timeout_ms = 0,
            .reset_enable = false,
            .interrupt_enable = false,
            .priority = 0
        };
    }

    /**
     * @brief Standard 1-second watchdog (production)
     */
    static constexpr WatchdogExpertConfig standard_1s() {
        return WatchdogExpertConfig{
            .enable = true,
            .timeout_ms = 1000,
            .reset_enable = true,
            .interrupt_enable = false,
            .priority = 0
        };
    }

    /**
     * @brief Watchdog with interrupt-before-reset
     */
    static constexpr WatchdogExpertConfig with_early_warning(u16 timeout_ms, u8 priority = 15) {
        return WatchdogExpertConfig{
            .enable = true,
            .timeout_ms = timeout_ms,
            .reset_enable = true,
            .interrupt_enable = true,
            .priority = priority
        };
    }

    /**
     * @brief Custom configuration
     */
    static constexpr WatchdogExpertConfig custom(
        u16 timeout_ms,
        bool reset = true,
        bool interrupt = false,
        u8 priority = 15) {

        return WatchdogExpertConfig{
            .enable = true,
            .timeout_ms = timeout_ms,
            .reset_enable = reset,
            .interrupt_enable = interrupt,
            .priority = priority
        };
    }
};

namespace expert {

/**
 * @brief Configure watchdog with expert settings
 *
 * Platform-agnostic configuration using Hardware Policy.
 *
 * @tparam WatchdogPolicy Platform-specific watchdog hardware policy
 * @param config Expert configuration
 * @return Result with error code
 */
template <typename WatchdogPolicy>
Result<void, ErrorCode> configure(const WatchdogExpertConfig& config) {
    if (!config.is_valid()) {
        return Err(ErrorCode::InvalidParameter);
    }

    if (!config.enable) {
        WatchdogPolicy::disable();
        return Ok();
    }

    // Configure using hardware policy
    WatchdogPolicy::configure(config.timeout_ms, config.reset_enable, config.interrupt_enable);

    if (config.interrupt_enable) {
        WatchdogPolicy::set_interrupt_priority(config.priority);
    }

    WatchdogPolicy::enable();

    return Ok();
}

}  // namespace expert

}  // namespace alloy::hal
