#ifndef ALLOY_HAL_STM32F1_DELAY_HPP
#define ALLOY_HAL_STM32F1_DELAY_HPP

#include <cstdint>

namespace alloy::hal::stm32f1 {

/// Simple busy-wait delay for STM32F1
///
/// This is a basic implementation using a busy-wait loop.
/// Assumes 72 MHz system clock.
///
/// For more accurate delays, use SysTick timer or hardware timers.
/// This implementation is good enough for simple blinky demos.
///
/// @param milliseconds Number of milliseconds to delay
inline void delay_ms(uint32_t milliseconds) {
    // Approximate cycles per millisecond at 72 MHz
    // Accounting for loop overhead: ~72000 / 4 = 18000 iterations
    constexpr uint32_t cycles_per_ms = 18000;

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
    // Approximate cycles per microsecond at 72 MHz
    // Accounting for loop overhead: ~72 / 4 = 18 iterations
    constexpr uint32_t cycles_per_us = 18;

    for (uint32_t us = 0; us < microseconds; ++us) {
        for (volatile uint32_t i = 0; i < cycles_per_us; ++i) {
            __asm__ volatile("nop");
        }
    }
}

} // namespace alloy::hal::stm32f1

#endif // ALLOY_HAL_STM32F1_DELAY_HPP
