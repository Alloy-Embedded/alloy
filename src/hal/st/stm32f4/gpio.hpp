/// STM32F4 GPIO Implementation
///
/// STM32F4 uses MODER registers (different from STM32F1's CRL/CRH)
/// Pin numbering: PA0=0, PA1=1, ..., PE15=79, etc.

#ifndef ALLOY_HAL_ST_STM32F4_GPIO_HPP
#define ALLOY_HAL_ST_STM32F4_GPIO_HPP

#include "hal/interface/gpio.hpp"
#include "generated/unknown/stm32f4/stm32f407/peripherals.hpp"
#include <cstdint>

namespace alloy::hal::st::stm32f4 {

// Import generated peripherals
using namespace alloy::generated::stm32f407;

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
    I = 8
};

/// Extract port and pin from pin number
/// STM32F4 uses format: Port * 16 + Pin
/// PA0=0, PA1=1, ..., PA15=15, PB0=16, PB1=17, ..., PD12=60, etc.
constexpr inline Port get_port(uint8_t pin) {
    return static_cast<Port>(pin / 16);
}

constexpr inline uint8_t get_pin_number(uint8_t pin) {
    return pin % 16;
}

/// STM32F4 GPIO pin implementation
/// Pin numbering: PA0=0, PA1=1, ..., PD12=60 (for LEDs on Discovery), etc.
template<uint8_t PIN>
class GpioPin {
public:
    static constexpr uint8_t pin_number = PIN;
    static constexpr Port port = get_port(PIN);
    static constexpr uint8_t pin_bit = get_pin_number(PIN);

    // Compile-time validation
    static_assert(PIN < 144, "GPIO pin must be less than 144 (9 ports * 16 pins)");

    /// Constructor
    GpioPin() = default;

    /// Configure pin mode
    void configure(hal::PinMode mode) {
        // Enable port clock first
        enable_port_clock();

        // Configure mode in MODER register
        if (mode == hal::PinMode::Output) {
            set_mode_output();
        } else if (mode == hal::PinMode::Input) {
            set_mode_input();
        }
    }

    /// Set pin to HIGH
    void set_high() {
        // Use BSRR (Bit Set/Reset Register) - atomic operation
        // Writing to BS[pin] sets the pin
        auto gpio = get_gpio_registers();
        gpio->BSRR = (1U << pin_bit);
    }

    /// Set pin to LOW
    void set_low() {
        // Use BSRR - BR[pin] resets the pin
        auto gpio = get_gpio_registers();
        gpio->BSRR = (1U << (pin_bit + 16));
    }

    /// Toggle pin
    void toggle() {
        auto gpio = get_gpio_registers();
        // Read ODR and toggle the bit
        if (gpio->ODR & (1U << pin_bit)) {
            set_low();
        } else {
            set_high();
        }
    }

    /// Read pin state
    bool read() const {
        auto gpio = get_gpio_registers();
        return (gpio->IDR & (1U << pin_bit)) != 0;
    }

private:
    /// Enable GPIO port clock
    static void enable_port_clock() {
        // Enable clock in RCC->AHB1ENR
        // GPIOA=bit0, GPIOB=bit1, GPIOC=bit2, etc.
        uint32_t port_bit = static_cast<uint32_t>(port);
        rcc::RCC->AHB1ENR |= (1U << port_bit);
    }

    /// Set pin mode to output (MODER = 0b01)
    static void set_mode_output() {
        auto gpio = get_gpio_registers();
        uint32_t moder = gpio->MODER;
        moder &= ~(0x3U << (pin_bit * 2));  // Clear mode bits
        moder |= (0x1U << (pin_bit * 2));   // Set mode = 0b01 (output)
        gpio->MODER = moder;

        // Set output type to push-pull (OTYPER bit = 0)
        gpio->OTYPER &= ~(1U << pin_bit);

        // Set output speed to low (OSPEEDR = 0b00)
        gpio->OSPEEDR &= ~(0x3U << (pin_bit * 2));
    }

    /// Set pin mode to input (MODER = 0b00)
    static void set_mode_input() {
        auto gpio = get_gpio_registers();
        uint32_t moder = gpio->MODER;
        moder &= ~(0x3U << (pin_bit * 2));  // Clear mode bits (0b00 = input)
        gpio->MODER = moder;

        // Set pull-up/pull-down to none (PUPDR = 0b00)
        gpio->PUPDR &= ~(0x3U << (pin_bit * 2));
    }

    /// Get pointer to GPIO port registers
    static auto get_gpio_registers() {
        // GPIO bases: GPIOA=0x40020000, GPIOB=0x40020400, etc. (0x400 offset)
        constexpr uint32_t GPIO_BASE = 0x40020000;
        constexpr uint32_t GPIO_OFFSET = 0x400;
        uint32_t port_base = GPIO_BASE + (static_cast<uint32_t>(port) * GPIO_OFFSET);
        return reinterpret_cast<gpio::Registers*>(port_base);
    }
};

// Static assertion to verify concept compliance
static_assert(alloy::hal::GpioPin<GpioPin<0>>,
              "STM32F4 GpioPin must satisfy GpioPin concept");

} // namespace alloy::hal::st::stm32f4

#endif // ALLOY_HAL_ST_STM32F4_GPIO_HPP
