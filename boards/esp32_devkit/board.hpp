/// ESP32 DevKit Board Definition
///
/// Modern C++20 board support with pre-instantiated peripherals.
/// This file defines board-specific pins and peripherals that users
/// can access directly without needing to know pin numbers.
///
/// Inspired by modm's board abstraction approach.

#ifndef ALLOY_BOARD_ESP32_DEVKIT_HPP
#define ALLOY_BOARD_ESP32_DEVKIT_HPP

#include "hal/espressif/esp32/gpio.hpp"
#include "hal/espressif/esp32/clock.hpp"
#include "hal/espressif/esp32/delay.hpp"
#include "hal/espressif/esp32/systick.hpp"
#include <cstdint>

namespace Board {

/// Board identification
inline constexpr const char* name = "ESP32 DevKit";
inline constexpr const char* mcu = "ESP32-WROOM-32";

/// Clock configuration
inline constexpr uint32_t system_clock_hz = 160'000'000;  // 160 MHz (default)
inline constexpr uint32_t xtal_frequency_hz = 40'000'000; // 40 MHz external crystal
inline constexpr uint32_t apb_clock_hz = 80'000'000;      // 80 MHz APB clock (fixed)

//
// Pre-defined GPIO Pins
//
// These are the most commonly used pins on the ESP32 DevKit board.
// Users can access them directly: Board::LedBlue, Board::UartTx, etc.
//

namespace detail {
    using namespace alloy::hal::esp32;

    /// Pin number assignments (direct GPIO numbering)
    /// GPIO0 = 0, GPIO1 = 1, GPIO2 = 2, etc.

    // LED (built-in, typically on GPIO2, active HIGH)
    inline constexpr uint8_t LED_PIN = 2;  // GPIO2

    // UART0 pins (used for programming/debugging)
    inline constexpr uint8_t UART0_TX_PIN = 1;   // GPIO1 (U0TXD)
    inline constexpr uint8_t UART0_RX_PIN = 3;   // GPIO3 (U0RXD)

    // I2C pins (default I2C pins on ESP32)
    inline constexpr uint8_t I2C_SDA_PIN = 21;  // GPIO21 (SDA)
    inline constexpr uint8_t I2C_SCL_PIN = 22;  // GPIO22 (SCL)

    // SPI pins (VSPI - commonly used SPI bus)
    inline constexpr uint8_t SPI_MOSI_PIN = 23;  // GPIO23 (VSPI MOSI)
    inline constexpr uint8_t SPI_MISO_PIN = 19;  // GPIO19 (VSPI MISO)
    inline constexpr uint8_t SPI_SCK_PIN  = 18;  // GPIO18 (VSPI CLK)
    inline constexpr uint8_t SPI_CS_PIN   = 5;   // GPIO5 (VSPI CS)

    // Touch pins (capacitive touch sensor capable pins)
    inline constexpr uint8_t TOUCH0_PIN = 4;   // GPIO4 (T0)
    inline constexpr uint8_t TOUCH1_PIN = 0;   // GPIO0 (T1)
    inline constexpr uint8_t TOUCH2_PIN = 2;   // GPIO2 (T2)
    inline constexpr uint8_t TOUCH3_PIN = 15;  // GPIO15 (T3)
    inline constexpr uint8_t TOUCH4_PIN = 13;  // GPIO13 (T4)
    inline constexpr uint8_t TOUCH5_PIN = 12;  // GPIO12 (T5)
    inline constexpr uint8_t TOUCH6_PIN = 14;  // GPIO14 (T6)
    inline constexpr uint8_t TOUCH7_PIN = 27;  // GPIO27 (T7)
    inline constexpr uint8_t TOUCH8_PIN = 33;  // GPIO33 (T8)
    inline constexpr uint8_t TOUCH9_PIN = 32;  // GPIO32 (T9)

    // ADC1 pins (analog input capable - GPIO32-39, 36-39 are input-only)
    inline constexpr uint8_t ADC1_CH0_PIN = 36;  // GPIO36 (VP, input-only)
    inline constexpr uint8_t ADC1_CH1_PIN = 37;  // GPIO37 (input-only)
    inline constexpr uint8_t ADC1_CH2_PIN = 38;  // GPIO38 (input-only)
    inline constexpr uint8_t ADC1_CH3_PIN = 39;  // GPIO39 (VN, input-only)
    inline constexpr uint8_t ADC1_CH4_PIN = 32;  // GPIO32
    inline constexpr uint8_t ADC1_CH5_PIN = 33;  // GPIO33
    inline constexpr uint8_t ADC1_CH6_PIN = 34;  // GPIO34 (input-only)
    inline constexpr uint8_t ADC1_CH7_PIN = 35;  // GPIO35 (input-only)

