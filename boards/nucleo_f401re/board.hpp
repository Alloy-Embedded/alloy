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

#include "hal/systick.hpp"

#include "board_config.hpp"
#include "device/system_clock.hpp"

namespace board {

using namespace nucleo_f401re;
using namespace alloy::hal;
// Board-specific SysTick type (84 MHz)
using BoardSysTick = alloy::hal::cortex_m::SysTick<ClockConfig::system_clock_hz>;

// RTOS Tick Source (must be 1ms tick for RTOS compatibility)
// Note: The 1ms tick period is configured at runtime via SysTickTimer::init_ms<BoardSysTick>(1)
using RTOSTick = BoardSysTick;

/**
 * @brief Initialize all board hardware
 *
 * This function:
 * 1. Configures system clock to 84 MHz
 * 2. Initializes SysTick timer (1ms tick)
 * 3. Initializes board resources through descriptor-driven drivers
 * 4. Enables interrupts
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
}  // namespace led

}  // namespace board
