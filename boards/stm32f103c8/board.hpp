/// STM32F103C8 Blue Pill Board Definition
///
/// Pin mappings and board-specific configuration

#ifndef ALLOY_BOARD_STM32F103C8_HPP
#define ALLOY_BOARD_STM32F103C8_HPP

#include "hal/st/stm32f1/clock.hpp"
#include "hal/st/stm32f1/gpio.hpp"
#include "core/types.hpp"

namespace alloy::board {

/// Board name
constexpr const char* BOARD_NAME = "STM32F103C8 Blue Pill";

/// Clock configuration
constexpr core::u32 HSE_FREQUENCY_HZ = 8000000;   // 8MHz external crystal
constexpr core::u32 SYSTEM_CLOCK_HZ = 72000000;   // 72MHz system clock
constexpr core::u32 APB1_CLOCK_HZ = 36000000;     // 36MHz APB1 (max for STM32F1)
constexpr core::u32 APB2_CLOCK_HZ = 72000000;     // 72MHz APB2

/// LED pin (active LOW on Blue Pill)
/// PC13 is connected to onboard LED (lights when LOW)
/// Pin numbering: PA0=0, PA1=1, ..., PC13=45, etc.
constexpr core::u8 LED_PIN = 45;  // PC13 = (2 * 16) + 13 = 45
constexpr bool LED_ACTIVE_LOW = true;

/// Get system clock instance
inline hal::st::stm32f1::SystemClock& get_system_clock() {
    static hal::st::stm32f1::SystemClock clock;
    return clock;
}

/// Initialize board (clock and basic peripherals)
inline core::Result<void> init() {
    auto& clock = get_system_clock();

    // Configure system clock to 72MHz
    hal::ClockConfig config{
        .source = hal::ClockSource::ExternalCrystal,
        .crystal_frequency_hz = HSE_FREQUENCY_HZ,
        .pll_multiplier = 9,  // 8MHz * 9 = 72MHz
        .ahb_divider = 1,
        .apb1_divider = 2,
        .apb2_divider = 1
    };

    auto result = clock.configure(config);
    if (result.is_error()) {
        return result;
    }

    // Enable GPIO clocks
    clock.enable_peripheral(hal::Peripheral::GpioA);
    clock.enable_peripheral(hal::Peripheral::GpioB);
    clock.enable_peripheral(hal::Peripheral::GpioC);

    return core::Result<void>::ok();
}

/// Configure LED pin as output
inline void init_led() {
    using namespace hal::st::stm32f1;

    // Configure PC13 as push-pull output
    auto led = GpioPin<LED_PIN>();
    led.configure(hal::PinMode::Output);

    // Turn LED off initially (HIGH = off on Blue Pill)
    if (LED_ACTIVE_LOW) {
        led.set_high();  // Set HIGH = LED off
    } else {
        led.set_low();
    }
}

/// Turn LED on
inline void led_on() {
    using namespace hal::st::stm32f1;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_LOW) {
        led.set_low();  // Set LOW = LED on
    } else {
        led.set_high();
    }
}

/// Turn LED off
inline void led_off() {
    using namespace hal::st::stm32f1;
    auto led = GpioPin<LED_PIN>();

    if (LED_ACTIVE_LOW) {
        led.set_high();  // Set HIGH = LED off
    } else {
        led.set_low();
    }
}

/// Toggle LED
inline void led_toggle() {
    using namespace hal::st::stm32f1;
    auto led = GpioPin<LED_PIN>();
    led.toggle();
}

/// Simple delay (busy wait - not accurate, for basic blink only)
/// @param ms Approximate milliseconds to delay
inline void delay_ms(core::u32 ms) {
    // Very rough estimate: at 72MHz, ~72000 cycles per ms
    // Assuming ~4 cycles per loop iteration
    volatile core::u32 count = ms * (SYSTEM_CLOCK_HZ / 4000);
    while (count--) {
        __asm__ volatile("nop");
    }
}

} // namespace alloy::board

#endif // ALLOY_BOARD_STM32F103C8_HPP
