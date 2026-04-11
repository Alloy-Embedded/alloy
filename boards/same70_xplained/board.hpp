#pragma once

/**
 * @file board.hpp
 * @brief SAME70 Xplained Ultra Board Support
 *
 * Provides a unified interface for the SAME70 Xplained Ultra development board.
 * This abstraction allows applications to be portable across different boards.
 *
 * ## Board Interface
 *
 * **Initialization:**
 * - `board::init()` - Initialize all board hardware
 *
 * **LED Control:**
 * - `board::led::on()` - Turn LED on
 * - `board::led::off()` - Turn LED off
 * - `board::led::toggle()` - Toggle LED state
 *
 * **Timing Functions:**
 * For delays and timing, use the SysTick API:
 * ```cpp
 * #include "hal/api/systick_simple.hpp"
 *
 * SysTickTimer::delay_ms<board::BoardSysTick>(100);  // 100ms delay
 * uint32_t uptime = SysTickTimer::millis<board::BoardSysTick>();
 * ```
 *
 * ## Hardware Configuration
 *
 * - **MCU:** ATSAME70Q21B (ARM Cortex-M7 @ 12 MHz)
 * - **LED:** Green LED on PC8 (active LOW)
 * - **Button:** SW0 on PA11 (active LOW)
 * - **Debug UART:** UART1 on EDBG virtual COM port
 *
 * @note Clock runs at 12 MHz using internal RC oscillator
 */

#include "board_config.hpp"
#include "hal/gpio.hpp"
#include "hal/vendors/arm/same70/systick_platform.hpp"
#include <cstdint>

namespace board {

using namespace same70_xplained;

// =============================================================================
// Timing
// =============================================================================

/**
 * @brief SysTick instance for timing functions
 *
 * Use this type with SysTickTimer API for delays and timing:
 * @code
 * SysTickTimer::delay_ms<board::BoardSysTick>(100);
 * @endcode
 */
using BoardSysTick = alloy::hal::same70::SysTick<12000000>;

/**
 * @brief RTOS Tick Source (must be 1ms tick for RTOS compatibility)
 */
using RTOSTick = BoardSysTick;

// =============================================================================
// LED Control
// =============================================================================

/**
 * @brief LED control interface
 *
 * Provides simple control of the onboard green LED.
 * The LED polarity is automatically handled (active-low on this board).
 */
namespace led {
/**
 * @brief Initialize LED GPIO
 *
 * Configures the LED pin as output and ensures it starts in OFF state.
 * Called automatically by board::init().
 */
void init();

/**
 * @brief Turn LED on
 */
void on();

/**
 * @brief Turn LED off
 */
void off();

/**
 * @brief Toggle LED state
 */
void toggle();

} // namespace led

// =============================================================================
// Board Initialization
// =============================================================================

/**
 * @brief Initialize board hardware
 *
 * Initializes all board peripherals and subsystems:
 * - Disables watchdog timers
 * - Configures system clock (12 MHz RC oscillator)
 * - Enables GPIO peripheral clocks
 * - Initializes SysTick timer (1ms period)
 * - Configures LED GPIO
 * - Enables interrupts
 *
 * Must be called at the beginning of main() before using any board functions.
 *
 * @code
 * int main() {
 *     board::init();
 *     // Your application code here
 * }
 * @endcode
 */
void init();

} // namespace board
