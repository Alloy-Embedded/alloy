/**
 * @file board.cpp
 * @brief SAME70 Xplained Ultra Board Implementation
 *
 * Implements hardware initialization and support for the SAME70 Xplained Ultra
 * development board. This file provides the concrete implementation of the
 * board interface defined in board.hpp.
 */

#include "board.hpp"
#include "hal/vendors/arm/cortex_m7/init_hooks.hpp"
#include "hal/api/clock_simple.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/api/watchdog_simple.hpp"
#include "hal/vendors/arm/same70/clock.hpp"
#include "hal/vendors/arm/same70/interrupt.hpp"
#include "hal/vendors/arm/same70/systick_platform.hpp"
#include "hal/vendors/atmel/same70/watchdog_hardware_policy.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"
#include <cstdint>

using namespace alloy::hal::same70;
using namespace alloy::generated::atsame70q21b;
using namespace alloy::hal;

namespace board {

// =============================================================================
// Internal State
// =============================================================================

// SysTick instance for timing (12 MHz clock)
using BoardSysTick = SysTick<12000000>;

// Initialization flag to prevent double-init
static bool board_initialized = false;


// =============================================================================
// Board Initialization
// =============================================================================

void init() {
    if (board_initialized) {
        return;
    }

    // Step 1: Disable watchdog timers
    // SAME70 has two independent watchdogs that must both be disabled for development
    using WDT_Policy = atmel::same70::Same70WatchdogHardwarePolicy<peripherals::WDT>;
    using RSWDT_Policy = atmel::same70::Same70WatchdogHardwarePolicy<peripherals::RSWDT>;
    Watchdog::disable<WDT_Policy>();
    Watchdog::disable<RSWDT_Policy>();

    // Step 2: Configure system clock
    // Using 12 MHz internal RC oscillator (safe default)
    auto clock_result = SystemClock::use_safe_default<Clock>();
    if (!clock_result.is_ok()) {
        // Clock initialization failed - system will continue at default frequency
    }

    // Step 3: Enable GPIO peripheral clocks
    // PIOA-PIOE correspond to peripheral IDs 10-14
    const u8 gpio_peripherals[] = {10, 11, 12, 13, 14};
    SystemClock::enable_peripherals<Clock>(gpio_peripherals, 5);

    // Step 4: Initialize SysTick timer (1ms period)
    SysTickTimer::init_ms<BoardSysTick>(1);

    // Step 5: Initialize board peripherals
    led::init();

    // Step 6: Enable interrupts
    Nvic::enable_global();

    // Step 7: Call platform-specific late initialization hook
    alloy::hal::arm::late_init();

    board_initialized = true;
}

} // namespace board

// =============================================================================
// Interrupt Service Routines
// =============================================================================

/**
 * @brief SysTick timer interrupt handler
 *
 * Called automatically every 1ms by the SysTick timer.
 * Updates the system time counter used by timing functions.
 * If RTOS is enabled, also forwards tick to RTOS scheduler.
 *
 * @note This overrides the weak default handler in startup code.
 */
extern "C" void SysTick_Handler() {
    // Update HAL tick (always)
    board::BoardSysTick::increment_tick();

    // Forward to RTOS scheduler (if enabled)
    #ifdef ALLOY_RTOS_ENABLED
        alloy::rtos::RTOS::tick();
    #endif
}
