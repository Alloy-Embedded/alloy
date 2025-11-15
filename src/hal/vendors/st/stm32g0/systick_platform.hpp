/**
 * @file systick_platform.hpp
 * @brief SysTick timer platform implementation for STM32G0
 *
 * Provides compile-time configured SysTick timer for timing and delays.
 * Uses ARM Cortex-M0+ SysTick peripheral.
 */

#pragma once

#include "core/types.hpp"
#include <cstdint>

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

/**
 * @brief ARM Cortex-M SysTick Register Structure
 */
struct SysTick_Registers {
    volatile uint32_t CTRL;   // 0x00: SysTick Control and Status Register
    volatile uint32_t LOAD;   // 0x04: SysTick Reload Value Register
    volatile uint32_t VAL;    // 0x08: SysTick Current Value Register
    volatile uint32_t CALIB;  // 0x0C: SysTick Calibration Value Register
};

// SysTick base address (Cortex-M standard)
constexpr uintptr_t SYST_BASE = 0xE000E010;

/**
 * @brief SysTick timer with compile-time frequency
 *
 * @tparam CLOCK_HZ System clock frequency in Hz
 *
 * Usage:
 * @code
 * using BoardSysTick = SysTick<64000000>;  // 64 MHz
 * SysTickTimer::init_ms<BoardSysTick>(1);  // 1ms tick
 * SysTickTimer::delay_ms<BoardSysTick>(100);  // 100ms delay
 * @endcode
 */
template <uint32_t CLOCK_HZ>
class SysTick {
public:
    static constexpr uint32_t clock_hz = CLOCK_HZ;

    /**
     * @brief Initialize SysTick with given period in milliseconds
     *
     * @param period_ms Period in milliseconds (1ms typical)
     */
    static void init_ms(uint32_t period_ms) {
        auto* systick = reinterpret_cast<volatile SysTick_Registers*>(SYST_BASE);

        // Calculate reload value: (clock_hz / 1000) * period_ms - 1
        uint32_t reload = ((CLOCK_HZ / 1000) * period_ms) - 1;

        systick->LOAD = reload;
        systick->VAL = 0;  // Clear current value

        // Enable SysTick: ENABLE | TICKINT | CLKSOURCE (processor clock)
        systick->CTRL = 0x7;
    }

    /**
     * @brief Get current tick count (incremented in ISR)
     */
    static uint32_t get_ticks() {
        return tick_count;
    }

    /**
     * @brief Get milliseconds since initialization
     */
    static uint32_t millis() {
        return tick_count;
    }

    /**
     * @brief Get microseconds (for delay functions)
     */
    static uint64_t micros() {
        return static_cast<uint64_t>(tick_count) * 1000;  // 1ms ticks to microseconds
    }

    /**
     * @brief Increment tick count (called from SysTick_Handler ISR)
     */
    static void increment_tick() {
        tick_count = tick_count + 1;  // Avoid deprecated volatile++
    }

private:
    static inline volatile uint32_t tick_count = 0;
};

} // namespace alloy::hal::st::stm32g0
