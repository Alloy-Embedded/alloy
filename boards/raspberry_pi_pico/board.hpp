/// Raspberry Pi Pico Board Definition
///
/// Pin mappings and board-specific configuration
/// Based on RP2040 microcontroller

#ifndef ALLOY_BOARD_RASPBERRY_PI_PICO_HPP
#define ALLOY_BOARD_RASPBERRY_PI_PICO_HPP

#include "hal/raspberrypi/rp2040/clock.hpp"
#include "hal/raspberrypi/rp2040/gpio.hpp"
#include "core/types.hpp"

namespace alloy::board {

/// Board name
constexpr const char* BOARD_NAME = "Raspberry Pi Pico";

/// Clock configuration
constexpr core::u32 XOSC_FREQUENCY_HZ = 12000000;    // 12MHz external crystal
constexpr core::u32 SYSTEM_CLOCK_HZ = 125000000;     // 125MHz system clock (PLL_SYS)

/// LED pin on Raspberry Pi Pico
/// GPIO25 is connected to the onboard LED
constexpr core::u8 LED_PIN = 25;  // GPIO25
constexpr bool LED_ACTIVE_HIGH = true;  // LED is active HIGH

/// Get system clock instance
inline hal::raspberrypi::rp2040::SystemClock& get_system_clock() {
    static hal::raspberrypi::rp2040::SystemClock clock;
    return clock;
}

/// Initialize board (clock and basic peripherals)
inline core::Result<void> init() {
    auto& clock = get_system_clock();

    // Configure system clock to 125MHz (PLL_SYS from 12MHz XOSC)
    hal::ClockConfig config{
        .source = hal::ClockSource::ExternalCrystal,
        .crystal_frequency_hz = XOSC_FREQUENCY_HZ,
        .pll_multiplier = 125,   // 12MHz * 125 / 6 / 2 = 125MHz
        .pll_divider = 6,
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
    using namespace hal::raspberrypi::rp2040;

    // Configure GPIO25 as output
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
    using namespace hal::raspberrypi::rp2040;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_HIGH) {
        led.set_high();  // HIGH = LED on
    } else {
        led.set_low();
    }
}

/// Turn LED off
inline void led_off() {
    using namespace hal::raspberrypi::rp2040;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_HIGH) {
        led.set_low();  // LOW = LED off
    } else {
        led.set_high();
    }
}

/// Toggle LED
inline void led_toggle() {
    using namespace hal::raspberrypi::rp2040;
    auto led = GpioPin<LED_PIN>();
    led.toggle();
}

/// Simple delay (busy wait - not accurate, for basic blink only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(core::u32 ms) {
    // Very rough estimate: at 125MHz, ~125000 cycles per ms
    // Assuming ~4 cycles per loop iteration
    volatile core::u32 count = ms * (SYSTEM_CLOCK_HZ / 4000);
    while (count--) {
        __asm__ volatile("nop");
    }
}

} // namespace alloy::board

#endif // ALLOY_BOARD_RASPBERRY_PI_PICO_HPP
