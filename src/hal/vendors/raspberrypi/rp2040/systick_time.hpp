#pragma once

#include "hal/platform/arm/systick.hpp"
#include <cstdint>

namespace alloy::hal::raspberrypi::rp2040::systick {

/**
 * @brief SysTick timer for RP2040 (125-133MHz CPU)
 *
 * Provides system time tracking for RTOS and application timing using ARM SysTick.
 *
 * Configured for 1ms tick rate by default (1000 Hz).
 *
 * Note: RP2040 has a dedicated 1MHz 64-bit hardware timer that is superior
 * to SysTick for time tracking. This SysTick implementation is provided for
 * compatibility and RTOS integration (FreeRTOS, etc), but consider using the
 * hardware timer for application timing.
 *
 * Hardware timer advantages:
 * - 1MHz precision (vs tick-based)
 * - 64-bit counter (no overflow)
 * - No interrupt overhead for reads
 * - Independent of CPU clock changes
 */
using SysTick = alloy::hal::platform::arm::SysTick<125000000>;

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

} // namespace alloy::hal::raspberrypi::rp2040::systick

/**
 * Example interrupt handler (place in your application code):
 *
 * extern "C" void SysTick_Handler() {
 *     alloy::hal::raspberrypi::rp2040::systick::tick();
 * }
 */

/**
 * Alternative: RP2040 Hardware Timer (recommended for application timing)
 *
 * The RP2040 has a 64-bit microsecond timer at 0x40054000:
 *
 * volatile uint32_t* TIMER_TIMELR = (volatile uint32_t*)0x40054028;  // Low 32 bits
 * volatile uint32_t* TIMER_TIMEHR = (volatile uint32_t*)0x4005402C;  // High 32 bits
 *
 * uint64_t get_time_us() {
 *     uint32_t low = *TIMER_TIMELR;
 *     uint32_t high = *TIMER_TIMEHR;
 *     return ((uint64_t)high << 32) | low;
 * }
 *
 * This timer runs at 1MHz and provides microsecond precision without
 * any interrupt overhead or configuration.
 */
