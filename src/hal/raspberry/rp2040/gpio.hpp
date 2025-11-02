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

/// Get IO_BANK0 base address for GPIO function selection
inline volatile uint32_t* get_io_bank0_ctrl(uint8_t pin) {
    constexpr uint32_t IO_BANK0_BASE = 0x40014000;
    // Each GPIO has 8 bytes: STATUS (4 bytes) + CTRL (4 bytes)
    return reinterpret_cast<volatile uint32_t*>(IO_BANK0_BASE + 0x004 + (pin * 8));
}

/// Get PADS_BANK0 register for pad configuration
inline volatile uint32_t* get_pads_bank0(uint8_t pin) {
    constexpr uint32_t PADS_BANK0_BASE = 0x4001C000;
    // GPIO0-29: offset 0x04 + (pin * 4)
    return reinterpret_cast<volatile uint32_t*>(PADS_BANK0_BASE + 0x04 + (pin * 4));
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
    /// @note RP2040 GPIO configuration requires:
    ///       1. PAD configuration (drive strength, pulls)
    ///       2. IO_BANK0 function select (SIO = function 5)
    ///       3. SIO output enable
    inline void configure(PinMode mode) {
        auto* sio = get_sio();
        if (!sio) return;

        // Get IO_BANK0 and PADS registers
        volatile uint32_t* io_ctrl = get_io_bank0_ctrl(PIN);
        volatile uint32_t* pads = get_pads_bank0(PIN);

        // Configure PAD (enable output, set drive strength)
        // IE=1 (input enable), OD=0 (not open-drain), SLEWFAST=0, DRIVE=0 (2mA)
        *pads = (1 << 6); // IE bit

        // Select SIO function (function 5) in IO_BANK0
        *io_ctrl = 5; // FUNCSEL = 5 (SIO controlled GPIO)

        switch (mode) {
            case PinMode::Input:
                // Clear output enable = input mode
                sio->GPIO_OE_CLR = pin_mask;
                // Clear pulls in PAD
                *pads = (1 << 6) | (1 << 7); // IE=1, SCHMITT=1
                break;

            case PinMode::InputPullUp:
                // Clear output enable for input
                sio->GPIO_OE_CLR = pin_mask;
                // Enable pull-up
                *pads = (1 << 6) | (1 << 7) | (1 << 3); // IE=1, SCHMITT=1, PUE=1
                break;

            case PinMode::InputPullDown:
                // Clear output enable for input
                sio->GPIO_OE_CLR = pin_mask;
                // Enable pull-down
                *pads = (1 << 6) | (1 << 7) | (1 << 2); // IE=1, SCHMITT=1, PDE=1
                break;

            case PinMode::Output:
                // Configure PAD for output
                *pads = (1 << 6) | (1 << 7); // IE=1, SCHMITT=1
                // Set output enable
                sio->GPIO_OE_SET = pin_mask;
                break;

            case PinMode::Alternate:
                // Clear output enable (alternate function controlled by IO_BANK0)
                sio->GPIO_OE_CLR = pin_mask;
                // Function selection would be done via io_ctrl with different FUNCSEL
                break;

            case PinMode::Analog:
                // Clear output enable
                sio->GPIO_OE_CLR = pin_mask;
                // Set function to NULL (9) to disconnect digital path
                *io_ctrl = 0x1f; // NULL function
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
