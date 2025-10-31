/// ESP32 GPIO Implementation
///
/// Minimal GPIO implementation for ESP32
/// ESP32 has 34 GPIO pins (0-39), some are input-only

#ifndef ALLOY_HAL_ESPRESSIF_ESP32_GPIO_HPP
#define ALLOY_HAL_ESPRESSIF_ESP32_GPIO_HPP

#include "hal/interface/gpio.hpp"
#include "generated/unknown/esp32/esp32/peripherals.hpp"
#include <cstdint>

namespace alloy::hal::espressif::esp32 {

// Import generated peripherals
using namespace alloy::generated::esp32;

/// ESP32 GPIO pin implementation
/// Pin numbering: GPIO0=0, GPIO1=1, ..., GPIO39=39
template<uint8_t PIN>
class GpioPin {
public:
    static constexpr uint8_t pin_number = PIN;

    // ESP32 has 40 GPIO pins (0-39)
    static_assert(PIN < 40, "GPIO pin must be 0-39 on ESP32");

    // Pins 34-39 are input-only
    static constexpr bool is_input_only = (PIN >= 34);

    /// Constructor
    GpioPin() = default;

    /// Configure pin mode
    void configure(hal::PinMode mode) {
        if (mode == hal::PinMode::Output) {
            set_as_output();
        } else {
            set_as_input();
        }
    }

    /// Set pin to HIGH
    void set_high() {
        static_assert(!is_input_only, "Cannot set output on input-only pin");
        if (PIN < 32) {
            gpio::GPIO->GPIO_OUT_W1TS = (1U << PIN);
        } else {
            gpio::GPIO->GPIO_OUT1_W1TS = (1U << (PIN - 32));
        }
    }

    /// Set pin to LOW
    void set_low() {
        static_assert(!is_input_only, "Cannot set output on input-only pin");
        if (PIN < 32) {
            gpio::GPIO->GPIO_OUT_W1TC = (1U << PIN);
        } else {
            gpio::GPIO->GPIO_OUT1_W1TC = (1U << (PIN - 32));
        }
    }

    /// Toggle pin
    void toggle() {
        static_assert(!is_input_only, "Cannot toggle input-only pin");
        if (read()) {
            set_low();
        } else {
            set_high();
        }
    }

    /// Read pin state
    bool read() const {
        if (PIN < 32) {
            return (gpio::GPIO->GPIO_IN & (1U << PIN)) != 0;
        } else {
            return (gpio::GPIO->GPIO_IN1 & (1U << (PIN - 32))) != 0;
        }
    }

private:
    /// Configure pin as output
    void set_as_output() {
        static_assert(!is_input_only, "Cannot configure input-only pin as output");

        // Enable output
        if (PIN < 32) {
            gpio::GPIO->GPIO_ENABLE_W1TS = (1U << PIN);
        } else {
            gpio::GPIO->GPIO_ENABLE1_W1TS = (1U << (PIN - 32));
        }

        // Configure function as GPIO (function 0)
        // ESP32 uses GPIO_FUNCx_OUT_SEL_CFG registers
        // For simplicity, we assume GPIO function is already selected
    }

    /// Configure pin as input
    void set_as_input() {
        // Disable output
        if (PIN < 32) {
            gpio::GPIO->GPIO_ENABLE_W1TC = (1U << PIN);
        } else {
            gpio::GPIO->GPIO_ENABLE1_W1TC = (1U << (PIN - 32));
        }
    }
};

// Static assertion to verify concept compliance
static_assert(alloy::hal::GpioPin<GpioPin<2>>,
              "ESP32 GpioPin must satisfy GpioPin concept");

} // namespace alloy::hal::espressif::esp32

#endif // ALLOY_HAL_ESPRESSIF_ESP32_GPIO_HPP
