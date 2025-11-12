/**
 * @file board.cpp
 * @brief SAME70 Xplained Ultra - Board Implementation
 *
 * Implements the standard board interface for SAME70 Xplained Ultra.
 * Provides hardware initialization and system utilities using direct
 * register access for minimal dependencies.
 */

#include "board.hpp"
#include "hal/vendors/arm/cortex_m7/init_hooks.hpp"
#include <cstdint>

namespace board {

// =============================================================================
// System State
// =============================================================================

/// Global tick counter (incremented by SysTick_Handler every 1ms)
volatile uint32_t system_ticks_ms = 0;

/// Initialization flag to prevent double-init
static bool board_initialized = false;

// =============================================================================
// Board Initialization
// =============================================================================

void init() {
    if (board_initialized) {
        return;  // Already initialized
    }

    // CRITICAL: Disable Watchdog Timer first!
    // The watchdog is enabled by default and will reset the system after ~16 seconds
    volatile uint32_t* WDT_MR = (volatile uint32_t*)0x400E1854;
    *WDT_MR = (1 << 15);  // WDDIS bit - disable watchdog

    // Enable peripheral clocks for GPIO ports
    // PMC_PCER0 register at 0x400E0610
    volatile uint32_t* PMC_PCER0 = (volatile uint32_t*)0x400E0610;
    *PMC_PCER0 = (1u << 10)  // PIOA (peripheral ID 10)
               | (1u << 11)  // PIOB (peripheral ID 11)
               | (1u << 12)  // PIOC (peripheral ID 12)
               | (1u << 13)  // PIOD (peripheral ID 13)
               | (1u << 14); // PIOE (peripheral ID 14)

    // TODO: Initialize PLL for 300 MHz operation
    // For now, running on 12 MHz RC oscillator (reset default)
    // This is sufficient for LED blink but UART/peripherals will be slow

    // Configure SysTick for 1ms ticks (direct register access)
    // At 12 MHz: reload value = 12000 - 1
    constexpr uint32_t cpu_freq_hz = 12'000'000;  // RC oscillator default
    constexpr uint32_t tick_freq_hz = 1'000;      // 1 kHz = 1ms ticks
    constexpr uint32_t reload_value = (cpu_freq_hz / tick_freq_hz) - 1;

    // SysTick registers (ARM Cortex-M standard)
    volatile uint32_t* SYST_CSR = (volatile uint32_t*)0xE000E010;  // Control and Status
    volatile uint32_t* SYST_RVR = (volatile uint32_t*)0xE000E014;  // Reload Value

    *SYST_RVR = reload_value;  // Set reload value
    *SYST_CSR = 0x00000007;    // Enable: ENABLE | TICKINT | CLKSOURCE (processor clock)

    // Initialize LED GPIO
    led::init();

    // Enable global interrupts at CPU level (CPSIE I instruction)
    __asm volatile ("cpsie i" ::: "memory");

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
 * Called every 1ms to increment system tick counter.
 * This handler overrides the weak default in startup.cpp.
 */
extern "C" void SysTick_Handler() {
    board::system_ticks_ms++;
}
