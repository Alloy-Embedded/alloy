#pragma once

/**
 * @file board.hpp
 * @brief Nucleo-G0B1RE Board Support
 *
 * Provides a unified interface for the STM32 Nucleo-G0B1RE development board.
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
 * #include "hal/systick.hpp"
 *
 * SysTickTimer::delay_ms<board::BoardSysTick>(100);  // 100ms delay
 * uint32_t uptime = SysTickTimer::millis<board::BoardSysTick>();
 * ```
 *
 * ## Hardware Configuration
 *
 * - **MCU:** STM32G0B1RET6 (ARM Cortex-M0+ @ 64 MHz)
 * - **LED:** Green LED (LD4) on PA5 (active HIGH)
 * - **Button:** User button (B1) on PC13 (active LOW)
 * - **Debug UART:** USART2 on ST-Link virtual COM port (PA2/PA3)
 *
 * @note Clock runs at 64 MHz using HSI oscillator
 */

#include <cstdint>

#include "device/system_clock.hpp"
#include "hal/systick.hpp"

#include "board_config.hpp"

namespace board {

using namespace nucleo_g0b1re;
using namespace alloy::hal;

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
using BoardSysTick = alloy::hal::cortex_m::SysTick<ClockConfig::system_clock_hz>;

/**
 * @brief RTOS Tick Source (must be 1ms tick for RTOS compatibility)
 *
 * Note: The 1ms tick period is configured at runtime via SysTickTimer::init_ms<BoardSysTick>(1)
 */
using RTOSTick = BoardSysTick;

// =============================================================================
// LED Control
// =============================================================================

/**
 * @brief LED control interface
 *
 * Provides simple control of the onboard green LED (LD4).
 * The LED polarity is automatically handled (active-high on this board).
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

}  // namespace led

// =============================================================================
// Board Initialization
// =============================================================================

/**
 * @brief Initialize board hardware
 *
 * Initializes all board peripherals and subsystems:
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

}  // namespace board
