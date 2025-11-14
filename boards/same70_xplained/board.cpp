/**
 * @file board.cpp
 * @brief SAME70 Xplained Ultra - Board Implementation
 *
 * Implements the standard board interface for SAME70 Xplained Ultra.
 * Uses HAL abstractions for clock and peripheral management.
 */

#include "board.hpp"
#include "hal/vendors/arm/cortex_m7/init_hooks.hpp"
#include "hal/platform/same70/clock.hpp"
#include "hal/platform/same70/interrupt.hpp"
#include "hal/vendors/atmel/same70/systick_hardware_policy.hpp"
#include <cstdint>

using namespace alloy::hal::same70;

namespace board {

// =============================================================================
// System State
// =============================================================================

/// Global tick counter in microseconds (incremented by SysTick_Handler every 1000us)
volatile uint64_t system_tick_us = 0;

/// Initialization flag to prevent double-init
static bool board_initialized = false;


// =============================================================================
// Board Initialization
// =============================================================================

void init() {
    if (board_initialized) {
        return;  // Already initialized
    }

    // CRITICAL: Disable BOTH Watchdog Timers first!
    // SAME70 has TWO watchdogs that must BOTH be disabled:
    //
    // 1. WDT (Watchdog Timer) at 0x400E1800
    //    - WDT_MR at offset 0x54 = 0x400E1854
    //    - Set bit 15 (WDDIS) to disable
    //
    // 2. RSWDT (Reinforced Secure Watchdog Timer) at 0x400E1900
    //    - RSWDT_MR at offset 0x04 = 0x400E1904
    //    - Set bit 15 (WDDIS) to disable
    //    - CRITICAL: Bits [11:0] MUST be set to 0xFFF (ALLONES field)
    //    - This is a SAME70-specific requirement per datasheet
    //
    // Both watchdogs are enabled by default and will reset the system!

    // Disable WDT (standard watchdog)
    volatile uint32_t* WDT_MR = (volatile uint32_t*)0x400E1854;
    *WDT_MR = (1 << 15);  // WDDIS bit

    // Disable RSWDT (reinforced secure watchdog)
    // Value: 0x8FFF = bit 15 (WDDIS) + bits [11:0] (ALLONES = 0xFFF)
    volatile uint32_t* RSWDT_MR = (volatile uint32_t*)0x400E1904;
    *RSWDT_MR = 0x00008FFF;  // WDDIS=1, ALLONES=0xFFF

    // Configure system clock to 12MHz using internal RC oscillator
    // NOTE: PLL configuration is currently not working (see docs/KNOWN_ISSUES.md)
    // Using 12MHz RC oscillator without PLL as a stable configuration
    auto clock_result = Clock::initialize(CLOCK_CONFIG_12MHZ_RC);

    uint32_t cpu_freq = 12000000;  // 12 MHz

    if (!clock_result.is_ok()) {
        // Clock initialization failed - this should not happen with RC oscillator
        // System will run at default 12MHz RC (same frequency, but not configured through HAL)
    }

    // Enable peripheral clocks for GPIO ports using Clock abstraction
    Clock::enablePeripheralClock(10);  // PIOA
    Clock::enablePeripheralClock(11);  // PIOB
    Clock::enablePeripheralClock(12);  // PIOC
    Clock::enablePeripheralClock(13);  // PIOD
    Clock::enablePeripheralClock(14);  // PIOE

    // Configure SysTick for 1ms ticks at 12MHz
    using SysTickPolicy = Same70SysTickHardwarePolicy<0xE000E010, 12000000>;
    SysTickPolicy::configure_ms(1);

    // Initialize LED GPIO
    led::init();

    // Enable global interrupts using interrupt controller abstraction
    Nvic::enable_global();

    // Call late initialization hook (for application-specific setup)
    alloy::hal::arm::late_init();

    board_initialized = true;
}

} // namespace board

// =============================================================================
// Interrupt Service Routines
// =============================================================================

/**
 * @brief SysTick interrupt handler
 *
 * Called every 1ms to increment system tick counter by 1000 µs.
 * This handler overrides the weak default in startup.cpp.
 */
extern "C" void SysTick_Handler() {
    // Reading CTRL register clears the COUNTFLAG
    volatile uint32_t ctrl = *reinterpret_cast<volatile uint32_t*>(0xE000E010);
    (void)ctrl;

    // Increment by 1000 microseconds (1ms)
    board::system_tick_us += 1000;
}
