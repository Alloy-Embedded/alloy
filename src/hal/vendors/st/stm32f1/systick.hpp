#pragma once

#include "hal/platform/arm/systick.hpp"
#include <cstdint>

namespace alloy::hal::st::stm32f1::systick {

/**
 * @brief SysTick timer for STM32F1 (72MHz CPU)
 *
 * Provides system time tracking for RTOS and application timing.
 * Configured for 1ms tick rate by default (1000 Hz).
 */
using SysTick = alloy::hal::platform::arm::SysTick<72000000>;

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

} // namespace alloy::hal::st::stm32f1::systick

/**
 * Example interrupt handler (place in your application code):
 *
 * extern "C" void SysTick_Handler() {
 *     alloy::hal::st::stm32f1::systick::tick();
 * }
 */
