// Alloy Framework - ARM Cortex-M SysTick Timer
//
// Provides system tick timer configuration and control functions
// Compatible with all ARM Cortex-M cores (M0/M0+/M3/M4/M7/M23/M33/M55)
//
// The SysTick timer is a 24-bit down counter that can be used for:
// - Time base for RTOS scheduling
// - General purpose timing
// - Periodic interrupts
//
// Functions provided:
// - Configure SysTick with reload value and interrupt enable
// - Enable/disable SysTick
// - Get current counter value
// - Check if counter reached zero

#pragma once

#include <stdint.h>

#include "core_common.hpp"

namespace alloy::arm::cortex_m::systick {

/// Configure SysTick timer
/// @param ticks: Reload value (1-0xFFFFFF). Timer counts down from this value.
/// @param enable_interrupt: If true, SysTick exception is enabled
/// @return true if configuration successful, false if ticks out of range
///
/// Example: Configure for 1ms tick at 72MHz:
///   configure(72000 - 1, true);
inline bool configure(uint32_t ticks, bool enable_interrupt = true) {
    // Check reload value is valid (24-bit)
    if (ticks > 0xFFFFFF || ticks == 0) {
        return false;
    }

    // Disable SysTick during configuration
    SYSTICK()->CTRL = 0;

    // Set reload value (counts from this value down to 0)
    SYSTICK()->LOAD = ticks - 1;

    // Reset current value to 0
    SYSTICK()->VAL = 0;

    // Configure and enable:
    // - Use processor clock
    // - Enable/disable interrupt
    // - Enable counter
    uint32_t ctrl = systick_ctrl::CLKSOURCE | systick_ctrl::ENABLE;
    if (enable_interrupt) {
        ctrl |= systick_ctrl::TICKINT;
    }

    SYSTICK()->CTRL = ctrl;

    return true;
}

/// Disable SysTick timer
inline void disable() {
    SYSTICK()->CTRL = 0;
}

/// Get current SysTick counter value
/// @return Current counter value (0 to reload value)
inline uint32_t get_value() {
    return SYSTICK()->VAL;
}

/// Get SysTick reload value
/// @return Reload value
inline uint32_t get_reload() {
    return SYSTICK()->LOAD;
}

/// Check if SysTick has counted to zero since last check
/// @return true if counter reached zero
/// Note: This bit is cleared on read
inline bool has_counted_to_zero() {
    return (SYSTICK()->CTRL & systick_ctrl::COUNTFLAG) != 0;
}

/// Check if SysTick is enabled
/// @return true if SysTick is enabled
inline bool is_enabled() {
    return (SYSTICK()->CTRL & systick_ctrl::ENABLE) != 0;
}

/// Configure SysTick for millisecond timing
/// @param cpu_freq_hz: CPU frequency in Hz
/// @param ms: Milliseconds per tick (typically 1)
/// @param enable_interrupt: If true, SysTick exception is enabled
/// @return true if configuration successful
///
/// Example: Configure for 1ms tick at 72MHz:
///   configure_ms(72000000, 1, true);
inline bool configure_ms(uint32_t cpu_freq_hz, uint32_t ms, bool enable_interrupt = true) {
    uint64_t ticks = (static_cast<uint64_t>(cpu_freq_hz) * ms) / 1000;

    // Check if ticks fit in 24-bit reload register
    if (ticks > 0xFFFFFF || ticks == 0) {
        return false;
    }

    return configure(static_cast<uint32_t>(ticks), enable_interrupt);
}

/// Configure SysTick for microsecond timing
/// @param cpu_freq_hz: CPU frequency in Hz
/// @param us: Microseconds per tick
/// @param enable_interrupt: If true, SysTick exception is enabled
/// @return true if configuration successful
///
/// Example: Configure for 100us tick at 72MHz:
///   configure_us(72000000, 100, true);
inline bool configure_us(uint32_t cpu_freq_hz, uint32_t us, bool enable_interrupt = true) {
    uint64_t ticks = (static_cast<uint64_t>(cpu_freq_hz) * us) / 1000000;

    // Check if ticks fit in 24-bit reload register
    if (ticks > 0xFFFFFF || ticks == 0) {
        return false;
    }

    return configure(static_cast<uint32_t>(ticks), enable_interrupt);
}

/// Simple delay using SysTick (blocking)
/// @param ticks: Number of SysTick ticks to wait
/// Note: SysTick must be already configured and running
///
/// Example: Delay for 1000 ticks:
///   delay_ticks(1000);
inline void delay_ticks(uint32_t ticks) {
    while (ticks > 0) {
        if (has_counted_to_zero()) {
            --ticks;
        }
    }
}

}  // namespace alloy::arm::cortex_m::systick
