#ifndef ALLOY_HAL_RP2040_DELAY_HPP
#define ALLOY_HAL_RP2040_DELAY_HPP

#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040 {

/// Simple busy-wait delay for RP2040
///
/// This is a basic implementation using a busy-wait loop.
/// Assumes 125 MHz system clock (standard for RP2040).
///
/// For more accurate delays, use hardware timers.
/// This implementation is good enough for simple blinky demos.
///
/// @param milliseconds Number of milliseconds to delay
inline void delay_ms(uint32_t milliseconds) {
    // Approximate cycles per millisecond at 125 MHz
    // Accounting for loop overhead: ~125000 / 4 = 31250 iterations
    constexpr uint32_t cycles_per_ms = 31250;

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
    // Approximate cycles per microsecond at 125 MHz
    // Accounting for loop overhead: ~125 / 4 = 31 iterations
    constexpr uint32_t cycles_per_us = 31;

    for (uint32_t us = 0; us < microseconds; ++us) {
        for (volatile uint32_t i = 0; i < cycles_per_us; ++i) {
            __asm__ volatile("nop");
        }
    }
}

} // namespace alloy::hal::raspberrypi::rp2040

#endif // ALLOY_HAL_RP2040_DELAY_HPP
