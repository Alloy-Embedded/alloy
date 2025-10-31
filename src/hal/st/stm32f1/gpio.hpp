#ifndef ALLOY_HAL_STM32F1_GPIO_HPP
#define ALLOY_HAL_STM32F1_GPIO_HPP

#include "../../interface/gpio.hpp"
#include <cstdint>
#include <peripherals.hpp>

namespace alloy::hal::stm32f1 {

/// MCU-specific namespace concept
/// Validates that a namespace contains the required generated peripherals
template<typename McuNamespace>
concept HasStm32F1Peripherals = requires {
    typename McuNamespace::gpio::Registers;
    typename McuNamespace::rcc::Registers;
    { McuNamespace::traits::num_gpio_instances } -> std::convertible_to<uint32_t>;
};

/// Default to the configured MCU namespace (set by CMake)
/// This can be overridden by boards that need a different variant
namespace mcu = alloy::generated::stm32f103c8;

/// GPIO port identifiers
enum class Port : uint8_t {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
    F = 5,
    G = 6
};

/// GPIO Port Concept - validates that a type can be used as a GPIO port
template<typename T>
concept GpioPortType = requires {
    typename T::Registers;
} && requires(typename T::Registers* regs) {
    { regs->CRL } -> std::convertible_to<uint32_t>;
    { regs->CRH } -> std::convertible_to<uint32_t>;
    { regs->IDR } -> std::convertible_to<uint32_t>;
    { regs->ODR } -> std::convertible_to<uint32_t>;
    { regs->BSRR } -> std::convertible_to<uint32_t>;
};

/// Get GPIO port registers from port enum
/// This uses the generated peripheral definitions from the MCU namespace
inline mcu::gpio::Registers* get_gpio_port(Port port) {
    using namespace mcu::gpio;

    switch (port) {
        case Port::A: return GPIOA;
        case Port::B: return GPIOB;
        case Port::C: return GPIOC;
        case Port::D: return GPIOD;
        case Port::E: return GPIOE;
        case Port::F: return GPIOF;
        case Port::G: return GPIOG;
    }
    return nullptr;
}

/// Enable clock for GPIO port (uses generated RCC peripheral)
inline void enable_gpio_clock(Port port) {
    using namespace mcu::rcc;

    // GPIOA=bit 2, GPIOB=bit 3, GPIOC=bit 4, etc. in APB2ENR
    uint32_t bit = 2 + static_cast<uint8_t>(port);
    RCC->APB2ENR |= (1U << bit);
}

/// Extract port from pin number
/// STM32F1 uses format: Port * 16 + Pin
/// PA0=0, PA1=1, ..., PA15=15, PB0=16, PB1=17, ..., PC13=45, etc.
constexpr inline Port get_port_from_pin(uint8_t pin) {
    return static_cast<Port>(pin / 16);
}

/// Extract pin bit (0-15) from pin number
constexpr inline uint8_t get_pin_bit(uint8_t pin) {
    return pin % 16;
}

/// Modern C++20 GPIO Pin Implementation
///
/// This class template provides compile-time GPIO pin configuration
/// using automatically generated peripheral definitions.
///
/// Features:
/// - Compile-time pin validation using concepts
/// - Zero-cost abstractions (everything inline/constexpr)
/// - Uses generated peripheral registers
/// - Type-safe API
///
/// Template Parameters:
/// - PIN: Pin number (0-based: PA0=0, PA1=1, ..., PC13=45, etc.)
///
/// Example usage:
/// \code
/// // Create LED pin (PC13 on Blue Pill = pin 45)
/// GpioPin<45> led;
/// led.configure(PinMode::Output);
/// led.set_high();
/// led.toggle();
/// \endcode
template<uint8_t PIN>
class GpioPin {
public:
    // Compile-time constants
    static constexpr uint8_t pin_number = PIN;
    static constexpr Port port = get_port_from_pin(PIN);
    static constexpr uint8_t pin_bit = get_pin_bit(PIN);

