/**
 * @file main.cpp
 * @brief Portable LED Blink Example - Works on Any Board!
 *
 * This example demonstrates the power of board abstraction.
 * The SAME CODE compiles for different boards by simply changing
 * the build configuration - no code changes needed!
 *
 * Features:
 * - board::init() - Initializes clock, SysTick, and GPIO automatically
 * - board::delay_ms() - Precise millisecond delays using SysTick timer
 * - board::led::toggle() - Hardware-independent LED control
 *
 * Expected Behavior:
 * - LED blinks with precise 500ms ON/OFF timing
 * - Continues indefinitely
 *
 * Build for SAME70:
 *   make same70-blink
 *
 * Build for other boards (when available):
 *   make BOARD=stm32f103_bluepill blink
 *   make BOARD=arduino_zero blink
 *
 * The magic: Each board has its own board.hpp that implements
 * the same interface (board::init, board::led::toggle, etc.)
 */

// Include board-specific header based on build configuration
// CMake sets the board include path, so we can use a simple include
#include "board.hpp"

int main() {
    // Initialize board hardware (clocks, timers, GPIO)
    // This function is board-specific but has the same interface
    board::init();

    // Blink LED forever with precise 500ms timing
    // Same code works on all boards!
    while (true) {
        board::led::toggle();
        board::delay_ms(500);
    }

    return 0;
}
