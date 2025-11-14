/**
 * @file gpio_simple.hpp
 * @brief Level 1 Simple API for GPIO
 *
 * Provides one-liner setup and common presets for GPIO pins.
 * Simple API is designed for common use cases with minimal configuration.
 *
 * @note Part of Alloy HAL API Layer
 */

#pragma once

#include "core/error_code.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"

namespace alloy::hal {

using namespace alloy::core;

/**
 * @brief GPIO pin wrapper with simple interface
 *
 * Wraps a GpioPin template with a simpler interface that handles
 * active-high/active-low logic automatically.
 *
 * @tparam PinType Type satisfying GpioPin interface (e.g., GpioPin<PORT, PIN>)
 */
template <typename PinType>
class SimpleGpioPin {
public:
    constexpr SimpleGpioPin(bool active_high = true)
        : active_high_(active_high), pin_() {}

    /**
     * @brief Turn pin ON (respects active-high/active-low)
     */
    Result<void, ErrorCode> on() {
        return active_high_ ? pin_.set() : pin_.clear();
    }

    /**
     * @brief Turn pin OFF (respects active-high/active-low)
     */
    Result<void, ErrorCode> off() {
        return active_high_ ? pin_.clear() : pin_.set();
    }

    /**
     * @brief Toggle pin state
     */
    Result<void, ErrorCode> toggle() {
        return pin_.toggle();
    }

    /**
     * @brief Read pin state (returns logical state, not physical)
     */
    Result<bool, ErrorCode> is_on() const {
        auto result = pin_.read();
        if (!result.is_ok()) {
            return Err(result.error());
        }
        bool physical_state = result.value();
        return Ok(active_high_ ? physical_state : !physical_state);
    }

    /**
     * @brief Configure as output
     */
    Result<void, ErrorCode> set_output() {
        return pin_.setDirection(PinDirection::Output);
    }

    /**
     * @brief Configure as input
     */
    Result<void, ErrorCode> set_input() {
        return pin_.setDirection(PinDirection::Input);
    }

    /**
     * @brief Configure pull resistor
     */
    Result<void, ErrorCode> set_pull(PinPull pull) {
        return pin_.setPull(pull);
    }

    /**
     * @brief Access underlying platform pin
     */
    PinType& platform_pin() { return pin_; }
    const PinType& platform_pin() const { return pin_; }

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
