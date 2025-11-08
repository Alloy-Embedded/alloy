#pragma once

#include <stdint.h>

namespace alloy::hal::platform::arm {

/**
 * @brief SysTick timer for system time tracking (RTOS timebase)
 *
 * This provides a system tick counter suitable for RTOS scheduling.
 * Unlike systick_delay.hpp (which uses SysTick in polling mode), this
 * configures SysTick to run continuously with interrupts enabled.
 *
 * Features:
 * - Configurable tick rate (typically 1ms for RTOS)
 * - 64-bit tick counter (never overflows in practice)
 * - Thread-safe tick counter update
 * - Can be used as timebase for FreeRTOS, custom RTOS, or application timing
 *
 * Usage:
 * 1. Call init() during startup with desired tick frequency
 * 2. Implement SysTick_Handler() in your code to call tick()
 * 3. Use get_ticks() to read elapsed ticks
 * 4. Use get_time_ms() / get_time_us() for time measurements
 *
 * @tparam SystemClockHz System clock frequency in Hz
 */
template <uint32_t SystemClockHz>
class SysTick {
   public:
    /**
     * @brief Initialize SysTick for continuous operation
     * @param tick_rate_hz Desired tick rate (e.g., 1000 for 1ms ticks)
     *
     * Common tick rates:
     * - 1000 Hz (1ms) - Standard for most RTOS
     * - 10000 Hz (100us) - High resolution timing
     * - 100 Hz (10ms) - Low overhead
     */
    static void init(uint32_t tick_rate_hz = 1000) {
        // Calculate reload value for desired tick rate
        // SysTick counts down from LOAD to 0
        uint32_t reload_value = (SystemClockHz / tick_rate_hz) - 1;

        // Ensure reload value fits in 24 bits
        if (reload_value > 0x00FFFFFF) {
            // Tick rate too high for system clock
            reload_value = 0x00FFFFFF;
        }

        // Store tick period for time calculations
        tick_period_us = 1000000 / tick_rate_hz;

        // SysTick registers (standard ARM Cortex-M addresses)
        volatile uint32_t* const SYST_CSR = reinterpret_cast<volatile uint32_t*>(0xE000E010);
        volatile uint32_t* const SYST_RVR = reinterpret_cast<volatile uint32_t*>(0xE000E014);
        volatile uint32_t* const SYST_CVR = reinterpret_cast<volatile uint32_t*>(0xE000E018);

        // Disable SysTick during configuration
        *SYST_CSR = 0;

        // Set reload value
        *SYST_RVR = reload_value;

        // Clear current value
        *SYST_CVR = 0;

        // Enable SysTick: CPU clock source, interrupt enabled, counter enabled
        // Bit 0: ENABLE - Enable counter
        // Bit 1: TICKINT - Enable interrupt
        // Bit 2: CLKSOURCE - Use processor clock
        *SYST_CSR = 0x7;

        // Reset tick counter
        tick_counter = 0;
    }

    /**
     * @brief Increment tick counter (call from interrupt handler)
     *
     * This should be called from your SysTick_Handler() interrupt:
     *
     * extern "C" void SysTick_Handler() {
     *     SysTick::tick();
     * }
     */
    static inline void tick() { tick_counter++; }

    /**
     * @brief Get current tick count
     * @return Number of ticks since init()
     */
    static inline uint64_t get_ticks() {
        // Read is atomic on Cortex-M (32-bit aligned)
        // For extra safety on tick_counter update, could disable interrupts
        return tick_counter;
    }

    /**
     * @brief Get elapsed time in milliseconds
     * @return Milliseconds since init()
     */
    static inline uint64_t get_time_ms() { return (get_ticks() * tick_period_us) / 1000; }

    /**
     * @brief Get elapsed time in microseconds
     * @return Microseconds since init()
     */
    static inline uint64_t get_time_us() { return get_ticks() * tick_period_us; }

    /**
     * @brief Get elapsed time in seconds
     * @return Seconds since init()
     */
    static inline uint32_t get_time_s() { return static_cast<uint32_t>(get_time_ms() / 1000); }

    /**
     * @brief Delay using busy-wait based on tick counter
     * @param ms Milliseconds to delay
     *
     * This is a blocking delay that polls the tick counter.
     * Unlike systick_delay.hpp, this doesn't reconfigure SysTick.
     */
    static void delay_ms(uint32_t ms) {
        uint64_t start = get_ticks();
        uint64_t ticks_to_wait = (ms * 1000) / tick_period_us;

        while ((get_ticks() - start) < ticks_to_wait) {
            // Could use WFI (Wait For Interrupt) here for power saving
            __asm__ volatile("nop");
        }
    }

    /**
     * @brief Get current tick period in microseconds
     * @return Microseconds per tick
     */
    static inline uint32_t get_tick_period_us() { return tick_period_us; }

    /**
     * @brief Reset tick counter to zero
     */
    static inline void reset() { tick_counter = 0; }

   private:
    // 64-bit counter - will never overflow in practice
    // At 1000 Hz: overflows after ~584 million years
    static inline volatile uint64_t tick_counter = 0;

    // Tick period in microseconds (calculated during init)
    static inline uint32_t tick_period_us = 1000;  // Default 1ms
};

}  // namespace alloy::hal::platform::arm