    // DAC pins (digital-to-analog converter)
    inline constexpr uint8_t DAC1_PIN = 25;  // GPIO25 (DAC1)
    inline constexpr uint8_t DAC2_PIN = 26;  // GPIO26 (DAC2)
}

//
// Pre-instantiated GPIO Pins
//
// These are type aliases that users can instantiate.
// They provide compile-time pin configuration.
//

/// On-board LED (GPIO2, active HIGH)
/// Usage: Board::LedBlue led; led.configure(PinMode::Output);
using LedBlue = alloy::hal::esp32::GpioPin<detail::LED_PIN>;

/// UART0 TX (GPIO1) - used for programming
using Uart0Tx = alloy::hal::esp32::GpioPin<detail::UART0_TX_PIN>;

/// UART0 RX (GPIO3) - used for programming
using Uart0Rx = alloy::hal::esp32::GpioPin<detail::UART0_RX_PIN>;

/// I2C SDA (GPIO21)
using I2cSda = alloy::hal::esp32::GpioPin<detail::I2C_SDA_PIN>;

/// I2C SCL (GPIO22)
using I2cScl = alloy::hal::esp32::GpioPin<detail::I2C_SCL_PIN>;

/// SPI MOSI (GPIO23)
using SpiMosi = alloy::hal::esp32::GpioPin<detail::SPI_MOSI_PIN>;

/// SPI MISO (GPIO19)
using SpiMiso = alloy::hal::esp32::GpioPin<detail::SPI_MISO_PIN>;

/// SPI SCK (GPIO18)
using SpiSck = alloy::hal::esp32::GpioPin<detail::SPI_SCK_PIN>;

/// SPI CS (GPIO5)
using SpiCs = alloy::hal::esp32::GpioPin<detail::SPI_CS_PIN>;

/// Touch sensor 0 (GPIO4)
using Touch0 = alloy::hal::esp32::GpioPin<detail::TOUCH0_PIN>;

/// Touch sensor 1 (GPIO0)
using Touch1 = alloy::hal::esp32::GpioPin<detail::TOUCH1_PIN>;

/// Touch sensor 2 (GPIO2)
using Touch2 = alloy::hal::esp32::GpioPin<detail::TOUCH2_PIN>;

/// Touch sensor 3 (GPIO15)
using Touch3 = alloy::hal::esp32::GpioPin<detail::TOUCH3_PIN>;

/// Touch sensor 4 (GPIO13)
using Touch4 = alloy::hal::esp32::GpioPin<detail::TOUCH4_PIN>;

/// Touch sensor 5 (GPIO12)
using Touch5 = alloy::hal::esp32::GpioPin<detail::TOUCH5_PIN>;

/// Touch sensor 6 (GPIO14)
using Touch6 = alloy::hal::esp32::GpioPin<detail::TOUCH6_PIN>;

/// Touch sensor 7 (GPIO27)
using Touch7 = alloy::hal::esp32::GpioPin<detail::TOUCH7_PIN>;

/// Touch sensor 8 (GPIO33)
using Touch8 = alloy::hal::esp32::GpioPin<detail::TOUCH8_PIN>;

/// Touch sensor 9 (GPIO32)
using Touch9 = alloy::hal::esp32::GpioPin<detail::TOUCH9_PIN>;

/// ADC1 Channel 0 (GPIO36, input-only)
using Adc1Ch0 = alloy::hal::esp32::GpioPin<detail::ADC1_CH0_PIN>;

/// ADC1 Channel 3 (GPIO39, input-only)
using Adc1Ch3 = alloy::hal::esp32::GpioPin<detail::ADC1_CH3_PIN>;

/// ADC1 Channel 4 (GPIO32)
using Adc1Ch4 = alloy::hal::esp32::GpioPin<detail::ADC1_CH4_PIN>;

/// ADC1 Channel 5 (GPIO33)
using Adc1Ch5 = alloy::hal::esp32::GpioPin<detail::ADC1_CH5_PIN>;

/// ADC1 Channel 6 (GPIO34, input-only)
using Adc1Ch6 = alloy::hal::esp32::GpioPin<detail::ADC1_CH6_PIN>;

/// ADC1 Channel 7 (GPIO35, input-only)
using Adc1Ch7 = alloy::hal::esp32::GpioPin<detail::ADC1_CH7_PIN>;

/// DAC1 (GPIO25)
using Dac1 = alloy::hal::esp32::GpioPin<detail::DAC1_PIN>;

/// DAC2 (GPIO26)
using Dac2 = alloy::hal::esp32::GpioPin<detail::DAC2_PIN>;

//
// Helper Functions
//

/// Initialize board (can be called before main)
inline void initialize() {
    using namespace alloy::hal::espressif::esp32;

    // Create static clock instance
    static SystemClock clock;

    // Configure system clock to 240MHz (maximum for ESP32)
    // ESP32 typically runs at 240MHz for best performance
    auto result = clock.set_frequency(240'000'000);
    if (!result.is_ok()) {
        // Fallback to 160MHz if 240MHz fails
        result = clock.set_frequency(160'000'000);
        if (!result.is_ok()) {
            // Final fallback to 80MHz
            clock.set_frequency(80'000'000);
        }
    }

    // Initialize SysTick for microsecond time tracking
    SystemTick::init();

    // ESP32 doesn't need manual peripheral clock enable (auto-enabled on use)
}

/// Simple delay (busy wait - not accurate, for basic use only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(uint32_t ms) {
    alloy::hal::espressif::esp32::delay_ms(ms);
}

//
// Board-specific LED helpers
//

/// LED control namespace
namespace Led {
    /// Turn LED on (handles active-high automatically)
    inline void on() {
        static LedBlue led;
        led.set_high();  // Active HIGH
    }

    /// Turn LED off (handles active-high automatically)
    inline void off() {
        static LedBlue led;
        led.set_low();  // Active HIGH
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

#endif // ALLOY_BOARD_ESP32_DEVKIT_HPP
