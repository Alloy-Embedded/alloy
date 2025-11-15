/**
 * @file gpio.hpp
 * @brief Template-based GPIO implementation for STM32F7 (Platform Layer)
 *
 * This file implements GPIO peripheral using the auto-generated hardware policy.
 * Uses Policy-Based Design for zero runtime overhead.
 *
 * Design Principles:
 * - Template-based: Port address and pin number resolved at compile-time
 * - Zero overhead: Fully inlined, single instruction for read/write
 * - Compile-time masks: Pin masks computed at compile-time
 * - Type-safe: Strong typing prevents pin conflicts
 * - Hardware Policy: Uses auto-generated Stm32f4GpioHardwarePolicy
 *
 * @note Uses auto-generated hardware policy from gpio_hardware_policy.hpp
 */

#pragma once

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"
#include "hal/vendors/st/stm32f7/gpio_hardware_policy.hpp"
#include "hal/vendors/st/stm32f7/stm32f722/peripherals.hpp"

namespace alloy::hal::st::stm32f7 {

using namespace alloy::core;
using namespace alloy::hal;

/**
 * @brief Template-based GPIO pin for STM32F7
 *
 * This class provides a template-based GPIO implementation with ZERO runtime
 * overhead using auto-generated hardware policies.
 *
 * Template Parameters:
 * - PORT_BASE: GPIO port base address (compile-time constant)
 * - PIN_NUM: Pin number within port (0-15)
 *
 * Example usage:
 * @code
 * // Define LED pin (GPIOA pin 5)
 * using LedGreen = GpioPin<peripherals::GPIOA, 5>;
 *
 * auto led = LedGreen{};
 * led.setDirection(PinDirection::Output);
 * led.set();     // Turn on
 * led.clear();   // Turn off
 * led.toggle();  // Toggle state
 * @endcode
 *
 * @tparam PORT_BASE GPIO port base address
 * @tparam PIN_NUM Pin number (0-15)
 */
template <uint32_t PORT_BASE, uint8_t PIN_NUM>
class GpioPin {
public:
    // Compile-time constants
    static constexpr uint32_t port_base = PORT_BASE;
    static constexpr uint8_t pin_number = PIN_NUM;
    static constexpr uint32_t pin_mask = (1u << PIN_NUM);

    // Validate pin number at compile-time
    static_assert(PIN_NUM < 16, "Pin number must be 0-15");

    // Use hardware policy (180 MHz peripheral clock)
    using HwPolicy = Stm32f7GPIOHardwarePolicy<PORT_BASE, 180000000>;

    /**
     * @brief Set pin HIGH (output = 1)
     */
    Result<void, ErrorCode> set() {
        HwPolicy::set_output(pin_mask);
        return Ok();
    }

    /**
     * @brief Set pin LOW (output = 0)
     */
    Result<void, ErrorCode> clear() {
        HwPolicy::clear_output(pin_mask);
        return Ok();
    }

    /**
     * @brief Toggle pin state
     */
    Result<void, ErrorCode> toggle() {
        HwPolicy::toggle_output(pin_mask);
        return Ok();
    }

    /**
     * @brief Read pin state
     */
    bool read() const {
        return HwPolicy::read_input(pin_mask);
    }

    /**
     * @brief Configure pin direction
     *
     * @param direction Input or Output
     */
    Result<void, ErrorCode> setDirection(PinDirection direction) {
        if (direction == PinDirection::Output) {
            HwPolicy::set_mode_output(PIN_NUM);
        } else {
            HwPolicy::set_mode_input(PIN_NUM);
        }
        return Ok();
    }

    /**
     * @brief Configure pull-up/pull-down
     *
     * @param pull None, PullUp, or PullDown
     */
    Result<void, ErrorCode> setPull(PinPull pull) {
        switch (pull) {
            case PinPull::PullUp:
                HwPolicy::set_pull_up(PIN_NUM);
                break;
            case PinPull::PullDown:
                HwPolicy::set_pull_down(PIN_NUM);
                break;
            case PinPull::None:
            default:
                HwPolicy::set_pull_none(PIN_NUM);
                break;
        }
        return Ok();
    }

    /**
     * @brief Configure output drive type
     *
     * @param drive PushPull or OpenDrain
     */
    Result<void, ErrorCode> setDrive(PinDrive drive) {
        if (drive == PinDrive::OpenDrain) {
            HwPolicy::set_output_type_opendrain(pin_mask);
        } else {
            HwPolicy::set_output_type_pushpull(pin_mask);
        }
        return Ok();
    }

    /**
     * @brief Configure alternate function
     *
     * @param af_number Alternate function number (0-15)
     */
    Result<void, ErrorCode> setAlternateFunction(uint8_t af_number) {
        HwPolicy::set_alternate_function(PIN_NUM, af_number);
        HwPolicy::set_mode_alternate(PIN_NUM);
        return Ok();
    }
};

} // namespace alloy::hal::st::stm32f7
