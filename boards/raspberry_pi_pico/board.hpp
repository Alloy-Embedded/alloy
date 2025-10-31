/// Raspberry Pi Pico Board Definition
///
/// Modern C++20 board support with pre-instantiated peripherals.
/// This file defines board-specific pins and peripherals that users
/// can access directly without needing to know pin numbers.
///
/// Inspired by modm's board abstraction approach.

#ifndef ALLOY_BOARD_RASPBERRY_PI_PICO_HPP
#define ALLOY_BOARD_RASPBERRY_PI_PICO_HPP

#include "hal/raspberry/rp2040/gpio.hpp"
#include <cstdint>

namespace Board {

/// Board identification
inline constexpr const char* name = "Raspberry Pi Pico";
inline constexpr const char* mcu = "RP2040";

/// Clock configuration
inline constexpr uint32_t system_clock_hz = 125'000'000;  // 125 MHz (default)
inline constexpr uint32_t xosc_frequency_hz = 12'000'000; // 12 MHz external crystal

//
// Pre-defined GPIO Pins
//
// These are the most commonly used pins on the Raspberry Pi Pico board.
// Users can access them directly: Board::Led, Board::Uart0Tx, etc.
//
// Pin numbering: RP2040 uses direct GPIO numbers (GPIO0-GPIO29)
//

namespace detail {
    using namespace alloy::hal::rp2040;

    // On-board LED (GPIO25)
    inline constexpr uint8_t LED_PIN = 25;

    // UART0 pins (default serial port)
    inline constexpr uint8_t UART0_TX_PIN = 0;   // GP0
    inline constexpr uint8_t UART0_RX_PIN = 1;   // GP1

    // UART1 pins (alternative serial port)
    inline constexpr uint8_t UART1_TX_PIN = 4;   // GP4
    inline constexpr uint8_t UART1_RX_PIN = 5;   // GP5

    // I2C0 pins (default I2C)
    inline constexpr uint8_t I2C0_SDA_PIN = 4;   // GP4
    inline constexpr uint8_t I2C0_SCL_PIN = 5;   // GP5

    // I2C1 pins (alternative I2C)
    inline constexpr uint8_t I2C1_SDA_PIN = 6;   // GP6
    inline constexpr uint8_t I2C1_SCL_PIN = 7;   // GP7

    // SPI0 pins (default SPI)
    inline constexpr uint8_t SPI0_MISO_PIN = 16;  // GP16
    inline constexpr uint8_t SPI0_CS_PIN   = 17;  // GP17
    inline constexpr uint8_t SPI0_SCK_PIN  = 18;  // GP18
    inline constexpr uint8_t SPI0_MOSI_PIN = 19;  // GP19

    // SPI1 pins (alternative SPI)
    inline constexpr uint8_t SPI1_MISO_PIN = 12;  // GP12
    inline constexpr uint8_t SPI1_CS_PIN   = 13;  // GP13
    inline constexpr uint8_t SPI1_SCK_PIN  = 14;  // GP14
    inline constexpr uint8_t SPI1_MOSI_PIN = 15;  // GP15

