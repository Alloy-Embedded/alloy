/**
 * @file gpio_simple.hpp
 * @brief Level 1 Simple API for GPIO
 *
 * Provides one-liner setup and common presets for GPIO pins.
 * Simple API is designed for common use cases with minimal configuration.
 *
 * Design Principles:
 * - One-liner initialization for common use cases
 * - Active-high/active-low abstraction
 * - Factory methods for presets (LED, button, etc.)
 * - Built on GpioBase CRTP for code reuse
 *
 * @note Part of Phase 1.7: Refactor GPIO APIs (library-quality-improvements)
 * @see docs/architecture/CRTP_PATTERN.md
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"
#include "hal/api/gpio_base.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief GPIO pin wrapper with simple interface
 *
 * Wraps a GpioPin template with a simpler interface that handles
 * active-high/active-low logic automatically. Now inherits from GpioBase
 * using CRTP for zero-overhead code reuse.
 *
 * @tparam PinType Type satisfying GpioPin interface (e.g., GpioPin<PORT, PIN>)
 */
template <typename PinType>
class SimpleGpioPin : public GpioBase<SimpleGpioPin<PinType>> {
    using Base = GpioBase<SimpleGpioPin<PinType>>;
    friend Base;

public:
    constexpr SimpleGpioPin(bool active_high = true)
        : active_high_(active_high), pin_() {}

    // ========================================================================
    // Inherited Interface from GpioBase (CRTP)
    // ========================================================================

    // Inherit all common GPIO methods from base
    using Base::on;              // Turn pin logically ON
    using Base::off;             // Turn pin logically OFF
    using Base::toggle;          // Toggle pin state
    using Base::is_on;           // Check if logically ON
    using Base::is_off;          // Check if logically OFF
    using Base::set;             // Set pin physically HIGH
    using Base::clear;           // Set pin physically LOW
    using Base::read;            // Read physical pin state
    using Base::set_direction;   // Set pin direction
    using Base::set_output;      // Configure as output
    using Base::set_input;       // Configure as input
    using Base::set_pull;        // Set pull resistor
    using Base::set_drive;       // Set drive mode
    using Base::configure_push_pull_output;
    using Base::configure_open_drain_output;
    using Base::configure_input_pullup;
    using Base::configure_input_pulldown;
    using Base::configure_input_floating;

    /**
     * @brief Access underlying platform pin
     */
    PinType& platform_pin() { return pin_; }
    const PinType& platform_pin() const { return pin_; }

    // ========================================================================
    // Implementation Methods (public for concept checking)
    // ========================================================================

    /**
     * @brief Turn pin logically ON - implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> on_impl() noexcept {
        return active_high_ ? pin_.set() : pin_.clear();
    }

    /**
     * @brief Turn pin logically OFF - implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> off_impl() noexcept {
        return active_high_ ? pin_.clear() : pin_.set();
    }

    /**
     * @brief Toggle pin state - implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> toggle_impl() noexcept {
        return pin_.toggle();
    }

    /**
     * @brief Check if pin is logically ON - implementation
     */
    [[nodiscard]] constexpr Result<bool, ErrorCode> is_on_impl() const noexcept {
        auto result = pin_.read();
        if (result.is_err()) {
            return Err(std::move(result).err());
        }
        bool physical_state = result.unwrap();
        return Ok(active_high_ ? physical_state : !physical_state);
    }

    /**
     * @brief Set pin physically HIGH - implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_impl() noexcept {
        return pin_.set();
    }

    /**
     * @brief Set pin physically LOW - implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> clear_impl() noexcept {
        return pin_.clear();
    }

    /**
     * @brief Read physical pin state - implementation
     */
    [[nodiscard]] constexpr Result<bool, ErrorCode> read_impl() const noexcept {
        return pin_.read();
    }

