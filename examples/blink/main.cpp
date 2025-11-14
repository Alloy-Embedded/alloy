/**
 * @file main.cpp
 * @brief Universal LED Blink Example
 *
 * Demonstrates board-independent LED control using the CoreZero HAL framework.
 * This example works on any supported board by simply changing the ALLOY_BOARD define.
 *
 * ## What This Example Shows
 *
 * - Board initialization with `board::init()`
 * - LED control using portable `board::led` API
 * - Delays using `SysTickTimer` API
 * - **Board-independent code** - same source for all targets
 *
 * ## Supported Boards
 *
 * - **same70_xplained:** SAME70 Xplained Ultra (green LED on PC8)
 * - **nucleo_g0b1re:** STM32 Nucleo-G0B1RE (green LED on PA5)
 * - **nucleo_g071rb:** STM32 Nucleo-G071RB (green LED on PA5)
 * - **nucleo_f401re:** STM32 Nucleo-F401RE (green LED on PA5)
 *
 * ## Expected Behavior
 *
 * The onboard green LED blinks with a 500ms ON / 500ms OFF pattern (1 Hz).
 *
 * ## How It Works
 *
 * The ALLOY_BOARD macro is defined by the build system based on the selected board.
 * The correct board header is included automatically using conditional compilation.
 * All board-specific details (clock, pins, peripherals) are handled by the board
 * layer, keeping the application code completely portable.
 *
 * @note This demonstrates the power of the CoreZero HAL abstraction -
 *       **one codebase runs on multiple architectures** without modifications.
 */

// Include board support based on build configuration
#if defined(ALLOY_BOARD_SAME70_XPLAINED)
    #include "same70_xplained/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#else
    #error "Unsupported board! Define ALLOY_BOARD_* in your build system."
#endif

#include "hal/api/systick_simple.hpp"

using namespace alloy::hal;

int main() {
    // Initialize all board hardware (clock, GPIO, SysTick, etc.)
    board::init();

    // Blink LED forever - same code works on all boards!
    while (true) {
        board::led::on();
        SysTickTimer::delay_ms<board::BoardSysTick>(500);

        board::led::off();
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }

    return 0;
}
