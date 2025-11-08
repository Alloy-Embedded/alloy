#ifndef ALLOY_HAL_HOST_GPIO_HPP
#define ALLOY_HAL_HOST_GPIO_HPP

#include <array>

#include <stdint.h>

#include "../interface/gpio.hpp"

namespace alloy::hal::host {

/// Host (PC) GPIO pin mock implementation
///
/// This implementation simulates GPIO operations by printing to console
/// and tracking pin state internally. Useful for:
/// - Development without hardware
/// - Unit testing application logic
/// - Validating API design
///
/// Example usage:
/// \code
/// #include "hal/host/gpio.hpp"
///
/// alloy::hal::host::GpioPin<13> led;
/// led.configure(PinMode::Output);
/// led.set_high();   // Prints: [GPIO Mock] Pin 13 set HIGH
/// led.toggle();     // Prints: [GPIO Mock] Pin 13 set LOW
/// \endcode
template <uint8_t PIN>
class GpioPin {
   public:
    static constexpr uint8_t pin_number = PIN;

    /// Constructor - initializes pin state to LOW
    GpioPin() : mode_(PinMode::Input), state_(false) {}

    /// Configure pin mode
    /// @param mode Desired pin mode (Input, Output, etc)
    void configure(PinMode mode);

    /// Set pin output to HIGH
    /// Prints debug message and updates internal state
    void set_high();

    /// Set pin output to LOW
    /// Prints debug message and updates internal state
    void set_low();

    /// Toggle pin output state
    /// Flips state between HIGH and LOW, prints debug message
    void toggle();

    /// Read current pin state
    /// @return true if pin is HIGH, false if LOW
    bool read() const { return state_; }

    /// Get current pin mode
    /// @return Current PinMode
    PinMode get_mode() const { return mode_; }

   private:
    PinMode mode_;
    bool state_;
};

// Static assertion to verify concept compliance
static_assert(alloy::hal::GpioPin<GpioPin<0>>, "host::GpioPin must satisfy GpioPin concept");

}  // namespace alloy::hal::host

#endif  // ALLOY_HAL_HOST_GPIO_HPP
