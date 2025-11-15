/**
 * @file systick_platform.hpp
 * @brief SysTick Platform Layer for SAME70
 *
 * Provides platform-specific SysTick implementation with global time counter.
 * This layer sits between the API Layer and Hardware Policy.
 *
 * Architecture:
 * - Maintains global tick counter (updated by ISR in board.cpp)
 * - Provides timing methods (micros, millis)
 * - Uses Hardware Policy for register access
 *
 * @note Part of Alloy HAL Platform Layer
 */

#pragma once

#include "core/types.hpp"
#include "hal/vendors/atmel/same70/systick_hardware_policy.hpp"

namespace alloy::hal::same70 {

using namespace alloy::core;

/**
 * @brief SysTick Platform Implementation for SAME70
 *
 * Template parameter allows different clock frequencies.
 *
 * @tparam SysTickPolicy Hardware policy (e.g., Same70SysTickHardwarePolicy<BASE, CLK>)
 */
template <typename SysTickPolicy>
class SysTickPlatform {
public:
    /**
     * @brief Global tick counter in microseconds
     *
     * This counter is incremented by the SysTick ISR (in board.cpp).
     * It's volatile because it's modified by ISR.
     */
    static inline volatile u64 tick_counter_us = 0;

    /**
     * @brief Configured tick period in microseconds
     *
     * Set by configure_ms() or configure_us().
     */
    static inline u32 tick_period_us = 1000;  // Default: 1ms

    /**
     * @brief Initialize SysTick with millisecond period
     *
     * Configures SysTick to interrupt every N milliseconds.
     *
     * @param ms Millisecond period (default: 1ms)
     */
    static void init_ms(u32 ms = 1) {
        tick_period_us = ms * 1000;
        SysTickPolicy::configure_ms(ms);
    }

    /**
     * @brief Initialize SysTick with microsecond period
     *
     * Configures SysTick to interrupt every N microseconds.
     *
     * @param us Microsecond period
     */
    static void init_us(u32 us) {
        tick_period_us = us;
        SysTickPolicy::configure_us(us);
    }

    /**
     * @brief Get current time in microseconds
     *
     * Provides high-precision timing by reading the SysTick counter
     * between interrupts.
     *
     * @return Microseconds since initialization
     */
    static u64 micros() {
        // Read SysTick counter (counts DOWN from LOAD to 0)
        u32 load = tick_period_us * (SysTickPolicy::cpu_clock_hz / 1000000);
        u32 val1 = SysTickPolicy::get_current_value();
        u64 tick1 = tick_counter_us;
        u32 val2 = SysTickPolicy::get_current_value();

        // If counter wrapped (interrupt occurred), use updated tick
        u64 base_us;
        u32 counter;

        if (val2 <= val1) {
            // Normal case: counter decreased
            base_us = tick1;
            counter = val1;
        } else {
            // Counter wrapped (interrupt occurred between reads)
            base_us = tick_counter_us;
            counter = val2;
        }

        // Calculate elapsed ticks within current period
        u32 elapsed_ticks = load - counter;
        u32 sub_period_us = (elapsed_ticks * tick_period_us) / (load + 1);

        return base_us + sub_period_us;
    }

    /**
     * @brief Get current time in milliseconds
     *
     * @return Milliseconds since initialization
     */
    static u32 millis() {
        return static_cast<u32>(tick_counter_us / 1000);
    }

    /**
     * @brief Increment tick counter (called by ISR)
     *
     * This should be called by the SysTick_Handler ISR in board.cpp.
     */
    static void increment_tick() {
        tick_counter_us += tick_period_us;
    }

    /**
     * @brief Reset tick counter
     *
     * Useful for resetting uptime or testing.
     */
    static void reset_counter() {
        tick_counter_us = 0;
    }

    /**
     * @brief Get configured tick period in microseconds
     *
     * @return Tick period in microseconds
     */
    static u32 get_tick_period_us() {
        return tick_period_us;
    }

    // Forward hardware policy methods for convenience
    static void enable() { SysTickPolicy::enable(); }
    static void disable() { SysTickPolicy::disable(); }
    static void enable_interrupt() { SysTickPolicy::enable_interrupt(); }
    static void disable_interrupt() { SysTickPolicy::disable_interrupt(); }
};

// ============================================================================
// Type Alias for SAME70 (12 MHz default, can be reconfigured)
// ============================================================================

/// Default SysTick instance for SAME70 at 12 MHz
template <u32 CPU_FREQ_HZ = 12000000>
using SysTick = SysTickPlatform<Same70SysTickHardwarePolicy<0xE000E010, CPU_FREQ_HZ>>;

}  // namespace alloy::hal::same70
