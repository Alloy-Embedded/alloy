#ifndef ALLOY_HAL_ESP32_GPIO_HPP
#define ALLOY_HAL_ESP32_GPIO_HPP

#include "../../interface/gpio.hpp"
#include "../../../generated/espressif_systems_shanghai_co_ltd/esp32/esp32/peripherals.hpp"
#include <cstdint>

namespace alloy::hal::esp32 {

/// MCU-specific namespace concept
/// Validates that a namespace contains the required generated peripherals
template<typename McuNamespace>
concept HasEsp32Peripherals = requires {
    typename McuNamespace::gpio::Registers;
    { McuNamespace::traits::num_gpio_instances } -> std::convertible_to<uint32_t>;
};

/// Default to the configured MCU namespace (set by CMake)
/// This can be overridden by boards that need a different variant
namespace mcu = alloy::generated::esp32;

/// GPIO Port Concept - validates that a type can be used as GPIO
template<typename T>
concept GpioPortType = requires {
    typename T::Registers;
} && requires(typename T::Registers* regs) {
    { regs->OUT } -> std::convertible_to<uint32_t>;
    { regs->OUT_W1TS } -> std::convertible_to<uint32_t>;
    { regs->OUT_W1TC } -> std::convertible_to<uint32_t>;
    { regs->IN } -> std::convertible_to<uint32_t>;
    { regs->ENABLE } -> std::convertible_to<uint32_t>;
    { regs->ENABLE_W1TS } -> std::convertible_to<uint32_t>;
    { regs->ENABLE_W1TC } -> std::convertible_to<uint32_t>;
};

/// Get GPIO registers
/// ESP32 has single GPIO peripheral for all pins
inline mcu::gpio::Registers* get_gpio() {
    using namespace mcu::gpio;
    return GPIO;
}

/// Modern C++20 GPIO Pin Implementation for ESP32
///
/// This class template provides compile-time GPIO pin configuration
/// using automatically generated peripheral definitions.
///
/// Features:
/// - Compile-time pin validation using concepts
/// - Zero-cost abstractions (everything inline/constexpr)
/// - Uses generated peripheral registers
/// - Type-safe API
/// - Atomic set/clear operations using W1TS/W1TC registers
///
/// Template Parameters:
/// - PIN: Pin number (0-39, where GPIO0=0, GPIO1=1, etc.)
///
/// ESP32 GPIO Notes:
/// - GPIO pins 0-39 (40 pins total)
/// - Pins 34-39 are INPUT-ONLY (no output capability)
/// - Pins 0-31 use OUT/OUT_W1TS/OUT_W1TC registers
/// - Pins 32-39 use OUT1/OUT1_W1TS/OUT1_W1TC registers
/// - W1TS = Write 1 to Set (atomic)
/// - W1TC = Write 1 to Clear (atomic)
///
/// Example usage:
/// \code
/// // Create LED pin (GPIO2 on ESP32 DevKit)
/// GpioPin<2> led;
/// led.configure(PinMode::Output);
/// led.set_high();
/// led.toggle();
/// \endcode
template<uint8_t PIN>
class GpioPin {
public:
    // Compile-time constants
    static constexpr uint8_t pin_number = PIN;

    // ESP32 has 40 GPIO pins (0-39)
    static_assert(PIN < 40, "Pin number must be < 40 (ESP32 has GPIO0-GPIO39)");

    // Pins 34-39 are input-only on ESP32
    static constexpr bool is_input_only = (PIN >= 34 && PIN <= 39);

    // Determine which register bank to use (0-31 or 32-39)
    static constexpr bool is_high_bank = (PIN >= 32);
    static constexpr uint8_t pin_bit = is_high_bank ? (PIN - 32) : PIN;

    /// Constructor - no clock enable needed for ESP32 GPIO
    GpioPin() = default;

