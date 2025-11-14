/**
 * @file main.cpp
 * @brief LED Blink Example
 *
 * Demonstrates basic board initialization and LED control using the CoreZero HAL.
 *
 * ## What This Example Shows
 *
 * - Board initialization with `board::init()`
 * - LED control using `board::led` API
 * - Delays using `SysTickTimer` API
 * - Clean, portable code structure
 *
 * ## Hardware Requirements
 *
 * - **Board:** SAME70 Xplained Ultra
 * - **LED:** Onboard green LED (PC8)
 * - **Power:** USB or external 5V
 *
 * ## Expected Behavior
 *
 * The green LED blinks with a 500ms ON / 500ms OFF pattern (1 Hz).
 *
 * ## Code Structure
 *
 * The example demonstrates the layered HAL architecture:
 * - Application code uses high-level board and timer APIs
 * - Board layer handles hardware initialization
 * - HAL APIs provide portable interfaces
 * - Platform layer manages hardware-specific details
 *
 * @note To port this example to another board, simply change the board include
 *       and update the board::init() call. The LED and timing APIs remain the same.
 */

#include "same70_xplained/board.hpp"
#include "hal/api/systick_simple.hpp"

using namespace alloy::hal;

int main() {
    // Initialize all board hardware
    board::init();

    // Blink LED forever
    while (true) {
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(500);

        board::led::off();
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }

    return 0;
}
