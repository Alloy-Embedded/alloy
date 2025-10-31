/// Arduino Zero Board Definition
///
/// Pin mappings and board-specific configuration
/// Based on ATSAMD21G18 microcontroller

#ifndef ALLOY_BOARD_ARDUINO_ZERO_HPP
#define ALLOY_BOARD_ARDUINO_ZERO_HPP

#include "hal/microchip/samd21/clock.hpp"
#include "hal/microchip/samd21/gpio.hpp"
#include "core/types.hpp"

namespace alloy::board {

/// Board name
constexpr const char* BOARD_NAME = "Arduino Zero";

/// Clock configuration
constexpr core::u32 XOSC32K_FREQUENCY_HZ = 32768;    // 32.768kHz external crystal
constexpr core::u32 SYSTEM_CLOCK_HZ = 48000000;      // 48MHz system clock (DFLL48M)

/// LED pin on Arduino Zero
/// PA17 is connected to the yellow LED (L)
/// Pin numbering: Port A (0) * 32 + 17 = 17
constexpr core::u8 LED_PIN = 17;  // PA17
constexpr bool LED_ACTIVE_HIGH = true;  // LED is active HIGH

/// Get system clock instance
inline hal::microchip::samd21::SystemClock& get_system_clock() {
    static hal::microchip::samd21::SystemClock clock;
    return clock;
}

/// Initialize board (clock and basic peripherals)
inline core::Result<void> init() {
    auto& clock = get_system_clock();

    // Configure system clock to 48MHz (DFLL48M from 32kHz crystal)
    hal::ClockConfig config{
        .source = hal::ClockSource::ExternalCrystal,
        .crystal_frequency_hz = XOSC32K_FREQUENCY_HZ,
        .pll_multiplier = 1,     // Not used for DFLL
        .pll_divider = 1,
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
    using namespace hal::microchip::samd21;

    // Configure PA17 as output
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
    using namespace hal::microchip::samd21;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_HIGH) {
        led.set_high();  // HIGH = LED on
    } else {
        led.set_low();
    }
}

/// Turn LED off
inline void led_off() {
    using namespace hal::microchip::samd21;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_HIGH) {
        led.set_low();  // LOW = LED off
    } else {
        led.set_high();
    }
}

/// Toggle LED
inline void led_toggle() {
    using namespace hal::microchip::samd21;
    auto led = GpioPin<LED_PIN>();
    led.toggle();
}

/// Simple delay (busy wait - not accurate, for basic blink only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(core::u32 ms) {
    // Very rough estimate: at 48MHz, ~48000 cycles per ms
    // Assuming ~4 cycles per loop iteration
    volatile core::u32 count = ms * (SYSTEM_CLOCK_HZ / 4000);
    while (count--) {
        __asm__ volatile("nop");
    }
}

} // namespace alloy::board

#endif // ALLOY_BOARD_ARDUINO_ZERO_HPP
