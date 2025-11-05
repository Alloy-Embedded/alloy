#pragma once

#include "hal/platform/arm/systick_delay.hpp"
#include <cstdint>

namespace alloy::hal::atmel::same70::delay {

/**
 * @brief Delay implementation for SAME70/SAMV71
 *
 * Uses the shared ARM Cortex-M SysTick delay template configured for
 * SAME70's 300MHz CPU clock frequency.
 */
using Delay = alloy::hal::platform::arm::SysTickDelay<300000000>;

/**
 * @brief Initialize delay functionality
 *
 * No initialization required for SysTick delays.
 */
inline void init() {
    Delay::init();
}

/**
 * @brief Delay for specified milliseconds
 * @param ms Number of milliseconds to delay
 */
inline void delay_ms(uint32_t ms) {
    Delay::delay_ms(ms);
}

/**
 * @brief Delay for specified microseconds
 * @param us Number of microseconds to delay
 */
inline void delay_us(uint32_t us) {
    Delay::delay_us(us);
}

/**
 * @brief Delay for specified CPU cycles
 * @param cycles Number of CPU cycles to delay
 */
inline void delay_cycles(uint32_t cycles) {
    Delay::delay_cycles(cycles);
}

}  // namespace alloy::hal::atmel::same70::delay
