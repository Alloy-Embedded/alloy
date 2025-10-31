/// ESP32 DevKit Board Definition
///
/// Pin mappings and board-specific configuration for ESP32 DevKit

#ifndef ALLOY_BOARD_ESP32_DEVKIT_HPP
#define ALLOY_BOARD_ESP32_DEVKIT_HPP

#include "hal/espressif/esp32/clock.hpp"
#include "hal/espressif/esp32/gpio.hpp"
#include "core/types.hpp"

namespace alloy::board {

/// Board name
constexpr const char* BOARD_NAME = "ESP32 DevKit";

/// Clock configuration
constexpr core::u32 XTAL_FREQUENCY_HZ = 40000000;    // 40MHz external crystal
constexpr core::u32 SYSTEM_CLOCK_HZ = 160000000;     // 160MHz system clock (default)
constexpr core::u32 APB_CLOCK_HZ = 80000000;         // 80MHz APB (fixed)

/// LED pin (most ESP32 DevKit boards have LED on GPIO2)
/// GPIO2 is also connected to built-in blue LED on many boards
constexpr core::u8 LED_PIN = 2;  // GPIO2
constexpr bool LED_ACTIVE_HIGH = true;  // LED is active HIGH

/// Get system clock instance
inline hal::espressif::esp32::SystemClock& get_system_clock() {
    static hal::espressif::esp32::SystemClock clock;
    return clock;
}

/// Initialize board (clock and basic peripherals)
inline core::Result<void> init() {
    auto& clock = get_system_clock();

    // Configure system clock to 160MHz
    hal::ClockConfig config{
        .source = hal::ClockSource::ExternalCrystal,
        .crystal_frequency_hz = XTAL_FREQUENCY_HZ,
        .pll_multiplier = 4,  // 40MHz * 8 = 320MHz PLL, then /2 = 160MHz
        .ahb_divider = 1,
        .apb1_divider = 1,
        .apb2_divider = 1
    };

    auto result = clock.configure(config);
    if (result.is_error()) {
        return result;
    }

    return core::Result<void>::ok();
}

/// Configure LED pin as output
inline void init_led() {
    using namespace hal::espressif::esp32;

    // Configure GPIO2 as output
    auto led = GpioPin<LED_PIN>();
    led.configure(hal::PinMode::Output);

    // Turn LED off initially
    if (LED_ACTIVE_HIGH) {
        led.set_low();  // LOW = LED off
    } else {
        led.set_high();
    }
}

/// Turn LED on
inline void led_on() {
    using namespace hal::espressif::esp32;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_HIGH) {
        led.set_high();  // HIGH = LED on
    } else {
        led.set_low();
    }
}

/// Turn LED off
inline void led_off() {
    using namespace hal::espressif::esp32;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_HIGH) {
        led.set_low();  // LOW = LED off
    } else {
        led.set_high();
    }
}

/// Toggle LED
inline void led_toggle() {
    using namespace hal::espressif::esp32;
    auto led = GpioPin<LED_PIN>();
    led.toggle();
}

/// Simple delay (busy wait - not accurate, for basic blink only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(core::u32 ms) {
    // Very rough estimate: at 160MHz, ~160000 cycles per ms
    // Assuming ~4 cycles per loop iteration
    volatile core::u32 count = ms * (SYSTEM_CLOCK_HZ / 4000);
    while (count--) {
        __asm__ volatile("nop");
    }
}

} // namespace alloy::board

#endif // ALLOY_BOARD_ESP32_DEVKIT_HPP
