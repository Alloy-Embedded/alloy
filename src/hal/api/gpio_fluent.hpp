/**
 * @file gpio_fluent.hpp
 * @brief Level 2 Fluent API for GPIO
 *
 * Provides chainable builder pattern for readable GPIO configuration.
 * Fluent API enables method chaining for clear, self-documenting code.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "hal/api/gpio_simple.hpp"
#include "hal/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief Builder state tracking for compile-time validation
 */
struct GpioBuilderState {
    bool has_direction = false;
    bool has_pull = false;
    bool has_drive = false;
    bool has_active_level = false;

    constexpr bool is_valid() const {
        return has_direction;  // Minimum requirement
    }
};

/**
 * @brief Fluent GPIO configuration result
 *
 * @tparam PinType GPIO pin type
 */
template <typename PinType>
struct FluentGpioConfig {
    SimpleGpioPin<PinType> pin;

    /**
     * @brief Turn pin ON
     */
    Result<void, ErrorCode> on() { return pin.on(); }

    /**
     * @brief Turn pin OFF
     */
    Result<void, ErrorCode> off() { return pin.off(); }

    /**
     * @brief Toggle pin
     */
    Result<void, ErrorCode> toggle() { return pin.toggle(); }

    /**
     * @brief Check if pin is ON
     */
    Result<bool, ErrorCode> is_on() const { return pin.is_on(); }

    /**
     * @brief Access underlying simple pin
     */
    SimpleGpioPin<PinType>& simple_pin() { return pin; }
};

/**
 * @brief Fluent GPIO configuration builder
 *
 * Provides chainable methods for readable GPIO configuration.
 *
 * Example usage:
 * @code
 * // Configure LED (active-low output)
 * auto led = GpioBuilder<GpioPin<peripherals::PIOC, 8>>()
 *     .as_output()
 *     .active_low()
 *     .push_pull()
 *     .initialize();
 * led.value().on();
 *
 * // Configure button (input with pull-up)
 * auto button = GpioBuilder<GpioPin<peripherals::PIOA, 11>>()
 *     .as_input()
 *     .pull_up()
 *     .active_low()
 *     .initialize();
 * if (button.value().is_on().value()) {
 *     // Button pressed
 * }
 *
 * // Open-drain output for I2C
 * auto sda = GpioBuilder<GpioPin<peripherals::PIOA, 3>>()
 *     .as_output()
 *     .open_drain()
 *     .pull_up()
 *     .initialize();
 * @endcode
 *
 * @tparam PinType GPIO pin type (e.g., GpioPin<PORT, PIN>)
 */
template <typename PinType>
class GpioBuilder {
public:
    constexpr GpioBuilder()
        : direction_(PinDirection::Input),
          pull_(PinPull::None),
          drive_(PinDrive::PushPull),
          active_high_(true),
          state_() {}

    /**
     * @brief Configure as output
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& as_output() {
        direction_ = PinDirection::Output;
        state_.has_direction = true;
        return *this;
    }

    /**
     * @brief Configure as input
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& as_input() {
        direction_ = PinDirection::Input;
        state_.has_direction = true;
        return *this;
    }

    /**
     * @brief Set direction explicitly
     *
     * @param dir Pin direction
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& direction(PinDirection dir) {
        direction_ = dir;
        state_.has_direction = true;
        return *this;
    }

    /**
     * @brief Configure with pull-up resistor
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& pull_up() {
        pull_ = PinPull::PullUp;
        state_.has_pull = true;
        return *this;
    }

    /**
     * @brief Configure with pull-down resistor
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& pull_down() {
        pull_ = PinPull::PullDown;
        state_.has_pull = true;
        return *this;
    }

    /**
     * @brief No pull resistor (floating)
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& no_pull() {
        pull_ = PinPull::None;
        state_.has_pull = true;
        return *this;
    }

    /**
     * @brief Set pull resistor explicitly
     *
     * @param pull Pull configuration
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& pull(PinPull pull) {
        pull_ = pull;
        state_.has_pull = true;
        return *this;
    }

    /**
     * @brief Configure as push-pull output
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& push_pull() {
        drive_ = PinDrive::PushPull;
        state_.has_drive = true;
        return *this;
    }

    /**
     * @brief Configure as open-drain output
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& open_drain() {
        drive_ = PinDrive::OpenDrain;
        state_.has_drive = true;
        return *this;
    }

    /**
     * @brief Set drive mode explicitly
     *
     * @param drive Drive mode
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& drive(PinDrive drive) {
        drive_ = drive;
        state_.has_drive = true;
        return *this;
    }

    /**
     * @brief Configure as active-high (default)
     *
     * ON means pin is HIGH.
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& active_high() {
        active_high_ = true;
        state_.has_active_level = true;
        return *this;
    }

    /**
     * @brief Configure as active-low
     *
     * ON means pin is LOW (common for LEDs).
     *
     * @return Reference to builder for chaining
     */
    constexpr GpioBuilder& active_low() {
        active_high_ = false;
        state_.has_active_level = true;
        return *this;
    }

    /**
     * @brief Initialize with configured settings
     *
     * @return Result with FluentGpioConfig or error
     */
    Result<FluentGpioConfig<PinType>, ErrorCode> initialize() const {
        if (!state_.is_valid()) {
            return Err(ErrorCode::InvalidParameter);
        }

        SimpleGpioPin<PinType> pin(active_high_);

        // Apply configuration
        auto dir_result = pin.platform_pin().setDirection(direction_);
        if (!dir_result.is_ok()) {
            return Err(dir_result.error());
        }

        if (state_.has_pull) {
            auto pull_result = pin.platform_pin().setPull(pull_);
            if (!pull_result.is_ok()) {
                return Err(pull_result.error());
            }
        }

        if (state_.has_drive) {
            auto drive_result = pin.platform_pin().setDrive(drive_);
            if (!drive_result.is_ok()) {
                return Err(drive_result.error());
            }
        }

        // Set initial state to OFF
        if (direction_ == PinDirection::Output) {
            auto off_result = pin.off();
            if (!off_result.is_ok()) {
                return Err(off_result.error());
            }
        }

        return Ok(FluentGpioConfig<PinType>{std::move(pin)});
    }

private:
    PinDirection direction_;
    PinPull pull_;
    PinDrive drive_;
    bool active_high_;
    GpioBuilderState state_;
};

}  // namespace alloy::hal
