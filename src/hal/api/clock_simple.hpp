/**
 * @file clock_simple.hpp
 * @brief Level 1 Simple API for System Clock
 *
 * Provides one-liner setup for common clock configurations.
 * Platform-agnostic API that works with any Clock implementation.
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
 * @brief Simple Clock API
 *
 * Provides presets for common clock configurations.
 * This API is platform-agnostic and uses the ClockImpl template parameter.
 *
 * Example usage:
 * @code
 * // Use platform's default safe configuration (typically internal oscillator)
 * auto result = SystemClock::use_safe_default<Clock>();
 *
 * // Use platform's high-performance configuration (typically PLL)
 * auto result = SystemClock::use_high_performance<Clock>();
 *
 * // Enable peripheral clocks using platform-specific IDs
 * SystemClock::enable_peripheral<Clock>(PeripheralId::UART0);
 * @endcode
 */
class SystemClock {
public:
    /**
     * @brief Configure with platform's safe default configuration
     *
     * Uses the platform's safest, most reliable clock configuration.
     * Typically an internal oscillator without PLL.
     *
     * @tparam ClockImpl Platform-specific clock implementation
     * @return Result with error code
     */
    template <typename ClockImpl>
    static Result<void, ErrorCode> use_safe_default() {
        // Platform layer must provide CLOCK_CONFIG_SAFE_DEFAULT
        return ClockImpl::initialize(ClockImpl::CLOCK_CONFIG_SAFE_DEFAULT);
    }

    /**
     * @brief Configure with platform's high-performance configuration
     *
     * Uses the platform's highest performance clock configuration.
     * May use PLL or external oscillator.
     *
     * @tparam ClockImpl Platform-specific clock implementation
     * @return Result with error code
     */
    template <typename ClockImpl>
    static Result<void, ErrorCode> use_high_performance() {
        return ClockImpl::initialize(ClockImpl::CLOCK_CONFIG_HIGH_PERFORMANCE);
    }

    /**
     * @brief Configure with platform's medium-performance configuration
     *
     * @tparam ClockImpl Platform-specific clock implementation
     * @return Result with error code
     */
    template <typename ClockImpl>
    static Result<void, ErrorCode> use_medium_performance() {
        return ClockImpl::initialize(ClockImpl::CLOCK_CONFIG_MEDIUM_PERFORMANCE);
    }

    /**
     * @brief Configure with custom configuration
     *
     * @tparam ClockImpl Platform-specific clock implementation
     * @tparam ConfigType Platform-specific configuration type
     * @param config Custom configuration
     * @return Result with error code
     */
    template <typename ClockImpl, typename ConfigType>
    static Result<void, ErrorCode> use_custom(const ConfigType& config) {
        return ClockImpl::initialize(config);
    }

    /**
     * @brief Enable peripheral clock
     *
     * @tparam ClockImpl Platform-specific clock implementation
     * @param peripheral_id Platform-specific peripheral ID
     * @return Result with error code
     */
    template <typename ClockImpl>
    static Result<void, ErrorCode> enable_peripheral(u8 peripheral_id) {
        return ClockImpl::enablePeripheralClock(peripheral_id);
    }

    /**
     * @brief Disable peripheral clock
     *
     * @tparam ClockImpl Platform-specific clock implementation
     * @param peripheral_id Platform-specific peripheral ID
     * @return Result with error code
     */
    template <typename ClockImpl>
    static Result<void, ErrorCode> disable_peripheral(u8 peripheral_id) {
        return ClockImpl::disablePeripheralClock(peripheral_id);
    }

    /**
     * @brief Enable multiple peripherals at once
     *
     * @tparam ClockImpl Platform-specific clock implementation
     * @param peripheral_ids Array of peripheral IDs
     * @param count Number of peripherals to enable
     * @return Result with error code
     */
    template <typename ClockImpl>
    static Result<void, ErrorCode> enable_peripherals(const u8* peripheral_ids, u8 count) {
        for (u8 i = 0; i < count; i++) {
            auto result = ClockImpl::enablePeripheralClock(peripheral_ids[i]);
            if (!result.is_ok()) {
                return result;
            }
        }
        return Ok();
    }

    /**
     * @brief Get current master clock frequency
     *
     * @tparam ClockImpl Platform-specific clock implementation
     * @return Frequency in Hz
     */
    template <typename ClockImpl>
    static u32 get_frequency_hz() {
        return ClockImpl::getMasterClockFrequency();
    }
};

}  // namespace alloy::hal
