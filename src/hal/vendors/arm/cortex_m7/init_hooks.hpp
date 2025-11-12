#pragma once

/**
 * @file init_hooks.hpp
 * @brief Initialization Hook System for ARM Cortex-M
 *
 * Provides flexible initialization points during startup sequence.
 * Applications can define these hooks to customize startup behavior.
 */

namespace alloy::hal::arm {

/**
 * @brief Initialization hook interface
 *
 * Applications can define these hooks to customize startup:
 *
 * - early_init():     Called BEFORE .data/.bss initialization
 * - pre_main_init():  Called AFTER .data/.bss, BEFORE main()
 * - late_init():      Called FROM main() via board::init()
 *
 * All hooks use weak symbols, so they're optional.
 * If not defined by application, default (empty) implementations are used.
 */

extern "C" {

/**
 * @brief Early initialization hook
 *
 * Called BEFORE .data and .bss initialization.
 *
 * Use for:
 * - Flash wait states configuration
 * - Early clock setup (if needed before .data copy)
 * - Watchdog disable
 * - Critical hardware initialization
 *
 * WARNING: Cannot access global variables yet!
 * Only registers and stack variables are safe to use.
 *
 * Example:
 * @code
 * extern "C" void early_init() {
 *     // Configure flash wait states for 300 MHz
 *     volatile uint32_t* EEFC_FMR = (uint32_t*)0x400E0A00;
 *     *EEFC_FMR = (6 << 8);  // 6 wait states
 * }
 * @endcode
 */
[[gnu::weak]]
void early_init();

/**
 * @brief Pre-main initialization hook
 *
 * Called AFTER .data/.bss initialization, BEFORE main().
 *
 * Use for:
 * - Clock system configuration (PLL, dividers)
 * - Memory controller setup
 * - Cache/MPU configuration
 * - Power management initialization
 *
 * CAN access global variables now.
 * Static constructors have NOT run yet.
 *
 * Example:
 * @code
 * extern "C" void pre_main_init() {
 *     // Configure PLL to 300 MHz
 *     configure_pll_300mhz();
 *
 *     // Enable instruction cache
 *     SCB->CCR |= SCB_CCR_IC_Msk;
 * }
 * @endcode
 */
[[gnu::weak]]
void pre_main_init();

/**
 * @brief Late initialization hook
 *
 * Called FROM main() via board::init().
 *
 * Use for:
 * - Peripheral initialization
 * - Board-specific setup
 * - Application-level initialization
 *
 * This is the RECOMMENDED hook for most use cases.
 * All C++ infrastructure is ready (global objects, static constructors).
 *
 * Example:
 * @code
 * extern "C" void late_init() {
 *     // Initialize UART console
 *     console::init();
 *
 *     // Initialize sensors
 *     sensor_board::init();
 * }
 * @endcode
 */
[[gnu::weak]]
void late_init();

} // extern "C"

/**
 * @brief Default implementations
 *
 * These are the weak default implementations.
 * They do nothing, so applications can selectively override only what they need.
 */

// Default early_init (does nothing)
__attribute__((weak))
void early_init() {
    // Default: no early initialization
}

// Default pre_main_init (does nothing)
__attribute__((weak))
void pre_main_init() {
    // Default: no pre-main initialization
}

// Default late_init (does nothing)
__attribute__((weak))
void late_init() {
    // Default: no late initialization
}

/**
 * @brief Hook execution order
 *
 * The complete startup sequence is:
 *
 * 1. Reset_Handler entry
 * 2. early_init()           ← HOOK 1 (before .data/.bss)
 * 3. Copy .data from flash to RAM
 * 4. Zero .bss section
 * 5. Call C++ static constructors
 * 6. pre_main_init()        ← HOOK 2 (after .data/.bss, before main)
 * 7. main()
 * 8. board::init()
 * 9. late_init()            ← HOOK 3 (from board::init)
 * 10. Application code
 */

/**
 * @brief Usage guidelines
 *
 * Choose the right hook for your needs:
 *
 * | Hook           | Global vars? | Constructors? | Use for                    |
 * |----------------|--------------|---------------|----------------------------|
 * | early_init     | ❌ NO        | ❌ NO         | Flash, critical HW         |
 * | pre_main_init  | ✅ YES       | ❌ NO         | Clocks, caches, memory     |
 * | late_init      | ✅ YES       | ✅ YES        | Peripherals, board setup   |
 *
 * Most applications should use late_init() only.
 */

} // namespace alloy::hal::arm
