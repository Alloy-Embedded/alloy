#pragma once

/**
 * @file board.hpp
 * @brief Board abstraction for Nucleo-F401RE
 *
 * Provides a unified API for board-level functionality:
 * - Board initialization
 * - LED control
 * - Button access
 * - SysTick timer
 */

#include "board_config.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/platform/st/stm32f4/systick_platform.hpp"

namespace board {

using namespace nucleo_f401re;
using namespace alloy::hal;
using namespace alloy::hal::st::stm32f4;

// Board-specific SysTick type (84 MHz)
using BoardSysTick = SysTick<ClockConfig::system_clock_hz>;

/**
 * @brief Initialize all board hardware
 *
 * This function:
 * 1. Configures system clock to 84 MHz
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
