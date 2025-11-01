/// STM32F103C8 Blue Pill Board Definition
///
/// Modern C++20 board support with pre-instantiated peripherals.
/// This file defines board-specific pins and peripherals that users
/// can access directly without needing to know pin numbers.
///
/// Inspired by modm's board abstraction approach.

#ifndef ALLOY_BOARD_STM32F103C8_HPP
#define ALLOY_BOARD_STM32F103C8_HPP

#include "hal/st/stm32f1/gpio.hpp"
#include "hal/st/stm32f1/clock.hpp"
#include "hal/st/stm32f1/delay.hpp"
#include "hal/st/stm32f1/systick.hpp"
#include <cstdint>

namespace Board {

/// Board identification
inline constexpr const char* name = "STM32F103C8 Blue Pill";
inline constexpr const char* mcu = "STM32F103C8T6";

/// Clock configuration
inline constexpr uint32_t system_clock_hz = 72'000'000;  // 72 MHz (with PLL)
inline constexpr uint32_t hse_frequency_hz = 8'000'000;  // 8 MHz external crystal

//
// Pre-defined GPIO Pins
//
// These are the most commonly used pins on the Blue Pill board.
// Users can access them directly: Board::LedBlue, Board::UsbDm, etc.
//

namespace detail {
    using namespace alloy::hal::stm32f1;

    /// Pin number calculation: Port * 16 + Pin
    /// PA0 = 0*16 + 0 = 0
    /// PB1 = 1*16 + 1 = 17
    /// PC13 = 2*16 + 13 = 45

    // LED (active LOW)
    inline constexpr uint8_t LED_PIN = 45;  // PC13

    // USB pins
    inline constexpr uint8_t USB_DM_PIN = 43;  // PA11
    inline constexpr uint8_t USB_DP_PIN = 44;  // PA12

    // USART1 pins
    inline constexpr uint8_t USART1_TX_PIN = 9;   // PA9
    inline constexpr uint8_t USART1_RX_PIN = 10;  // PA10

    // I2C1 pins
    inline constexpr uint8_t I2C1_SCL_PIN = 22;  // PB6
    inline constexpr uint8_t I2C1_SDA_PIN = 23;  // PB7

    // SPI1 pins
    inline constexpr uint8_t SPI1_SCK_PIN  = 5;   // PA5
    inline constexpr uint8_t SPI1_MISO_PIN = 6;   // PA6
    inline constexpr uint8_t SPI1_MOSI_PIN = 7;   // PA7
    inline constexpr uint8_t SPI1_NSS_PIN  = 4;   // PA4
}

//
// Pre-instantiated GPIO Pins
//
// These are type aliases that users can instantiate.
// They provide compile-time pin configuration.
//

/// On-board LED (PC13, active LOW)
/// Usage: Board::LedBlue led; led.configure(PinMode::Output);
using LedBlue = alloy::hal::stm32f1::GpioPin<detail::LED_PIN>;

/// USB Data Minus (PA11)
using UsbDm = alloy::hal::stm32f1::GpioPin<detail::USB_DM_PIN>;

/// USB Data Plus (PA12)
using UsbDp = alloy::hal::stm32f1::GpioPin<detail::USB_DP_PIN>;

/// USART1 TX (PA9)
using Usart1Tx = alloy::hal::stm32f1::GpioPin<detail::USART1_TX_PIN>;

/// USART1 RX (PA10)
using Usart1Rx = alloy::hal::stm32f1::GpioPin<detail::USART1_RX_PIN>;

/// I2C1 SCL (PB6)
using I2c1Scl = alloy::hal::stm32f1::GpioPin<detail::I2C1_SCL_PIN>;

/// I2C1 SDA (PB7)
using I2c1Sda = alloy::hal::stm32f1::GpioPin<detail::I2C1_SDA_PIN>;

/// SPI1 SCK (PA5)
using Spi1Sck = alloy::hal::stm32f1::GpioPin<detail::SPI1_SCK_PIN>;

/// SPI1 MISO (PA6)
using Spi1Miso = alloy::hal::stm32f1::GpioPin<detail::SPI1_MISO_PIN>;

/// SPI1 MOSI (PA7)
using Spi1Mosi = alloy::hal::stm32f1::GpioPin<detail::SPI1_MOSI_PIN>;

/// SPI1 NSS (PA4)
using Spi1Nss = alloy::hal::stm32f1::GpioPin<detail::SPI1_NSS_PIN>;

//
// Helper Functions
//

/// Initialize board (can be called before main)
inline void initialize() {
    using namespace alloy::hal::st::stm32f1;
    using alloy::hal::Peripheral;

    // Create static clock instance
    static SystemClock clock;

    // Configure system clock to 72MHz using HSE (8MHz crystal) + PLL
    auto result = clock.set_frequency(72'000'000);
    if (!result.is_ok()) {
        // Fallback to HSI if HSE fails
        clock.set_frequency(8'000'000);
    }

    // Initialize SysTick for microsecond time tracking
    SystemTick::init();

    // Enable GPIO clocks for all ports used on the board
    clock.enable_peripheral(Peripheral::GpioA);
    clock.enable_peripheral(Peripheral::GpioB);
    clock.enable_peripheral(Peripheral::GpioC);
}

/// Simple delay (busy wait - not accurate, for basic use only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(uint32_t ms) {
    alloy::hal::stm32f1::delay_ms(ms);
}

//
// Board-specific LED helpers
//

/// LED control namespace
namespace Led {
    /// Turn LED on (handles active-low automatically)
    inline void on() {
        static LedBlue led;
        led.set_low();  // Active LOW
    }

    /// Turn LED off (handles active-low automatically)
    inline void off() {
        static LedBlue led;
        led.set_high();  // Active LOW
    }

    /// Toggle LED state
    inline void toggle() {
        static LedBlue led;
        led.toggle();
    }

    /// Initialize LED as output
    inline void init() {
        static LedBlue led;
        led.configure(alloy::hal::PinMode::Output);
        off();  // Start with LED off
    }
}

} // namespace Board

#endif // ALLOY_BOARD_STM32F103C8_HPP
