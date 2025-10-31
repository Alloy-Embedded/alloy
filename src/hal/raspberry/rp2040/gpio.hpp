#ifndef ALLOY_HAL_RP2040_GPIO_HPP
#define ALLOY_HAL_RP2040_GPIO_HPP

#include "../../interface/gpio.hpp"
#include <cstdint>
#include <peripherals.hpp>

namespace alloy::hal::rp2040 {

/// MCU-specific namespace concept
/// Validates that a namespace contains the required generated peripherals
template<typename McuNamespace>
concept HasRp2040Peripherals = requires {
    typename McuNamespace::sio::Registers;
    { McuNamespace::traits::flash_size_kb } -> std::convertible_to<uint32_t>;
};

/// Default to the configured MCU namespace (set by CMake)
/// This can be overridden by boards that need a different variant
namespace mcu = alloy::generated::rp2040;

/// SIO Concept - validates that a type can be used as SIO peripheral
template<typename T>
concept SioPeripheralType = requires {
    typename T::Registers;
} && requires(typename T::Registers* regs) {
    { regs->GPIO_IN } -> std::convertible_to<uint32_t>;
    { regs->GPIO_OUT } -> std::convertible_to<uint32_t>;
    { regs->GPIO_OUT_SET } -> std::convertible_to<uint32_t>;
    { regs->GPIO_OUT_CLR } -> std::convertible_to<uint32_t>;
    { regs->GPIO_OUT_XOR } -> std::convertible_to<uint32_t>;
    { regs->GPIO_OE } -> std::convertible_to<uint32_t>;
    { regs->GPIO_OE_SET } -> std::convertible_to<uint32_t>;
    { regs->GPIO_OE_CLR } -> std::convertible_to<uint32_t>;
};

/// Get SIO peripheral (Single-cycle IO for fast GPIO access)
inline mcu::sio::Registers* get_sio() {
    return mcu::sio::SIO_IRQ_PROC0;
}

/// Modern C++20 GPIO Pin Implementation for RP2040
///
/// This class template provides compile-time GPIO pin configuration
/// using automatically generated peripheral definitions.
///
/// Features:
/// - Compile-time pin validation using concepts
/// - Zero-cost abstractions (everything inline/constexpr)
/// - Uses generated peripheral registers (SIO for fast GPIO)
/// - Type-safe API
/// - Atomic set/clear operations
///
/// Template Parameters:
/// - PIN: Pin number (0-29 for GPIO0-GPIO29)
///
/// Example usage:
/// \code
/// // Create LED pin (GPIO25 on Raspberry Pi Pico)
/// GpioPin<25> led;
/// led.configure(PinMode::Output);
/// led.set_high();
/// led.toggle();
/// \endcode
template<uint8_t PIN>
class GpioPin {
public:
    // Compile-time constants
    static constexpr uint8_t pin_number = PIN;
    static constexpr uint32_t pin_mask = (1U << PIN);

    // Compile-time validation
    static_assert(PIN < 30, "RP2040 has GPIO pins 0-29");

    /// Constructor - no clock enable needed on RP2040 (SIO always active)
    GpioPin() = default;

    /// Configure pin mode
    /// @param mode Desired pin mode (Input, Output, InputPullUp, InputPullDown)
    /// @note RP2040 GPIO configuration is simpler than STM32:
    ///       - Output: Set GPIO_OE bit (output enable)
    ///       - Input: Clear GPIO_OE bit
    ///       - Pull resistors would require IO_BANK0 configuration (not implemented here)
    inline void configure(PinMode mode) {
        auto* sio = get_sio();
        if (!sio) return;

        switch (mode) {
            case PinMode::Input:
                // Clear output enable = input mode
                sio->GPIO_OE_CLR = pin_mask;
                break;

            case PinMode::InputPullUp:
            case PinMode::InputPullDown:
                // Clear output enable for input
                sio->GPIO_OE_CLR = pin_mask;
                // TODO: Configure pull resistors via PADS_BANK0
                // For now, just treat as floating input
                break;

            case PinMode::Output:
                // Set output enable
                sio->GPIO_OE_SET = pin_mask;
                break;

            case PinMode::Alternate:
                // Clear output enable (alternate function controlled by IO_BANK0)
                sio->GPIO_OE_CLR = pin_mask;
                // TODO: Configure function via IO_BANK0
                break;

            case PinMode::Analog:
                // Clear output enable
                sio->GPIO_OE_CLR = pin_mask;
                // TODO: Configure ADC if needed
                break;
        }
    }

    /// Set pin HIGH (using atomic set register)
    inline void set_high() {
        auto* sio = get_sio();
        if (!sio) return;
        sio->GPIO_OUT_SET = pin_mask;
    }

    /// Set pin LOW (using atomic clear register)
    inline void set_low() {
        auto* sio = get_sio();
        if (!sio) return;
        sio->GPIO_OUT_CLR = pin_mask;
    }

    /// Toggle pin output state (using atomic XOR register)
    inline void toggle() {
        auto* sio = get_sio();
        if (!sio) return;
        sio->GPIO_OUT_XOR = pin_mask;
    }

    /// Read pin input state
    /// @return true if pin is HIGH, false if LOW
    inline bool read() const {
        auto* sio = get_sio();
        if (!sio) return false;
        return (sio->GPIO_IN & pin_mask) != 0;
    }
};

// Verify that our implementation satisfies the GPIO pin concept
static_assert(alloy::hal::GpioPin<GpioPin<0>>,
              "RP2040 GpioPin must satisfy the GpioPin concept");

/// Type alias for output pins (more readable)
template<uint8_t PIN>
using OutputPin = GpioPin<PIN>;

/// Type alias for input pins (more readable)
template<uint8_t PIN>
using InputPin = GpioPin<PIN>;

} // namespace alloy::hal::rp2040

#endif // ALLOY_HAL_RP2040_GPIO_HPP
