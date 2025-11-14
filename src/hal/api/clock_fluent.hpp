/**
 * @file clock_fluent.hpp
 * @brief Level 2 Fluent API for System Clock
 *
 * Provides chainable builder pattern for clock configuration.
 * Platform-agnostic API.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/api/clock_simple.hpp"

namespace alloy::hal {

using namespace alloy::core;

struct ClockBuilderState {
    bool has_config = false;

    constexpr bool is_valid() const {
        return has_config;
    }
};

/**
 * @brief Fluent clock configuration builder
 *
 * Platform-agnostic clock configuration using builder pattern.
 *
 * Example:
 * @code
 * auto result = ClockBuilder<Clock>()
 *     .use_safe_default()
 *     .enable_peripherals(peripheral_ids, count)
 *     .initialize();
 * @endcode
 */
template <typename ClockImpl, typename ConfigType>
class ClockBuilder {
public:
    constexpr ClockBuilder() : config_ptr_(nullptr), state_() {}

    /**
     * @brief Use platform's safe default configuration
     */
    constexpr ClockBuilder& use_safe_default() {
        config_ptr_ = &ClockImpl::CLOCK_CONFIG_SAFE_DEFAULT;
        state_.has_config = true;
        return *this;
    }

    /**
     * @brief Use platform's high-performance configuration
     */
    constexpr ClockBuilder& use_high_performance() {
        config_ptr_ = &ClockImpl::CLOCK_CONFIG_HIGH_PERFORMANCE;
        state_.has_config = true;
        return *this;
    }

    /**
     * @brief Use platform's medium-performance configuration
     */
    constexpr ClockBuilder& use_medium_performance() {
        config_ptr_ = &ClockImpl::CLOCK_CONFIG_MEDIUM_PERFORMANCE;
        state_.has_config = true;
        return *this;
    }

    /**
     * @brief Use custom configuration
     */
    constexpr ClockBuilder& use_custom(const ConfigType& config) {
        config_ptr_ = &config;
        state_.has_config = true;
        return *this;
    }

    /**
     * @brief Enable specific peripheral
     *
     * @param peripheral_id Platform-specific peripheral ID
     */
    ClockBuilder& enable_peripheral(u8 peripheral_id) {
        if (peripheral_count_ < MAX_PERIPHERALS) {
            peripherals_to_enable_[peripheral_count_++] = peripheral_id;
        }
        return *this;
    }

    /**
     * @brief Enable multiple peripherals
     *
     * @param peripheral_ids Array of peripheral IDs
     * @param count Number of peripherals
     */
    ClockBuilder& enable_peripherals(const u8* peripheral_ids, u8 count) {
        for (u8 i = 0; i < count && peripheral_count_ < MAX_PERIPHERALS; i++) {
            peripherals_to_enable_[peripheral_count_++] = peripheral_ids[i];
        }
        return *this;
    }

    /**
     * @brief Initialize with configured settings
     */
    Result<void, ErrorCode> initialize() const {
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        // Initialize clock
        auto result = ClockImpl::initialize(*config_ptr_);
        if (!result.is_ok()) {
            return result;
        }

        // Enable requested peripherals
        for (u8 i = 0; i < peripheral_count_; i++) {
            auto enable_result = ClockImpl::enablePeripheralClock(peripherals_to_enable_[i]);
            if (!enable_result.is_ok()) {
                return enable_result;
            }
        }

        return Ok();
    }

private:
    static constexpr u8 MAX_PERIPHERALS = 32;  // Maximum peripherals to track

    const ConfigType* config_ptr_;
    ClockBuilderState state_;
    u8 peripherals_to_enable_[MAX_PERIPHERALS];
    u8 peripheral_count_ = 0;
};

}  // namespace alloy::hal
