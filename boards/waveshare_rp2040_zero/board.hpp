/// Waveshare RP2040-Zero Board Definition
///
/// Modern C++20 board support with WS2812 RGB LED.
/// This board is smaller than the Raspberry Pi Pico and features:
/// - RP2040 microcontroller (dual ARM Cortex-M0+ @ 125MHz)
/// - WS2812 RGB LED on GPIO16
/// - 2MB flash
/// - 264KB RAM
/// - USB-C connector
/// - Compact form factor

#ifndef ALLOY_BOARD_WAVESHARE_RP2040_ZERO_HPP
#define ALLOY_BOARD_WAVESHARE_RP2040_ZERO_HPP

#include "hal/raspberry/rp2040/gpio.hpp"
#include "hal/raspberrypi/rp2040/ws2812_complete.hpp"
#include "hal/raspberrypi/rp2040/clock.hpp"
#include "hal/raspberrypi/rp2040/delay.hpp"
#include "hal/raspberrypi/rp2040/systick.hpp"
#include <cstdint>

namespace Board {

/// Board identification
inline constexpr const char* name = "Waveshare RP2040-Zero";
inline constexpr const char* mcu = "RP2040";

/// Clock configuration
inline constexpr uint32_t system_clock_hz = 125'000'000;  // 125 MHz (default)
inline constexpr uint32_t xosc_frequency_hz = 12'000'000; // 12 MHz external crystal

//
// Pre-defined GPIO Pins
//

namespace detail {
    using namespace alloy::hal::rp2040;

    // On-board WS2812 RGB LED (GPIO16)
    inline constexpr uint8_t RGB_LED_PIN = 16;

    // UART0 pins (default serial port)
    inline constexpr uint8_t UART0_TX_PIN = 0;   // GP0
    inline constexpr uint8_t UART0_RX_PIN = 1;   // GP1

    // I2C0 pins (default I2C)
    inline constexpr uint8_t I2C0_SDA_PIN = 4;   // GP4
    inline constexpr uint8_t I2C0_SCL_PIN = 5;   // GP5

    // SPI0 pins (default SPI)
    inline constexpr uint8_t SPI0_MISO_PIN = 16;  // GP16 (shared with LED!)
    inline constexpr uint8_t SPI0_CS_PIN   = 17;  // GP17
    inline constexpr uint8_t SPI0_SCK_PIN  = 18;  // GP18
    inline constexpr uint8_t SPI0_MOSI_PIN = 19;  // GP19
}

//
// WS2812 RGB LED
//

/// On-board RGB LED (WS2812 on GPIO16)
using RgbLed = alloy::hal::raspberrypi::rp2040::WS2812Complete<detail::RGB_LED_PIN>;

//
// Helper Functions
//

/// Initialize board (can be called before main)
inline void initialize() {
    using namespace alloy::hal::raspberrypi::rp2040;

    // Create static clock instance
    static SystemClock clock;

    // Configure system clock to 125MHz (standard RP2040 frequency)
    auto result = clock.set_frequency(125'000'000);
    if (!result.is_ok()) {
        // Fallback to ROSC if XOSC/PLL fails
        clock.set_frequency(6'500'000);
    }

    // Initialize SysTick (timer already running, just mark as initialized)
    SystemTick::init();

    // RP2040 enables peripheral clocks automatically
}

/// Simple delay (busy wait - not accurate, for basic use only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(uint32_t ms) {
    alloy::hal::raspberrypi::rp2040::delay_ms(ms);
}

//
// Board-specific LED helpers
//

/// LED control namespace for WS2812 RGB LED
namespace Led {
    /// Turn LED on with specified color (default: white)
    inline void on(const alloy::hal::raspberrypi::rp2040::RgbColor& color = alloy::hal::raspberrypi::rp2040::Colors::White) {
        static RgbLed led;
        led.set_color(color);
    }

    /// Turn LED off
    inline void off() {
        static RgbLed led;
        led.off();
    }

    /// Set LED color
    inline void set_color(const alloy::hal::raspberrypi::rp2040::RgbColor& color) {
        static RgbLed led;
        led.set_color(color);
    }

    /// Set brightness (0-255)
    inline void set_brightness(uint8_t brightness) {
        static RgbLed led;
        led.set_brightness(brightness);
    }

    /// Initialize LED
    inline void init() {
        static RgbLed led;
        led.init();
        off();  // Start with LED off
    }
}

} // namespace Board

#endif // ALLOY_BOARD_WAVESHARE_RP2040_ZERO_HPP
