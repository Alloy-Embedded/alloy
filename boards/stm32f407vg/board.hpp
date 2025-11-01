/// STM32F407VG Discovery Board Definition
///
/// Modern C++20 board support with pre-instantiated peripherals.
/// This file defines board-specific pins and peripherals that users
/// can access directly without needing to know pin numbers.
///
/// Inspired by modm's board abstraction approach.

#ifndef ALLOY_BOARD_STM32F407VG_HPP
#define ALLOY_BOARD_STM32F407VG_HPP

#include "hal/st/stm32f4/gpio.hpp"
#include "hal/st/stm32f4/clock.hpp"
#include "hal/st/stm32f4/delay.hpp"
#include "hal/st/stm32f4/systick.hpp"
#include <cstdint>

namespace Board {

/// Board identification
inline constexpr const char* name = "STM32F407VG Discovery";
inline constexpr const char* mcu = "STM32F407VGT6";

/// Clock configuration
inline constexpr uint32_t system_clock_hz = 168'000'000;  // 168 MHz (with PLL)
inline constexpr uint32_t hse_frequency_hz = 8'000'000;   // 8 MHz external crystal

//
// Pre-defined GPIO Pins
//
// These are the most commonly used pins on the STM32F407 Discovery board.
// Users can access them directly: Board::LedGreen, Board::Usart2Tx, etc.
//

namespace detail {
    using namespace alloy::hal::stm32f4;

    /// Pin number calculation: Port * 16 + Pin
    /// PA0 = 0*16 + 0 = 0
    /// PB1 = 1*16 + 1 = 17
    /// PD12 = 3*16 + 12 = 60

    // LEDs (active HIGH on STM32F407 Discovery)
    inline constexpr uint8_t LED_GREEN_PIN = 60;   // PD12
    inline constexpr uint8_t LED_ORANGE_PIN = 61;  // PD13
    inline constexpr uint8_t LED_RED_PIN = 62;     // PD14
    inline constexpr uint8_t LED_BLUE_PIN = 63;    // PD15

    // User Button (active LOW)
    inline constexpr uint8_t BUTTON_USER_PIN = 0;  // PA0

    // USART2 pins (connected to ST-Link VCP)
    inline constexpr uint8_t USART2_TX_PIN = 2;   // PA2
    inline constexpr uint8_t USART2_RX_PIN = 3;   // PA3

    // USART1 pins
    inline constexpr uint8_t USART1_TX_PIN = 9;   // PA9
    inline constexpr uint8_t USART1_RX_PIN = 10;  // PA10

    // I2C1 pins
    inline constexpr uint8_t I2C1_SCL_PIN = 22;  // PB6
    inline constexpr uint8_t I2C1_SDA_PIN = 25;  // PB9

    // I2C3 pins (connected to accelerometer and audio codec)
    inline constexpr uint8_t I2C3_SCL_PIN = 8;   // PA8
    inline constexpr uint8_t I2C3_SDA_PIN = 41;  // PC9

    // SPI1 pins
    inline constexpr uint8_t SPI1_SCK_PIN  = 5;   // PA5
    inline constexpr uint8_t SPI1_MISO_PIN = 6;   // PA6
    inline constexpr uint8_t SPI1_MOSI_PIN = 7;   // PA7
    inline constexpr uint8_t SPI1_NSS_PIN  = 4;   // PA4

    // SPI2 pins (connected to accelerometer)
    inline constexpr uint8_t SPI2_SCK_PIN  = 29;  // PB13
    inline constexpr uint8_t SPI2_MISO_PIN = 30;  // PB14
    inline constexpr uint8_t SPI2_MOSI_PIN = 31;  // PB15
    inline constexpr uint8_t SPI2_NSS_PIN  = 28;  // PB12

