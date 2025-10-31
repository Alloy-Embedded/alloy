/// STM32F407VG Discovery Board Definition
///
/// Pin mappings and board-specific configuration
/// Discovery board has 4 LEDs on PD12, PD13, PD14, PD15

#ifndef ALLOY_BOARD_STM32F407VG_HPP
#define ALLOY_BOARD_STM32F407VG_HPP

#include "hal/st/stm32f4/clock.hpp"
#include "hal/st/stm32f4/gpio.hpp"
#include "core/types.hpp"

namespace alloy::board {

/// Board name
constexpr const char* BOARD_NAME = "STM32F407VG Discovery";

/// Clock configuration
constexpr core::u32 HSE_FREQUENCY_HZ = 8000000;   // 8MHz external crystal
constexpr core::u32 SYSTEM_CLOCK_HZ = 168000000;  // 168MHz system clock
constexpr core::u32 APB1_CLOCK_HZ = 42000000;     // 42MHz APB1 (max for STM32F4)
constexpr core::u32 APB2_CLOCK_HZ = 84000000;     // 84MHz APB2 (max for STM32F4)

/// LED pins on STM32F407 Discovery
/// PD12 = Green LED
/// PD13 = Orange LED
/// PD14 = Red LED
/// PD15 = Blue LED
/// Pin numbering: Port D (3) * 16 + pin = 48 + pin
constexpr core::u8 LED_GREEN = 60;   // PD12 = 3*16 + 12
constexpr core::u8 LED_ORANGE = 61;  // PD13 = 3*16 + 13
constexpr core::u8 LED_RED = 62;     // PD14 = 3*16 + 14
constexpr core::u8 LED_BLUE = 63;    // PD15 = 3*16 + 15

// Default LED for blink examples
constexpr core::u8 LED_PIN = LED_GREEN;
constexpr bool LED_ACTIVE_HIGH = true;  // LEDs are active HIGH

/// Get system clock instance
inline hal::st::stm32f4::SystemClock& get_system_clock() {
    static hal::st::stm32f4::SystemClock clock;
    return clock;
}

/// Initialize board (clock and basic peripherals)
inline core::Result<void> init() {
    auto& clock = get_system_clock();

    // Configure system clock to 168MHz
    hal::ClockConfig config{
        .source = hal::ClockSource::ExternalCrystal,
        .crystal_frequency_hz = HSE_FREQUENCY_HZ,
        .pll_multiplier = 168,   // VCO = 8MHz * 168 / 4 = 336MHz
        .pll_divider = 4,        // PLLM = 4
        .ahb_divider = 1,
        .apb1_divider = 4,       // APB1 = 168/4 = 42MHz
        .apb2_divider = 2        // APB2 = 168/2 = 84MHz
    };

    auto result = clock.configure(config);
    if (result.is_error()) {
        return result;
    }

    return core::Result<void>::ok();
}

/// Configure LED pin as output
inline void init_led() {
    using namespace hal::st::stm32f4;

    // Configure PD12 (green LED) as output
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
    using namespace hal::st::stm32f4;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_HIGH) {
        led.set_high();  // HIGH = LED on
    } else {
        led.set_low();
    }
}

/// Turn LED off
inline void led_off() {
    using namespace hal::st::stm32f4;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_HIGH) {
        led.set_low();  // LOW = LED off
    } else {
        led.set_high();
    }
}

/// Toggle LED
inline void led_toggle() {
    using namespace hal::st::stm32f4;
    auto led = GpioPin<LED_PIN>();
    led.toggle();
}

/// Simple delay (busy wait - not accurate, for basic blink only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(core::u32 ms) {
    // Very rough estimate: at 168MHz, ~168000 cycles per ms
    // Assuming ~4 cycles per loop iteration
    volatile core::u32 count = ms * (SYSTEM_CLOCK_HZ / 4000);
    while (count--) {
        __asm__ volatile("nop");
    }
}

} // namespace alloy::board

#endif // ALLOY_BOARD_STM32F407VG_HPP
