/// Arduino Zero Board Definition
///
/// Modern C++20 board support with pre-instantiated peripherals.
/// This file defines board-specific pins and peripherals that users
/// can access directly without needing to know pin numbers.
///
/// Based on ATSAMD21G18 microcontroller.
///
/// Pin mappings follow Arduino Zero standard:
/// - Digital pins D0-D13
/// - Analog pins A0-A5
/// - UART, I2C, SPI interfaces
///
/// Inspired by modm's board abstraction approach.

#ifndef ALLOY_BOARD_ARDUINO_ZERO_HPP
#define ALLOY_BOARD_ARDUINO_ZERO_HPP

#include "hal/microchip/samd21/gpio.hpp"
#include "hal/microchip/samd21/clock.hpp"
#include "hal/microchip/samd21/delay.hpp"
#include "hal/microchip/samd21/systick.hpp"
#include <cstdint>

namespace Board {

/// Board identification
inline constexpr const char* name = "Arduino Zero";
inline constexpr const char* mcu = "ATSAMD21G18A";

/// Clock configuration
inline constexpr uint32_t system_clock_hz = 48'000'000;  // 48 MHz (DFLL48M)
inline constexpr uint32_t xosc32k_frequency_hz = 32'768;  // 32.768 kHz external crystal

//
// Pre-defined GPIO Pins
//
// These are the most commonly used pins on the Arduino Zero board.
// Users can access them directly: Board::Led, Board::UartTx, etc.
//

namespace detail {
    using namespace alloy::hal::samd21;

    /// Pin number calculation: Port * 32 + Pin
    /// PA00 = 0*32 + 0 = 0
    /// PA17 = 0*32 + 17 = 17
    /// PB08 = 1*32 + 8 = 40

    // LED (active HIGH)
    inline constexpr uint8_t LED_PIN = 17;  // PA17

    // Arduino Digital Pins D0-D13
    inline constexpr uint8_t D0_PIN  = 11;  // PA11 (SERCOM0 PAD3 - RX)
    inline constexpr uint8_t D1_PIN  = 10;  // PA10 (SERCOM0 PAD2 - TX)
    inline constexpr uint8_t D2_PIN  = 8;   // PA08
    inline constexpr uint8_t D3_PIN  = 9;   // PA09
    inline constexpr uint8_t D4_PIN  = 14;  // PA14
    inline constexpr uint8_t D5_PIN  = 15;  // PA15
    inline constexpr uint8_t D6_PIN  = 20;  // PA20
    inline constexpr uint8_t D7_PIN  = 21;  // PA21
    inline constexpr uint8_t D8_PIN  = 6;   // PA06
    inline constexpr uint8_t D9_PIN  = 7;   // PA07
    inline constexpr uint8_t D10_PIN = 18;  // PA18
    inline constexpr uint8_t D11_PIN = 16;  // PA16
    inline constexpr uint8_t D12_PIN = 19;  // PA19
    inline constexpr uint8_t D13_PIN = 17;  // PA17 (LED)

    // Arduino Analog Pins A0-A5
    inline constexpr uint8_t A0_PIN = 2;   // PA02 (AIN0)
    inline constexpr uint8_t A1_PIN = 40;  // PB08 (AIN2)
    inline constexpr uint8_t A2_PIN = 41;  // PB09 (AIN3)
    inline constexpr uint8_t A3_PIN = 4;   // PA04 (AIN4)
    inline constexpr uint8_t A4_PIN = 5;   // PA05 (AIN5)
    inline constexpr uint8_t A5_PIN = 34;  // PB02 (AIN10)

    // UART (SERCOM0)
    inline constexpr uint8_t UART_TX_PIN = 10;  // PA10 (D1)
    inline constexpr uint8_t UART_RX_PIN = 11;  // PA11 (D0)

    // I2C (SERCOM3)
    inline constexpr uint8_t I2C_SDA_PIN = 22;  // PA22 (SDA)
    inline constexpr uint8_t I2C_SCL_PIN = 23;  // PA23 (SCL)

    // SPI (SERCOM4)
    inline constexpr uint8_t SPI_MOSI_PIN = 44;  // PB10 (MOSI)
    inline constexpr uint8_t SPI_MISO_PIN = 22;  // PA12 (MISO)
    inline constexpr uint8_t SPI_SCK_PIN  = 45;  // PB11 (SCK)
    inline constexpr uint8_t SPI_SS_PIN   = 14;  // PA14 (SS)

