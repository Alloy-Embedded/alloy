#pragma once

#include <stdint.h>

namespace alloy::hal::platform::arm {

/**
 * @brief Shared SysTick-based delay implementation for ARM Cortex-M
 *
 * This template provides a generic delay implementation using the ARM Cortex-M
 * SysTick timer. It can be instantiated by any ARM Cortex-M family by providing
 * the system clock frequency.
 *
 * Features:
 * - Precise millisecond and microsecond delays
 * - Uses SysTick hardware timer (standard on all ARM Cortex-M)
 * - Blocking implementation (busy-wait)
 * - No peripheral resources consumed
 *
 * Note: This implementation uses polling mode and will conflict with RTOS
 * if the RTOS is using SysTick for scheduling. For RTOS compatibility,
 * use RTOS-provided delay functions instead.
 *
 * @tparam SystemClockHz System clock frequency in Hz
 */
template<uint32_t SystemClockHz>
class SysTickDelay {
public:
    /**
     * @brief Initialize SysTick for delay functionality
     *
     * Configures SysTick to count down from a known value without interrupts.
     * Call this once during system initialization.
     */
    static void init() {
        // We'll configure SysTick on-demand in each delay function
        // to avoid conflicts with potential RTOS usage
    }

    /**
     * @brief Delay for specified milliseconds
     * @param ms Number of milliseconds to delay
     */
    static void delay_ms(uint32_t ms) {
        // Each tick represents one cycle of the system clock
        // For millisecond precision, we reload with (SystemClockHz / 1000)
        constexpr uint32_t TICKS_PER_MS = SystemClockHz / 1000;

        for (uint32_t i = 0; i < ms; i++) {
            delay_ticks(TICKS_PER_MS);
        }
    }

    /**
     * @brief Delay for specified microseconds
     * @param us Number of microseconds to delay
     */
    static void delay_us(uint32_t us) {
        // For microsecond precision
        constexpr uint32_t TICKS_PER_US = SystemClockHz / 1000000;

        // If the clock is too slow for microsecond precision, use minimum delay
        if constexpr (TICKS_PER_US == 0) {
            // Clock too slow for microsecond delays - use cycle-based fallback
            volatile uint32_t count = us * (SystemClockHz / 4000000);
            while (count--);
        } else {
            for (uint32_t i = 0; i < us; i++) {
                delay_ticks(TICKS_PER_US);
            }
        }
    }

    /**
     * @brief Delay for specified number of CPU cycles
     * @param cycles Number of CPU cycles to delay
     *
     * Note: This uses SysTick for cycle counting. For very short delays
     * (< 100 cycles), overhead may dominate.
     */
    static void delay_cycles(uint32_t cycles) {
        if (cycles == 0) return;

        // SysTick is a 24-bit down-counter
        constexpr uint32_t MAX_TICKS = 0x00FFFFFF;

        while (cycles > MAX_TICKS) {
            delay_ticks(MAX_TICKS);
            cycles -= MAX_TICKS;
        }

        if (cycles > 0) {
            delay_ticks(cycles);
        }
    }

private:
    /**
     * @brief Delay for specified number of SysTick ticks
     * @param ticks Number of ticks to delay (max 0x00FFFFFF)
     *
     * SysTick is a 24-bit down-counter that counts at the CPU clock rate.
     */
    static void delay_ticks(uint32_t ticks) {
        // SysTick registers (standard ARM Cortex-M addresses)
        volatile uint32_t* const SYST_CSR   = reinterpret_cast<volatile uint32_t*>(0xE000E010);
        volatile uint32_t* const SYST_RVR   = reinterpret_cast<volatile uint32_t*>(0xE000E014);
        volatile uint32_t* const SYST_CVR   = reinterpret_cast<volatile uint32_t*>(0xE000E018);

        // Disable SysTick
        *SYST_CSR = 0;

        // Set reload value (ticks - 1)
        *SYST_RVR = ticks - 1;

        // Clear current value
        *SYST_CVR = 0;

        // Enable SysTick: CPU clock, no interrupt, enable
        *SYST_CSR = 0x5;

        // Wait for COUNTFLAG (bit 16) to be set
        // This indicates the counter has reached 0
        while ((*SYST_CSR & (1U << 16)) == 0);

        // Disable SysTick
        *SYST_CSR = 0;
    }
};

}  // namespace alloy::hal::platform::arm
