#ifndef ALLOY_HAL_STM32F4_DELAY_HPP
#define ALLOY_HAL_STM32F4_DELAY_HPP

#include <cstdint>

namespace alloy::hal::st::stm32f4 {

/// Simple busy-wait delay for STM32F4
///
/// This is a basic implementation using a busy-wait loop.
/// Assumes 168 MHz system clock (typical for STM32F407).
///
/// For more accurate delays, use SysTick timer or hardware timers.
/// This implementation is good enough for simple blinky demos.
///
/// @param milliseconds Number of milliseconds to delay
inline void delay_ms(uint32_t milliseconds) {
    // Approximate cycles per millisecond at 168 MHz
    // Accounting for loop overhead: ~168000 / 4 = 42000 iterations
    constexpr uint32_t cycles_per_ms = 42000;

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
    // Approximate cycles per microsecond at 168 MHz
    // Accounting for loop overhead: ~168 / 4 = 42 iterations
    constexpr uint32_t cycles_per_us = 42;

    for (uint32_t us = 0; us < microseconds; ++us) {
        for (volatile uint32_t i = 0; i < cycles_per_us; ++i) {
            __asm__ volatile("nop");
        }
    }
}

} // namespace alloy::hal::st::stm32f4

#endif // ALLOY_HAL_STM32F4_DELAY_HPP
