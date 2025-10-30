#ifndef ALLOY_HAL_INTERFACE_GPIO_HPP
#define ALLOY_HAL_INTERFACE_GPIO_HPP

#include <concepts>
#include <cstdint>

namespace alloy::hal {

/// Pin configuration mode
enum class PinMode : uint8_t {
    Input = 0,        ///< Digital input
    Output,           ///< Digital output
    InputPullUp,      ///< Input with pull-up resistor
    InputPullDown,    ///< Input with pull-down resistor
    Alternate,        ///< Alternate function (UART, SPI, etc)
    Analog            ///< Analog input (ADC)
};

/// Concept defining the interface for GPIO pin implementations
///
/// Any type satisfying this concept can be used as a GPIO pin in Alloy.
/// Implementations must provide basic digital I/O operations.
///
/// Example implementation:
/// \code
/// class MyGpioPin {
/// public:
///     void set_high() { /* ... */ }
///     void set_low() { /* ... */ }
///     void toggle() { /* ... */ }
///     bool read() const { /* ... */ }
/// };
/// static_assert(GpioPin<MyGpioPin>);
/// \endcode
template<typename T>
concept GpioPin = requires(T pin, const T const_pin) {
    // Output operations
    { pin.set_high() } -> std::same_as<void>;
    { pin.set_low() } -> std::same_as<void>;
    { pin.toggle() } -> std::same_as<void>;

    // Input operation
    { const_pin.read() } -> std::same_as<bool>;
};

/// Compile-time configured GPIO pin template
///
/// This template provides compile-time pin configuration with mode-based
/// operation restrictions. Invalid operations for a given mode will cause
/// compile-time errors.
///
/// Template Parameters:
/// - PIN: Pin number (platform-specific, e.g., GPIO pin number)
/// - MODE: Pin mode from PinMode enum
///
/// Example usage:
/// \code
/// // Output pin - can write, cannot read
/// ConfiguredGpioPin<13, PinMode::Output> led;
/// led.set_high();  // OK
/// led.toggle();    // OK
/// // led.read();   // Compile error - Output pins don't have read()
///
/// // Input pin - can read, cannot write
/// ConfiguredGpioPin<5, PinMode::Input> button;
/// bool state = button.read();  // OK
/// // button.set_high();        // Compile error - Input pins don't have set_high()
///
/// // Input with pull-up
/// ConfiguredGpioPin<12, PinMode::InputPullUp> sensor;
/// bool value = sensor.read();  // OK
/// \endcode
template<uint8_t PIN, PinMode MODE>
class ConfiguredGpioPin {
public:
    static constexpr uint8_t pin_number = PIN;
    static constexpr PinMode pin_mode = MODE;

    // Output operations - only available for Output mode
    void set_high() requires (MODE == PinMode::Output) {
        // Implementation will be provided by platform-specific specialization
        static_assert(MODE == PinMode::Output,
            "set_high() is only available for Output pins");
    }

    void set_low() requires (MODE == PinMode::Output) {
        static_assert(MODE == PinMode::Output,
            "set_low() is only available for Output pins");
    }

    void toggle() requires (MODE == PinMode::Output) {
        static_assert(MODE == PinMode::Output,
            "toggle() is only available for Output pins");
    }

    // Input operation - available for Input modes
    bool read() const requires (MODE == PinMode::Input ||
                                MODE == PinMode::InputPullUp ||
                                MODE == PinMode::InputPullDown) {
        static_assert(MODE == PinMode::Input ||
                     MODE == PinMode::InputPullUp ||
                     MODE == PinMode::InputPullDown,
            "read() is only available for Input, InputPullUp, or InputPullDown pins");
        return false; // Platform-specific implementation needed
    }

protected:
    ConfiguredGpioPin() = default;
};

// Compile-time validation helpers

/// Validate that a pin number is within valid range
/// Usage: ALLOY_VALIDATE_PIN_NUMBER<25>();
template<uint8_t PIN>
consteval void validate_pin_number() {
    // Platform-specific implementations will specialize this
    // Default: allow all pin numbers (validation happens at platform level)
}

/// Type alias for output pins
template<uint8_t PIN>
using OutputPin = ConfiguredGpioPin<PIN, PinMode::Output>;

/// Type alias for input pins
template<uint8_t PIN>
using InputPin = ConfiguredGpioPin<PIN, PinMode::Input>;

/// Type alias for input pins with pull-up
template<uint8_t PIN>
using InputPinPullUp = ConfiguredGpioPin<PIN, PinMode::InputPullUp>;

/// Type alias for input pins with pull-down
template<uint8_t PIN>
using InputPinPullDown = ConfiguredGpioPin<PIN, PinMode::InputPullDown>;

} // namespace alloy::hal

#endif // ALLOY_HAL_INTERFACE_GPIO_HPP