    // ADC pins (ADC0-ADC3 on GPIO26-29)
    inline constexpr uint8_t ADC0_PIN = 26;  // GP26 / ADC0
    inline constexpr uint8_t ADC1_PIN = 27;  // GP27 / ADC1
    inline constexpr uint8_t ADC2_PIN = 28;  // GP28 / ADC2
    // GPIO29 (ADC3) is connected to VSYS/3 on Pico
}

//
// Pre-instantiated GPIO Pins
//
// These are type aliases that users can instantiate.
// They provide compile-time pin configuration.
//

/// On-board LED (GPIO25, active HIGH)
/// Usage: Board::LedGreen led; led.configure(PinMode::Output);
using LedGreen = alloy::hal::rp2040::GpioPin<detail::LED_PIN>;

/// UART0 TX (GP0)
using Uart0Tx = alloy::hal::rp2040::GpioPin<detail::UART0_TX_PIN>;

/// UART0 RX (GP1)
using Uart0Rx = alloy::hal::rp2040::GpioPin<detail::UART0_RX_PIN>;

/// UART1 TX (GP4)
using Uart1Tx = alloy::hal::rp2040::GpioPin<detail::UART1_TX_PIN>;

/// UART1 RX (GP5)
using Uart1Rx = alloy::hal::rp2040::GpioPin<detail::UART1_RX_PIN>;

/// I2C0 SDA (GP4)
using I2c0Sda = alloy::hal::rp2040::GpioPin<detail::I2C0_SDA_PIN>;

/// I2C0 SCL (GP5)
using I2c0Scl = alloy::hal::rp2040::GpioPin<detail::I2C0_SCL_PIN>;

/// I2C1 SDA (GP6)
using I2c1Sda = alloy::hal::rp2040::GpioPin<detail::I2C1_SDA_PIN>;

/// I2C1 SCL (GP7)
using I2c1Scl = alloy::hal::rp2040::GpioPin<detail::I2C1_SCL_PIN>;

/// SPI0 MISO (GP16)
using Spi0Miso = alloy::hal::rp2040::GpioPin<detail::SPI0_MISO_PIN>;

/// SPI0 CS (GP17)
using Spi0Cs = alloy::hal::rp2040::GpioPin<detail::SPI0_CS_PIN>;

/// SPI0 SCK (GP18)
using Spi0Sck = alloy::hal::rp2040::GpioPin<detail::SPI0_SCK_PIN>;

/// SPI0 MOSI (GP19)
using Spi0Mosi = alloy::hal::rp2040::GpioPin<detail::SPI0_MOSI_PIN>;

/// SPI1 MISO (GP12)
using Spi1Miso = alloy::hal::rp2040::GpioPin<detail::SPI1_MISO_PIN>;

/// SPI1 CS (GP13)
using Spi1Cs = alloy::hal::rp2040::GpioPin<detail::SPI1_CS_PIN>;

/// SPI1 SCK (GP14)
using Spi1Sck = alloy::hal::rp2040::GpioPin<detail::SPI1_SCK_PIN>;

/// SPI1 MOSI (GP15)
using Spi1Mosi = alloy::hal::rp2040::GpioPin<detail::SPI1_MOSI_PIN>;

/// ADC0 (GP26)
using Adc0 = alloy::hal::rp2040::GpioPin<detail::ADC0_PIN>;

/// ADC1 (GP27)
using Adc1 = alloy::hal::rp2040::GpioPin<detail::ADC1_PIN>;

/// ADC2 (GP28)
using Adc2 = alloy::hal::rp2040::GpioPin<detail::ADC2_PIN>;

//
// Helper Functions
//

/// Initialize board (can be called before main)
inline void initialize() {
    // Future: Initialize clocks, enable peripherals, etc.
}

/// Simple delay (busy wait - not accurate, for basic use only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(uint32_t ms) {
    // Very rough estimate: at 125MHz system clock
    // Assuming ~4 cycles per loop iteration
    volatile uint32_t count = ms * 31250;
    while (count--) {
        __asm__ volatile("nop");
    }
}

//
// Board-specific LED helpers
//

/// LED control namespace
namespace Led {
    /// Turn LED on (active HIGH on Pico)
    inline void on() {
        static LedGreen led;
        led.set_high();
    }

    /// Turn LED off (active HIGH on Pico)
    inline void off() {
        static LedGreen led;
        led.set_low();
    }

    /// Toggle LED state
    inline void toggle() {
        static LedGreen led;
        led.toggle();
    }

    /// Initialize LED as output
    inline void init() {
        static LedGreen led;
        led.configure(alloy::hal::PinMode::Output);
        off();  // Start with LED off
    }
}

} // namespace Board

#endif // ALLOY_BOARD_RASPBERRY_PI_PICO_HPP
