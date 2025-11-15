/**
 * @file gpio.hpp
 * @brief Template-based GPIO implementation for STM32G0 (Platform Layer)
 *
 * This file implements GPIO peripheral using the auto-generated hardware policy.
 * Uses Policy-Based Design for zero runtime overhead.
 *
 * Design Principles:
 * - Template-based: Port address and pin number resolved at compile-time
 * - Zero overhead: Fully inlined, single instruction for read/write
 * - Compile-time masks: Pin masks computed at compile-time
 * - Type-safe: Strong typing prevents pin conflicts
 * - Hardware Policy: Uses auto-generated Stm32g0GpioHardwarePolicy
 *
 * @note Uses auto-generated hardware policy from gpio_hardware_policy.hpp
 */

#pragma once

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include "hal/types.hpp"
#include "hal/vendors/st/stm32g0/gpio_hardware_policy.hpp"
#include "hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp"

#if __cplusplus >= 202002L
#include "hal/core/concepts.hpp"
#endif

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;
using namespace alloy::hal;

/**
 * @brief Template-based GPIO pin for STM32G0
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

    // Use hardware policy (64 MHz peripheral clock)
    using HwPolicy = Stm32g0GPIOHardwarePolicy<PORT_BASE, 64000000>;

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
     * @brief Write pin state (HIGH or LOW)
     *
     * @param value true = HIGH, false = LOW
     */
    Result<void, ErrorCode> write(bool value) {
        if (value) {
            return set();
        } else {
            return clear();
        }
    }

    /**
     * @brief Read pin state
     *
     * @return Result containing pin state (true = HIGH, false = LOW)
     */
    Result<bool, ErrorCode> read() const {
        return Ok(HwPolicy::read_input(pin_mask));
    }

    /**
     * @brief Check if pin is configured as output
     *
     * @return Result containing true if output, false if input
     */
    Result<bool, ErrorCode> isOutput() const {
        // Read MODER register (2 bits per pin)
        uint32_t mode = HwPolicy::read_mode(PIN_NUM);
        // Mode 01 = Output, 00 = Input, 10 = Alternate, 11 = Analog
        return Ok(mode == 1);
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

// ============================================================================
// Concept Validation (C++20)
// ============================================================================

#if __cplusplus >= 202002L
// Compile-time validation: Verify that GpioPin satisfies the GpioPin concept
// Using GPIOA pin 5 as example (LED on most Nucleo boards)
static_assert(alloy::hal::concepts::GpioPin<GpioPin<0x50000000, 5>>,
              "STM32G0 GpioPin must satisfy GpioPin concept - missing required methods");
#endif

} // namespace alloy::hal::st::stm32g0