    // Accelerometer chip select (LIS3DSH)
    inline constexpr uint8_t ACCEL_CS_PIN = 51;  // PE3
}

//
// Pre-instantiated GPIO Pins
//
// These are type aliases that users can instantiate.
// They provide compile-time pin configuration.
//

/// On-board LED Green (PD12, active HIGH)
/// Usage: Board::LedGreen led; led.configure(PinMode::Output);
using LedGreen = alloy::hal::stm32f4::GpioPin<detail::LED_GREEN_PIN>;

/// On-board LED Orange (PD13, active HIGH)
using LedOrange = alloy::hal::stm32f4::GpioPin<detail::LED_ORANGE_PIN>;

/// On-board LED Red (PD14, active HIGH)
using LedRed = alloy::hal::stm32f4::GpioPin<detail::LED_RED_PIN>;

/// On-board LED Blue (PD15, active HIGH)
using LedBlue = alloy::hal::stm32f4::GpioPin<detail::LED_BLUE_PIN>;

/// User Button (PA0, active LOW - pressed = LOW)
using ButtonUser = alloy::hal::stm32f4::GpioPin<detail::BUTTON_USER_PIN>;

/// USART2 TX (PA2, connected to ST-Link VCP for printf)
using Usart2Tx = alloy::hal::stm32f4::GpioPin<detail::USART2_TX_PIN>;

/// USART2 RX (PA3, connected to ST-Link VCP)
using Usart2Rx = alloy::hal::stm32f4::GpioPin<detail::USART2_RX_PIN>;

/// USART1 TX (PA9)
using Usart1Tx = alloy::hal::stm32f4::GpioPin<detail::USART1_TX_PIN>;

/// USART1 RX (PA10)
using Usart1Rx = alloy::hal::stm32f4::GpioPin<detail::USART1_RX_PIN>;

/// I2C1 SCL (PB6)
using I2c1Scl = alloy::hal::stm32f4::GpioPin<detail::I2C1_SCL_PIN>;

/// I2C1 SDA (PB9)
using I2c1Sda = alloy::hal::stm32f4::GpioPin<detail::I2C1_SDA_PIN>;

/// I2C3 SCL (PA8, connected to on-board sensors)
using I2c3Scl = alloy::hal::stm32f4::GpioPin<detail::I2C3_SCL_PIN>;

/// I2C3 SDA (PC9, connected to on-board sensors)
using I2c3Sda = alloy::hal::stm32f4::GpioPin<detail::I2C3_SDA_PIN>;

/// SPI1 SCK (PA5)
using Spi1Sck = alloy::hal::stm32f4::GpioPin<detail::SPI1_SCK_PIN>;

/// SPI1 MISO (PA6)
using Spi1Miso = alloy::hal::stm32f4::GpioPin<detail::SPI1_MISO_PIN>;

/// SPI1 MOSI (PA7)
using Spi1Mosi = alloy::hal::stm32f4::GpioPin<detail::SPI1_MOSI_PIN>;

/// SPI1 NSS (PA4)
using Spi1Nss = alloy::hal::stm32f4::GpioPin<detail::SPI1_NSS_PIN>;

/// SPI2 SCK (PB13, connected to accelerometer)
using Spi2Sck = alloy::hal::stm32f4::GpioPin<detail::SPI2_SCK_PIN>;

/// SPI2 MISO (PB14, connected to accelerometer)
using Spi2Miso = alloy::hal::stm32f4::GpioPin<detail::SPI2_MISO_PIN>;

/// SPI2 MOSI (PB15, connected to accelerometer)
using Spi2Mosi = alloy::hal::stm32f4::GpioPin<detail::SPI2_MOSI_PIN>;

/// SPI2 NSS (PB12, connected to accelerometer)
using Spi2Nss = alloy::hal::stm32f4::GpioPin<detail::SPI2_NSS_PIN>;

/// Accelerometer Chip Select (PE3)
using AccelCs = alloy::hal::stm32f4::GpioPin<detail::ACCEL_CS_PIN>;

//
// Helper Functions
//

/// Initialize board (can be called before main)
inline void initialize() {
    using namespace alloy::hal::st::stm32f4;
    using alloy::hal::Peripheral;

    // Create static clock instance
    static SystemClock clock;

    // Configure system clock to 168MHz using HSE (8MHz crystal) + PLL
    auto result = clock.set_frequency(168'000'000);
    if (!result.is_ok()) {
        // Fallback to HSI if HSE fails
        clock.set_frequency(16'000'000);
    }

    // Initialize SysTick for microsecond time tracking
    SystemTick::init();

    // Enable GPIO clocks for all ports used on the Discovery board
    clock.enable_peripheral(Peripheral::GpioA);
    clock.enable_peripheral(Peripheral::GpioB);
    clock.enable_peripheral(Peripheral::GpioC);
    clock.enable_peripheral(Peripheral::GpioD);  // LEDs on PD12-15
    clock.enable_peripheral(Peripheral::GpioE);
}

/// Simple delay (busy wait - not accurate, for basic use only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(uint32_t ms) {
    alloy::hal::st::stm32f4::delay_ms(ms);
}

//
// Board-specific LED helpers
//

/// LED control namespace
namespace Led {
    /// Turn LED on (handles active-high automatically)
    inline void on() {
        static LedGreen led;
        led.set_high();  // Active HIGH
    }

    /// Turn LED off (handles active-high automatically)
    inline void off() {
        static LedGreen led;
        led.set_low();  // Active HIGH
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

#endif // ALLOY_BOARD_STM32F407VG_HPP