    // USB pins
    inline constexpr uint8_t USB_DM_PIN = 24;  // PA24 (USB D-)
    inline constexpr uint8_t USB_DP_PIN = 25;  // PA25 (USB D+)
}

//
// Pre-instantiated GPIO Pins
//
// These are type aliases that users can instantiate.
// They provide compile-time pin configuration.
//

/// On-board LED (PA17, active HIGH)
/// Usage: Board::Led led; led.configure(PinMode::Output);
using Led = alloy::hal::samd21::GpioPin<detail::LED_PIN>;

/// Arduino Digital Pins (D0-D13)
using D0  = alloy::hal::samd21::GpioPin<detail::D0_PIN>;
using D1  = alloy::hal::samd21::GpioPin<detail::D1_PIN>;
using D2  = alloy::hal::samd21::GpioPin<detail::D2_PIN>;
using D3  = alloy::hal::samd21::GpioPin<detail::D3_PIN>;
using D4  = alloy::hal::samd21::GpioPin<detail::D4_PIN>;
using D5  = alloy::hal::samd21::GpioPin<detail::D5_PIN>;
using D6  = alloy::hal::samd21::GpioPin<detail::D6_PIN>;
using D7  = alloy::hal::samd21::GpioPin<detail::D7_PIN>;
using D8  = alloy::hal::samd21::GpioPin<detail::D8_PIN>;
using D9  = alloy::hal::samd21::GpioPin<detail::D9_PIN>;
using D10 = alloy::hal::samd21::GpioPin<detail::D10_PIN>;
using D11 = alloy::hal::samd21::GpioPin<detail::D11_PIN>;
using D12 = alloy::hal::samd21::GpioPin<detail::D12_PIN>;
using D13 = alloy::hal::samd21::GpioPin<detail::D13_PIN>;

/// Arduino Analog Pins (A0-A5)
using A0 = alloy::hal::samd21::GpioPin<detail::A0_PIN>;
using A1 = alloy::hal::samd21::GpioPin<detail::A1_PIN>;
using A2 = alloy::hal::samd21::GpioPin<detail::A2_PIN>;
using A3 = alloy::hal::samd21::GpioPin<detail::A3_PIN>;
using A4 = alloy::hal::samd21::GpioPin<detail::A4_PIN>;
using A5 = alloy::hal::samd21::GpioPin<detail::A5_PIN>;

/// UART Pins (SERCOM0)
using UartTx = alloy::hal::samd21::GpioPin<detail::UART_TX_PIN>;
using UartRx = alloy::hal::samd21::GpioPin<detail::UART_RX_PIN>;

/// I2C Pins (SERCOM3)
using I2cSda = alloy::hal::samd21::GpioPin<detail::I2C_SDA_PIN>;
using I2cScl = alloy::hal::samd21::GpioPin<detail::I2C_SCL_PIN>;

/// SPI Pins (SERCOM4)
using SpiMosi = alloy::hal::samd21::GpioPin<detail::SPI_MOSI_PIN>;
using SpiMiso = alloy::hal::samd21::GpioPin<detail::SPI_MISO_PIN>;
using SpiSck  = alloy::hal::samd21::GpioPin<detail::SPI_SCK_PIN>;
using SpiSs   = alloy::hal::samd21::GpioPin<detail::SPI_SS_PIN>;

/// USB Pins
using UsbDm = alloy::hal::samd21::GpioPin<detail::USB_DM_PIN>;
using UsbDp = alloy::hal::samd21::GpioPin<detail::USB_DP_PIN>;

//
// Helper Functions
//

/// Initialize board (can be called before main)
inline void initialize() {
    using namespace alloy::hal::microchip::samd21;
    using alloy::hal::Peripheral;

    // Create static clock instance
    static SystemClock clock;

    // Configure system clock to 48MHz using DFLL48M
    // Arduino Zero typically uses 48MHz from DFLL48M
    auto result = clock.set_frequency(48'000'000);
    if (!result.is_ok()) {
        // Fallback to OSC8M if DFLL fails
        clock.set_frequency(8'000'000);
    }

    // Initialize SysTick for microsecond time tracking
    SystemTick::init();

    // Enable GPIO port clocks
    clock.enable_peripheral(Peripheral::GpioA);
    clock.enable_peripheral(Peripheral::GpioB);
}

/// Simple delay (busy wait - not accurate, for basic use only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(uint32_t ms) {
    alloy::hal::microchip::samd21::delay_ms(ms);
}

//
// Board-specific LED helpers
//

/// LED control namespace
namespace LedControl {
    /// Turn LED on (handles active-high automatically)
    inline void on() {
        static Led led;
        led.set_high();  // Active HIGH
    }

    /// Turn LED off (handles active-high automatically)
    inline void off() {
        static Led led;
        led.set_low();  // Active HIGH
    }

    /// Toggle LED state
    inline void toggle() {
        static Led led;
        led.toggle();
    }

    /// Initialize LED as output
    inline void init() {
        static Led led;
        led.configure(alloy::hal::PinMode::Output);
        off();  // Start with LED off
    }
}

} // namespace Board

#endif // ALLOY_BOARD_ARDUINO_ZERO_HPP
