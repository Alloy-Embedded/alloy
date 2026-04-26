/**
 * @file main.cpp
 * @brief Universal LED Blink Example
 *
 * Demonstrates board-independent LED control using the Alloy HAL framework.
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
 * The onboard green LED blinks with a 500ms ON / 500ms OFF pattern.
 *
 * ## How It Works
 *
 * The ALLOY_BOARD macro is defined by the build system based on the selected board.
 * The correct board header is included automatically using conditional compilation.
 * All board-specific details (clock, pins, peripherals) are handled by the board
 * layer, keeping the application code completely portable.
 *
 * @note This demonstrates the power of the Alloy HAL abstraction -
 *       **one codebase runs on multiple architectures** without modifications.
 */

// Include board support based on build configuration
#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F722ZE)
    #include "nucleo_f722ze/board.hpp"
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    #include "raspberry_pi_pico/board.hpp"
#elif defined(ALLOY_BOARD_ESP32_DEVKIT)
    #include "esp32_devkit/board.hpp"
#elif defined(ALLOY_BOARD_ESP_WROVER_KIT)
    #include "esp_wrover_kit/board.hpp"
#else
    #error "Unsupported board! Define ALLOY_BOARD_* in your build system."
#endif

#include <cstdint>

#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board_uart.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board_uart.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board_uart.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board_uart.hpp"
#elif defined(ALLOY_BOARD_RASPBERRY_PI_PICO)
    #include "raspberry_pi_pico/board_uart.hpp"
#endif

#include "../common/uart_console.hpp"

namespace {

inline void busy_delay() {
    for (volatile std::uint32_t i = 0; i < 2'000'000u; ++i) {
    }
}

}  // namespace

int main() {
    // Initialize all board hardware (clock, GPIO, SysTick, etc.)
    board::init();

    auto uart = board::make_debug_uart();
    const auto uart_ready = uart.configure().is_ok();
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "blink ready");
    }

    std::uint32_t loop_count = 0u;

    // Blink LED forever - same code works on all boards!
    while (true) {
        board::led::toggle();
        busy_delay();
        ++loop_count;
        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "blink loop=");
            alloy::examples::uart_console::write_unsigned(uart, loop_count);
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
    }

    return 0;
}