    /// Configure pin mode
    /// @param mode Desired pin mode (Input, Output, etc.)
    inline void configure(PinMode mode) {
        auto* gpio_regs = get_gpio();
        if (!gpio_regs) return;

        switch (mode) {
            case PinMode::Input:
                // Disable output enable
                if constexpr (is_high_bank) {
                    gpio_regs->ENABLE1_W1TC = (1U << pin_bit);
                } else {
                    gpio_regs->ENABLE_W1TC = (1U << pin_bit);
                }
                break;

            case PinMode::InputPullUp:
                // Disable output, configure pull-up
                if constexpr (is_high_bank) {
                    gpio_regs->ENABLE1_W1TC = (1U << pin_bit);
                } else {
                    gpio_regs->ENABLE_W1TC = (1U << pin_bit);
                }
                // Note: Pull-up configuration requires IO_MUX registers
                // This is simplified - full implementation would set FUN_WPU bit
                break;

            case PinMode::InputPullDown:
                // Disable output, configure pull-down
                if constexpr (is_high_bank) {
                    gpio_regs->ENABLE1_W1TC = (1U << pin_bit);
                } else {
                    gpio_regs->ENABLE_W1TC = (1U << pin_bit);
                }
                // Note: Pull-down configuration requires IO_MUX registers
                // This is simplified - full implementation would set FUN_WPD bit
                break;

            case PinMode::Output:
                // Enable output (cannot configure input-only pins as output)
                static_assert(!is_input_only,
                    "Cannot configure pins 34-39 as output - they are input-only");

                if constexpr (!is_input_only) {
                    if constexpr (is_high_bank) {
                        gpio_regs->ENABLE1_W1TS = (1U << pin_bit);
                    } else {
                        gpio_regs->ENABLE_W1TS = (1U << pin_bit);
                    }
                }
                break;

            case PinMode::Alternate:
                // Alternate function setup requires GPIO matrix configuration
                // This is a simplified implementation
                static_assert(!is_input_only,
                    "Cannot configure pins 34-39 for alternate function output");
                break;

            case PinMode::Analog:
                // Analog mode - disable digital input/output
                if constexpr (is_high_bank) {
                    gpio_regs->ENABLE1_W1TC = (1U << pin_bit);
                } else {
                    gpio_regs->ENABLE_W1TC = (1U << pin_bit);
                }
                break;
        }
    }

    /// Set pin HIGH (using W1TS for atomic operation)
    inline void set_high() {
        static_assert(!is_input_only,
            "Cannot set output on input-only pins (GPIO34-39)");

        auto* gpio_regs = get_gpio();
        if (!gpio_regs) return;

        if constexpr (!is_input_only) {
            if constexpr (is_high_bank) {
                gpio_regs->OUT1_W1TS = (1U << pin_bit);  // Write 1 to Set
            } else {
                gpio_regs->OUT_W1TS = (1U << pin_bit);   // Write 1 to Set
            }
        }
    }

    /// Set pin LOW (using W1TC for atomic operation)
    inline void set_low() {
        static_assert(!is_input_only,
            "Cannot set output on input-only pins (GPIO34-39)");

        auto* gpio_regs = get_gpio();
        if (!gpio_regs) return;

        if constexpr (!is_input_only) {
            if constexpr (is_high_bank) {
                gpio_regs->OUT1_W1TC = (1U << pin_bit);  // Write 1 to Clear
            } else {
                gpio_regs->OUT_W1TC = (1U << pin_bit);   // Write 1 to Clear
            }
        }
    }

    /// Toggle pin output state
    inline void toggle() {
        static_assert(!is_input_only,
            "Cannot toggle output on input-only pins (GPIO34-39)");

        if constexpr (!is_input_only) {
            if (read()) {
                set_low();
            } else {
                set_high();
            }
        }
    }

    /// Read pin input state
    /// @return true if pin is HIGH, false if LOW
    inline bool read() const {
        auto* gpio_regs = get_gpio();
        if (!gpio_regs) return false;

        if constexpr (is_high_bank) {
            return (gpio_regs->IN1 & (1U << pin_bit)) != 0;
        } else {
            return (gpio_regs->IN & (1U << pin_bit)) != 0;
        }
    }
};

// Verify that our implementation satisfies the GPIO pin concept
static_assert(alloy::hal::GpioPin<GpioPin<0>>,
              "ESP32 GpioPin must satisfy the GpioPin concept");

/// Type alias for output pins (more readable)
template<uint8_t PIN>
using OutputPin = GpioPin<PIN>;

/// Type alias for input pins (more readable)
template<uint8_t PIN>
using InputPin = GpioPin<PIN>;

} // namespace alloy::hal::esp32

#endif // ALLOY_HAL_ESP32_GPIO_HPP
