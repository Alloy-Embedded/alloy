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
#include "hal/api/gpio_simple.hpp"
#include "hal/platform/same70/gpio.hpp"
#include "hal/platform/same70/systick_platform.hpp"
#include "hal/vendors/atmel/same70/atsame70q21b/peripherals.hpp"
#include <cstdint>

namespace board {

using namespace same70_xplained;
using namespace alloy::generated::atsame70q21b;
using namespace alloy::hal::same70;
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
using BoardSysTick = SysTick<12000000>;

/**
 * @brief RTOS Tick Source (must be 1ms tick for RTOS compatibility)
 */
using RTOSTick = BoardSysTick;
static_assert(BoardSysTick::tick_period_ms == 1,
              "RTOS requires 1ms SysTick period");

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

static auto led_pin = []() {
    if (LedConfig::led_green_active_high) {
        return Gpio::output<LedConfig::led_green>();
    } else {
        return Gpio::output_active_low<LedConfig::led_green>();
    }
}();

/**
 * @brief Initialize LED GPIO
 *
 * Configures the LED pin as output and ensures it starts in OFF state.
 * Called automatically by board::init().
 */
inline void init() {
    led_pin.off();
}

/**
 * @brief Turn LED on
 */
inline void on() {
    led_pin.on();
}

/**
 * @brief Turn LED off
 */
inline void off() {
    led_pin.off();
}

/**
 * @brief Toggle LED state
 */
inline void toggle() {
    led_pin.toggle();
}

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