    // Compile-time validation using generated traits from MCU namespace
    static_assert(PIN < 128, "Pin number must be < 128 (max 8 ports * 16 pins)");
    static_assert(static_cast<uint8_t>(port) < mcu::traits::num_gpio_instances,
                  "GPIO port not available on this MCU");

    /// Constructor - enables port clock automatically
    GpioPin() {
        enable_gpio_clock(port);
    }

    /// Configure pin mode
    /// @param mode Desired pin mode (Input, Output, Analog, etc.)
    inline void configure(PinMode mode) {
        auto* gpio_regs = get_gpio_port(port);
        if (!gpio_regs) return;

        // STM32F1 uses 4 bits per pin: [CNF1 CNF0 MODE1 MODE0]
        // MODE: 00=Input, 01=Output 10MHz, 10=Output 2MHz, 11=Output 50MHz
        // CNF: depends on MODE (see datasheet)
        uint32_t config = 0;

        switch (mode) {
            case PinMode::Input:
                // MODE=00, CNF=01 (floating input)
                config = 0b0100;
                break;

            case PinMode::InputPullUp:
            case PinMode::InputPullDown:
                // MODE=00, CNF=10 (input with pull-up/down)
                config = 0b1000;
                break;

            case PinMode::Output:
                // MODE=11 (50MHz), CNF=00 (push-pull output)
                config = 0b0011;
                break;

            case PinMode::Alternate:
                // MODE=11 (50MHz), CNF=10 (alternate function push-pull)
                config = 0b1011;
                break;

            case PinMode::Analog:
                // MODE=00, CNF=00 (analog mode)
                config = 0b0000;
                break;
        }

        // Configure CRL (pins 0-7) or CRH (pins 8-15)
        if (pin_bit < 8) {
            uint32_t shift = pin_bit * 4;
            gpio_regs->CRL &= ~(0xFU << shift);
            gpio_regs->CRL |= (config << shift);
        } else {
            uint32_t shift = (pin_bit - 8) * 4;
            gpio_regs->CRH &= ~(0xFU << shift);
            gpio_regs->CRH |= (config << shift);
        }

        // Configure pull-up/down via ODR register
        if (mode == PinMode::InputPullUp) {
            gpio_regs->ODR |= (1U << pin_bit);   // Set = pull-up
        } else if (mode == PinMode::InputPullDown) {
            gpio_regs->ODR &= ~(1U << pin_bit);  // Clear = pull-down
        }
    }

    /// Set pin HIGH (using BSRR for atomic operation)
    inline void set_high() {
        auto* gpio_regs = get_gpio_port(port);
        if (!gpio_regs) return;
        gpio_regs->BSRR = (1U << pin_bit);  // Bit Set
    }

    /// Set pin LOW (using BSRR for atomic operation)
    inline void set_low() {
        auto* gpio_regs = get_gpio_port(port);
        if (!gpio_regs) return;
        gpio_regs->BSRR = (1U << (pin_bit + 16));  // Bit Reset
    }

    /// Toggle pin output state
    inline void toggle() {
        auto* gpio_regs = get_gpio_port(port);
        if (!gpio_regs) return;
        gpio_regs->ODR ^= (1U << pin_bit);
    }

    /// Read pin input state
    /// @return true if pin is HIGH, false if LOW
    inline bool read() const {
        auto* gpio_regs = get_gpio_port(port);
        if (!gpio_regs) return false;
        return (gpio_regs->IDR & (1U << pin_bit)) != 0;
    }
};

// Verify that our implementation satisfies the GPIO pin concept
static_assert(alloy::hal::GpioPin<GpioPin<0>>,
              "STM32F1 GpioPin must satisfy the GpioPin concept");

/// Type alias for output pins (more readable)
template<uint8_t PIN>
using OutputPin = GpioPin<PIN>;

/// Type alias for input pins (more readable)
template<uint8_t PIN>
using InputPin = GpioPin<PIN>;

} // namespace alloy::hal::stm32f1

#endif // ALLOY_HAL_STM32F1_GPIO_HPP
