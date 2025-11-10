/**
 * @file main.cpp
 * @brief Simple LED Blink using Board Abstraction Layer
 *
 * This example demonstrates the board abstraction layer with SysTick delays.
 * The code is portable and works on any supported board!
 *
 * Features:
 * - board::init() - Initializes clock, SysTick, and GPIO automatically
 * - board::delay_ms() - Precise millisecond delays using SysTick timer
 * - board::led::toggle() - Hardware-independent LED control
 *
 * Expected Behavior:
 * - LED blinks with precise 500ms timing
 * - Continues indefinitely
 *
 * Build: cmake --build build-same70 --target blink_led
 * Flash: ./scripts/flash_with_bossa.sh build-same70/examples/blink_led/blink_led.bin
 *
 * @note SysTick_Handler is defined in board_config.cpp
 */

#include "same70_xplained/board.hpp"

// Use explicit namespace to avoid any ambiguity
using namespace alloy::boards::same70_xplained;

int main() {
    // Initialize board: Clock + SysTick + GPIO + Disable Watchdog
    board::init();

    // Blink LED forever with precise 500ms timing using SysTick
    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }

    return 0;
}
