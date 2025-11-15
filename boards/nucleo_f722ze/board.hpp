#pragma once

/**
 * @file board.hpp
 * @brief Board abstraction for Nucleo-F722ZE
 *
 * Provides a unified API for board-level functionality:
 * - Board initialization
 * - LED control
 * - Button access
 * - SysTick timer
 */

#include "board_config.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/vendors/st/stm32f7/systick_platform.hpp"

namespace board {

using namespace nucleo_f722ze;
using namespace alloy::hal;
using namespace alloy::hal::st::stm32f7;

// Board-specific SysTick type (180 MHz)
using BoardSysTick = SysTick<ClockConfig::system_clock_hz>;

// RTOS Tick Source (must be 1ms tick for RTOS compatibility)
// Note: The 1ms tick period is configured at runtime via SysTickTimer::init_ms<BoardSysTick>(1)
using RTOSTick = BoardSysTick;

/**
 * @brief Initialize all board hardware
 *
 * This function:
 * 1. Configures system clock to 216 MHz
 * 2. Enables GPIO peripheral clocks
 * 3. Initializes SysTick timer (1ms tick)
 * 4. Initializes LED GPIO
 * 5. Enables interrupts
 */
void init();

/**
 * @brief LED control namespace
 */
namespace led {
    /**
     * @brief Initialize LED GPIO
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
}

} // namespace board
