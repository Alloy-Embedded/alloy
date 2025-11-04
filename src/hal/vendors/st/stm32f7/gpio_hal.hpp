/**
 * GPIO HAL for STM32F4
 *
 * Generic GPIO abstraction that works with generated hardware definitions.
 * Uses hardware.hpp for register addresses (generated from SVD).
 *
 * STM32F4 uses different GPIO registers than STM32F1:
 * - MODER: Mode register (input, output, AF, analog)
 * - OTYPER: Output type register (push-pull, open-drain)
 * - OSPEEDR: Output speed register
 * - PUPDR: Pull-up/pull-down register
 * - IDR: Input data register
 * - ODR: Output data register
 * - BSRR: Bit set/reset register
 * - AFR[2]: Alternate function registers (AFRL, AFRH)
 */

#pragma once

#include <cstdint>

namespace alloy::hal::stm32f4 {

/**
 * GPIO Pin HAL for STM32F4
 *
 * Template-based pin control using hardware addresses from SVD.
 * Usage: Include the generated gpio.hpp for your MCU variant.
 */
template<typename Hardware, uint8_t Pin>
class GPIOPin {
public:
    // Extract port and pin number from pin value
    static constexpr uint8_t PORT = Pin / 16;      // 0=A, 1=B, 2=C, 3=D, 4=E, etc
    static constexpr uint8_t PIN_NUM = Pin % 16;   // 0-15
    static constexpr uint8_t AFR_INDEX = PIN_NUM / 8;     // 0=AFRL, 1=AFRH
    static constexpr uint8_t AFR_SHIFT = (PIN_NUM % 8) * 4;

    // Get port register (using generated hardware addresses)
    static typename Hardware::GPIO_Registers* port() {
        switch (PORT) {
            case 0: return Hardware::GPIOA;
            case 1: return Hardware::GPIOB;
            case 2: return Hardware::GPIOC;
            case 3: return Hardware::GPIOD;
            case 4: return Hardware::GPIOE;
            case 5: return Hardware::GPIOF;
            case 6: return Hardware::GPIOG;
            case 7: return Hardware::GPIOH;
            case 8: return Hardware::GPIOI;
            default: return nullptr;
        }
    }

    // Enable clock for this port
    static void enableClock() {
        constexpr uint32_t clock_bit = (1U << PORT);  // GPIOAEN is bit 0
        Hardware::RCC->AHB1ENR |= clock_bit;
    }

    // Configure pin mode (0=input, 1=output, 2=alternate function, 3=analog)
    static void setMode(uint8_t mode) {
        enableClock();
        auto* gpio = port();
        const uint32_t shift = PIN_NUM * 2;
        gpio->MODER = (gpio->MODER & ~(0x3U << shift)) | ((mode & 0x3) << shift);
    }

    // Configure output type (0=push-pull, 1=open-drain)
    static void setOutputType(bool openDrain) {
        auto* gpio = port();
        if (openDrain) {
            gpio->OTYPER |= (1U << PIN_NUM);
        } else {
            gpio->OTYPER &= ~(1U << PIN_NUM);
        }
    }

    // Configure output speed (0=low, 1=medium, 2=high, 3=very high)
    static void setSpeed(uint8_t speed) {
        auto* gpio = port();
        const uint32_t shift = PIN_NUM * 2;
        gpio->OSPEEDR = (gpio->OSPEEDR & ~(0x3U << shift)) | ((speed & 0x3) << shift);
    }

    // Configure pull-up/pull-down (0=none, 1=pull-up, 2=pull-down)
    static void setPullUpDown(uint8_t pupd) {
        auto* gpio = port();
        const uint32_t shift = PIN_NUM * 2;
        gpio->PUPDR = (gpio->PUPDR & ~(0x3U << shift)) | ((pupd & 0x3) << shift);
    }

    // Configure alternate function (AF0-AF15)
    static void setAlternateFunction(uint8_t af) {
        auto* gpio = port();
        gpio->AFR[AFR_INDEX] = (gpio->AFR[AFR_INDEX] & ~(0xFU << AFR_SHIFT)) | ((af & 0xF) << AFR_SHIFT);
    }

    // Configure as input
    static void configureInput(uint8_t pupd = 0) {  // 0 = No pull-up/down
        setMode(0);  // Input mode
        setPullUpDown(pupd);
    }

    // Configure as output (push-pull, high speed)
    static void configureOutput(bool openDrain = false, uint8_t speed = 2) {  // 2 = High speed
        setMode(1);  // Output mode
        setOutputType(openDrain);
        setSpeed(speed);
    }

    // Configure as analog input (for ADC)
    static void configureAnalog() {
        setMode(3);  // Analog mode
    }

    // Configure as alternate function (for UART, SPI, I2C, PWM)
    static void configureAlternateFunction(uint8_t af, bool openDrain = false, uint8_t speed = 2) {
        setMode(2);  // Alternate function mode
        setOutputType(openDrain);
        setSpeed(speed);
        setAlternateFunction(af);
    }

    // Set pin high
    static void set() {
        port()->BSRR = (1U << PIN_NUM);  // Set bit
    }

    // Set pin low
    static void reset() {
        port()->BSRR = (1U << (PIN_NUM + 16));  // Reset bit
    }

    // Toggle pin
    static void toggle() {
        if (read()) {
            reset();
        } else {
            set();
        }
    }

    // Read pin state
    static bool read() {
        return (port()->IDR & (1U << PIN_NUM)) != 0;
    }

    // Write pin state
    static void write(bool value) {
        if (value) {
            set();
        } else {
            reset();
        }
    }
};

} // namespace alloy::hal::stm32f4
