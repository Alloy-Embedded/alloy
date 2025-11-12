/**
 * @file board.cpp
 * @brief SAME70 Xplained Ultra Board Implementation
 *
 * Uses modern C++23 startup with initialization hooks.
 */

#include "board.hpp"
#include "hal/vendors/atmel/same70/systick_hardware_policy.hpp"
#include "hal/vendors/arm/cortex_m7/init_hooks.hpp"

namespace board {

// Global tick counter
volatile uint32_t system_ticks_ms = 0;

void init() {
    using SysTick = alloy::hal::same70::SysTickHardware;

    // TODO: Configure clocks to 300MHz
    // For now, assume bootloader/reset default

    // Configure SysTick for 1ms ticks
    SysTick::configure_ms(1);

    // Initialize LED
    led::init();

    // Call late initialization hook (allows application customization)
    alloy::hal::arm::late_init();
}

} // namespace board

// =============================================================================
// Interrupt Handlers
// =============================================================================

/**
 * @brief SysTick interrupt handler
 * 
 * Increments system tick counter every 1ms
 */
extern "C" void SysTick_Handler() {
    board::system_ticks_ms++;
}
