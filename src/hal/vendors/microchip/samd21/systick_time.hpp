#pragma once

#include "hal/platform/arm/systick.hpp"
#include <cstdint>

namespace alloy::hal::microchip::samd21::systick {

/**
 * @brief SysTick timer for SAMD21 (48MHz CPU)
 *
 * Provides system time tracking for RTOS and application timing using ARM SysTick.
 * This is an alternative to the TC3-based systick.hpp implementation.
 *
 * Configured for 1ms tick rate by default (1000 Hz).
 *
 * Note: SAMD21 also has a TC3-based timer implementation in systick.hpp.
 * Use this SysTick version for RTOS integration (FreeRTOS, etc).
 * Use TC3 version if you need SysTick for other purposes.
 */
using SysTick = alloy::hal::platform::arm::SysTick<48000000>;

/**
 * @brief Initialize SysTick with default 1ms tick rate
 * @param tick_rate_hz Tick frequency (default: 1000 Hz = 1ms ticks)
 */
inline void init(uint32_t tick_rate_hz = 1000) {
    SysTick::init(tick_rate_hz);
}

/**
 * @brief Increment tick counter (call from SysTick_Handler)
 */
inline void tick() {
    SysTick::tick();
}

/**
 * @brief Get current tick count
 */
inline uint64_t get_ticks() {
    return SysTick::get_ticks();
}

/**
 * @brief Get elapsed time in milliseconds
 */
inline uint64_t get_time_ms() {
    return SysTick::get_time_ms();
}

/**
 * @brief Get elapsed time in microseconds
 */
inline uint64_t get_time_us() {
    return SysTick::get_time_us();
}

/**
 * @brief Get elapsed time in seconds
 */
inline uint32_t get_time_s() {
    return SysTick::get_time_s();
}

/**
 * @brief Delay (blocking) based on tick counter
 */
inline void delay_ms(uint32_t ms) {
    SysTick::delay_ms(ms);
}

} // namespace alloy::hal::microchip::samd21::systick

/**
 * Example interrupt handler (place in your application code):
 *
 * extern "C" void SysTick_Handler() {
 *     alloy::hal::microchip::samd21::systick::tick();
 * }
 */
