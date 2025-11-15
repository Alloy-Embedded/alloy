/**
 * @file systick_platform.hpp
 * @brief SysTick timer platform implementation for all STM32 families
 *
 * Provides compile-time configured SysTick timer for timing and delays.
 * Uses ARM Cortex-M SysTick peripheral (common across all STM32 families).
 *
 * This file is shared across STM32F4, STM32F7, STM32G0, and other STM32 families
 * since the SysTick peripheral is identical across all ARM Cortex-M cores.
 */

#pragma once

#include "core/types.hpp"
#include <cstdint>

namespace alloy::hal::st::common {

using namespace alloy::core;

/**
 * @brief SysTick timer platform implementation (shared across all STM32)
 *
 * Template parameter allows compile-time configuration of the tick frequency.
 * All STM32 families use the same ARM Cortex-M SysTick peripheral.
 *
 * @tparam CLOCK_HZ System clock frequency in Hz
 */
template <uint32_t CLOCK_HZ>
class SysTickPlatform {
public:
    static constexpr uint32_t clock_hz = CLOCK_HZ;

    /**
     * @brief Initialize SysTick for 1ms tick rate
     */
    static void init() {
        constexpr uint32_t reload_value = (CLOCK_HZ / 1000) - 1;  // 1ms tick

        // SysTick registers (standard ARM Cortex-M)
        volatile uint32_t* const STK_CTRL  = reinterpret_cast<volatile uint32_t*>(0xE000E010);
        volatile uint32_t* const STK_LOAD  = reinterpret_cast<volatile uint32_t*>(0xE000E014);
        volatile uint32_t* const STK_VAL   = reinterpret_cast<volatile uint32_t*>(0xE000E018);

        *STK_LOAD = reload_value;
        *STK_VAL  = 0;                    // Clear current value
        *STK_CTRL = 0x07;                 // Enable, interrupt, use processor clock

        tick_count = 0;
    }

    /**
     * @brief Get current tick count
     * @return Number of milliseconds since init()
     */
    static uint32_t get_tick() {
        return tick_count;
    }

    /**
     * @brief Delay for specified milliseconds
     * @param ms Milliseconds to delay
     */
    static void delay_ms(uint32_t ms) {
        uint32_t start = get_tick();
        while ((get_tick() - start) < ms) {
            // Busy wait
        }
    }

    /**
     * @brief SysTick interrupt handler (call from ISR)
     *
     * Must be called from the SysTick_Handler() interrupt service routine.
     */
    static void tick() {
        tick_count++;
    }

    /**
     * @brief Get system clock frequency
     * @return Clock frequency in Hz
     */
    static constexpr uint32_t get_clock_hz() {
        return CLOCK_HZ;
    }

private:
    static inline volatile uint32_t tick_count = 0;
};

} // namespace alloy::hal::st::common
