#ifndef ALLOY_HAL_ESP32_DELAY_HPP
#define ALLOY_HAL_ESP32_DELAY_HPP

#include <cstdint>

namespace alloy::hal::espressif::esp32 {

/// Simple busy-wait delay for ESP32
///
/// This is a basic implementation using a busy-wait loop.
/// Assumes 240 MHz system clock (typical for ESP32 at max speed).
///
/// For more accurate delays, use FreeRTOS vTaskDelay or hardware timers.
/// This implementation is good enough for simple blinky demos.
///
/// @param milliseconds Number of milliseconds to delay
inline void delay_ms(uint32_t milliseconds) {
    // Approximate cycles per millisecond at 240 MHz
    // Accounting for loop overhead: ~240000 / 4 = 60000 iterations
    constexpr uint32_t cycles_per_ms = 60000;

    for (uint32_t ms = 0; ms < milliseconds; ++ms) {
        for (volatile uint32_t i = 0; i < cycles_per_ms; ++i) {
            // Busy wait
            __asm__ volatile("nop");
        }
    }
}

/// Microsecond delay
///
/// @param microseconds Number of microseconds to delay
inline void delay_us(uint32_t microseconds) {
    // Approximate cycles per microsecond at 240 MHz
    // Accounting for loop overhead: ~240 / 4 = 60 iterations
    constexpr uint32_t cycles_per_us = 60;

    for (uint32_t us = 0; us < microseconds; ++us) {
        for (volatile uint32_t i = 0; i < cycles_per_us; ++i) {
            __asm__ volatile("nop");
        }
    }
}

} // namespace alloy::hal::espressif::esp32

#endif // ALLOY_HAL_ESP32_DELAY_HPP
