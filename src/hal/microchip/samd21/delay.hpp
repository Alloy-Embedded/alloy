#ifndef ALLOY_HAL_SAMD21_DELAY_HPP
#define ALLOY_HAL_SAMD21_DELAY_HPP

#include <cstdint>

namespace alloy::hal::microchip::samd21 {

/// Simple busy-wait delay for SAMD21
///
/// This is a basic implementation using a busy-wait loop.
/// Assumes 48 MHz system clock (typical for SAMD21 with DFLL48M).
///
/// For more accurate delays, use SysTick timer or hardware timers.
/// This implementation is good enough for simple blinky demos.
///
/// @param milliseconds Number of milliseconds to delay
inline void delay_ms(uint32_t milliseconds) {
    // Approximate cycles per millisecond at 48 MHz
    // Accounting for loop overhead: ~48000 / 4 = 12000 iterations
    constexpr uint32_t cycles_per_ms = 12000;

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
    // Approximate cycles per microsecond at 48 MHz
    // Accounting for loop overhead: ~48 / 4 = 12 iterations
    constexpr uint32_t cycles_per_us = 12;

    for (uint32_t us = 0; us < microseconds; ++us) {
        for (volatile uint32_t i = 0; i < cycles_per_us; ++i) {
            __asm__ volatile("nop");
        }
    }
}

} // namespace alloy::hal::microchip::samd21

#endif // ALLOY_HAL_SAMD21_DELAY_HPP
