#ifndef ALLOY_HAL_STM32F4_GPIO_HPP
#define ALLOY_HAL_STM32F4_GPIO_HPP

#include "../../interface/gpio.hpp"
#include <cstdint>
#include <peripherals.hpp>

namespace alloy::hal::stm32f4 {

/// MCU-specific namespace concept
/// Validates that a namespace contains the required generated peripherals
template<typename McuNamespace>
concept HasStm32F4Peripherals = requires {
    typename McuNamespace::gpio::Registers;
    typename McuNamespace::rcc::Registers;
    { McuNamespace::traits::num_gpio_instances } -> std::convertible_to<uint32_t>;
};

/// Default to the configured MCU namespace (set by CMake)
/// This can be overridden by boards that need a different variant
namespace mcu = alloy::generated::stm32f407;

/// GPIO port identifiers
enum class Port : uint8_t {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
    F = 5,
    G = 6,
    H = 7,
    I = 8,
    J = 9,
    K = 10
};

/// GPIO Port Concept - validates that a type can be used as a GPIO port
template<typename T>
concept GpioPortType = requires {
    typename T::Registers;
} && requires(typename T::Registers* regs) {
    { regs->MODER } -> std::convertible_to<uint32_t>;
    { regs->OTYPER } -> std::convertible_to<uint32_t>;
    { regs->OSPEEDR } -> std::convertible_to<uint32_t>;
    { regs->PUPDR } -> std::convertible_to<uint32_t>;
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
        case Port::H: return GPIOH;
        case Port::I: return GPIOI;
        case Port::J: return GPIOJ;
        case Port::K: return GPIOK;
    }
    return nullptr;
}

/// Enable clock for GPIO port (uses generated RCC peripheral)
inline void enable_gpio_clock(Port port) {
    using namespace mcu::rcc;

    // GPIOA=bit 0, GPIOB=bit 1, GPIOC=bit 2, etc. in AHB1ENR
    uint32_t bit = static_cast<uint8_t>(port);
    RCC->AHB1ENR |= (1U << bit);
}

/// Extract port from pin number
/// STM32F4 uses format: Port * 16 + Pin
/// PA0=0, PA1=1, ..., PA15=15, PB0=16, PB1=17, ..., PD12=60, etc.
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
/// - PIN: Pin number (0-based: PA0=0, PA1=1, ..., PD12=60, etc.)
///
/// Example usage:
/// \code
/// // Create LED pin (PD12 on STM32F407 Discovery = pin 60)
/// GpioPin<60> led;
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
    static_assert(PIN < 176, "Pin number must be < 176 (max 11 ports * 16 pins)");
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

        // STM32F4 uses separate registers with 2 bits per pin
        // MODER: 00=Input, 01=Output, 10=Alternate, 11=Analog
        // OTYPER: 0=Push-pull, 1=Open-drain
        // OSPEEDR: 00=Low, 01=Medium, 10=Fast, 11=High speed
        // PUPDR: 00=None, 01=Pull-up, 10=Pull-down, 11=Reserved

        uint32_t moder_val = 0;
        uint32_t otyper_val = 0;
        uint32_t ospeedr_val = 0b11;  // Default to high speed
        uint32_t pupdr_val = 0;

        switch (mode) {
            case PinMode::Input:
                // MODER=00, no pull-up/down
                moder_val = 0b00;
                pupdr_val = 0b00;
                break;

            case PinMode::InputPullUp:
                // MODER=00, PUPDR=01 (pull-up)
                moder_val = 0b00;
                pupdr_val = 0b01;
                break;

            case PinMode::InputPullDown:
                // MODER=00, PUPDR=10 (pull-down)
                moder_val = 0b00;
                pupdr_val = 0b10;
                break;

            case PinMode::Output:
                // MODER=01, OTYPER=0 (push-pull)
                moder_val = 0b01;
                otyper_val = 0;
                break;

            case PinMode::Alternate:
                // MODER=10, OTYPER=0 (push-pull)
                moder_val = 0b10;
                otyper_val = 0;
                break;

            case PinMode::Analog:
                // MODER=11
                moder_val = 0b11;
                break;
        }

        // Configure MODER (2 bits per pin)
        uint32_t moder_shift = pin_bit * 2;
        gpio_regs->MODER &= ~(0x3U << moder_shift);
        gpio_regs->MODER |= (moder_val << moder_shift);

        // Configure OTYPER (1 bit per pin) - only for output modes
        if (mode == PinMode::Output || mode == PinMode::Alternate) {
            if (otyper_val) {
                gpio_regs->OTYPER |= (1U << pin_bit);
            } else {
                gpio_regs->OTYPER &= ~(1U << pin_bit);
            }
        }

        // Configure OSPEEDR (2 bits per pin) - only for output modes
        if (mode == PinMode::Output || mode == PinMode::Alternate) {
            uint32_t ospeedr_shift = pin_bit * 2;
            gpio_regs->OSPEEDR &= ~(0x3U << ospeedr_shift);
            gpio_regs->OSPEEDR |= (ospeedr_val << ospeedr_shift);
        }

        // Configure PUPDR (2 bits per pin)
        uint32_t pupdr_shift = pin_bit * 2;
        gpio_regs->PUPDR &= ~(0x3U << pupdr_shift);
        gpio_regs->PUPDR |= (pupdr_val << pupdr_shift);
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
              "STM32F4 GpioPin must satisfy the GpioPin concept");

/// Type alias for output pins (more readable)
template<uint8_t PIN>
using OutputPin = GpioPin<PIN>;

/// Type alias for input pins (more readable)
template<uint8_t PIN>
using InputPin = GpioPin<PIN>;

} // namespace alloy::hal::stm32f4

#endif // ALLOY_HAL_STM32F4_GPIO_HPP