    /**
     * @brief Set pin direction - implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_direction_impl(PinDirection direction) noexcept {
        return pin_.setDirection(direction);
    }

    /**
     * @brief Set pull resistor - implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_pull_impl(PinPull pull) noexcept {
        return pin_.setPull(pull);
    }

    /**
     * @brief Set drive mode - implementation
     */
    [[nodiscard]] constexpr Result<void, ErrorCode> set_drive_impl(PinDrive drive) noexcept {
        return pin_.setDrive(drive);
    }

private:
    bool active_high_;
    PinType pin_;
};

/**
 * @brief Simple GPIO API for common use cases
 *
 * Provides factory methods for quickly creating GPIO pins with
 * common configurations.
 *
 * Example usage:
 * @code
 * // Create output pin (active-high LED)
 * auto led = Gpio::output<GpioPin<peripherals::PIOC, 8>>();
 * led.on();
 * led.off();
 * led.toggle();
 *
 * // Create output pin with active-low (LED on when pin is LOW)
 * auto led_active_low = Gpio::output_active_low<GpioPin<peripherals::PIOC, 8>>();
 * led_active_low.on();  // Sets pin LOW
 *
 * // Create input with pull-up
 * auto button = Gpio::input_pullup<GpioPin<peripherals::PIOA, 11>>();
 * if (button.is_on().value()) {
 *     // Button pressed
 * }
 * @endcode
 */
class Gpio {
public:
    /**
     * @brief Create output pin (active-high)
     *
     * @tparam PinType GPIO pin type (e.g., GpioPin<PORT, PIN>)
     * @return SimpleGpioPin configured as output
     */
    template <typename PinType>
    static SimpleGpioPin<PinType> output() {
        SimpleGpioPin<PinType> pin(true);  // active-high
        pin.set_output();
        pin.off();  // Start in OFF state
        return pin;
    }

    /**
     * @brief Create output pin (active-low)
     *
     * Useful for LEDs that turn on when pin is LOW.
     *
     * @tparam PinType GPIO pin type
     * @return SimpleGpioPin configured as active-low output
     */
    template <typename PinType>
    static SimpleGpioPin<PinType> output_active_low() {
        SimpleGpioPin<PinType> pin(false);  // active-low
        pin.set_output();
        pin.off();  // Start in OFF state (physically HIGH)
        return pin;
    }

    /**
     * @brief Create input pin (no pull resistor)
     *
     * @tparam PinType GPIO pin type
     * @return SimpleGpioPin configured as input
     */
    template <typename PinType>
    static SimpleGpioPin<PinType> input() {
        SimpleGpioPin<PinType> pin(true);
        pin.set_input();
        pin.set_pull(PinPull::None);
        return pin;
    }

    /**
     * @brief Create input pin with pull-up resistor
     *
     * Useful for buttons (button press pulls pin LOW).
     *
     * @tparam PinType GPIO pin type
     * @return SimpleGpioPin configured as input with pull-up
     */
    template <typename PinType>
    static SimpleGpioPin<PinType> input_pullup() {
        SimpleGpioPin<PinType> pin(false);  // active-low (pressed = LOW)
        pin.set_input();
        pin.set_pull(PinPull::PullUp);
        return pin;
    }

    /**
     * @brief Create input pin with pull-down resistor
     *
     * @tparam PinType GPIO pin type
     * @return SimpleGpioPin configured as input with pull-down
     */
    template <typename PinType>
    static SimpleGpioPin<PinType> input_pulldown() {
        SimpleGpioPin<PinType> pin(true);  // active-high (pressed = HIGH)
        pin.set_input();
        pin.set_pull(PinPull::PullDown);
        return pin;
    }

    /**
     * @brief Create open-drain output
     *
     * Useful for I2C, 1-Wire, etc.
     *
     * @tparam PinType GPIO pin type
     * @return SimpleGpioPin configured as open-drain
     */
    template <typename PinType>
    static SimpleGpioPin<PinType> open_drain() {
        SimpleGpioPin<PinType> pin(true);
        pin.set_output();
        pin.platform_pin().setDrive(PinDrive::OpenDrain);
        pin.off();
        return pin;
    }
};

}  // namespace alloy::hal
