/**
 * @file main.cpp
 * @brief UART Logger Example
 *
 * Demonstrates how to use the logger system with UART output on embedded systems.
 *
 * ## What This Example Shows
 *
 * - Board initialization
 * - UART configuration for logging output
 * - Logger setup with UART sink
 * - Different log levels (INFO, WARN, ERROR, DEBUG)
 * - Periodic logging with timestamps
 *
 * ## Hardware Requirements
 *
 * - **Board:** Supported board with `board_uart.hpp`
 * - **UART:** Debug UART (EDBG virtual COM port)
 * - **Baud Rate:** 115200
 * - **Connection:** USB cable to PC (EDBG connector)
 *
 * ## Expected Behavior
 *
 * The example outputs log messages to UART every second:
 * - System startup message
 * - Periodic INFO messages with uptime
 * - Occasional WARN and ERROR messages for demonstration
 * - LED blinks in sync with logging
 *
 * ## Viewing Output
 *
 * 1. Connect board via USB (EDBG port)
 * 2. Open serial terminal (115200 baud, 8N1)
 * 3. Flash and run the example
 * 4. Observe formatted log messages with timestamps
 *
 * @note UART TX pin must be configured correctly for your board.
 *       Consult board documentation for UART pin mapping.
 */

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "uart_logger requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER

#include "hal/systick.hpp"

#include "logger/logger.hpp"
#include "logger/uart_logger.hpp"

namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (const auto result = uart.configure(); result.is_err()) {
        blink_error(100);
    }

    auto app_logger = alloy::logger::make_uart_logger(uart, {
        .default_level = alloy::logger::Level::Info,
        .enable_timestamps = false,
        .enable_colors = false,
        .enable_source_location = true,
        .timestamp_precision = alloy::logger::TimestampPrecision::Milliseconds,
        .line_ending = alloy::logger::LineEnding::CRLF,
    });

    LOG_INFO_TO(app_logger, "uart logger ready");

    std::uint32_t loop_count = 0;
    while (true) {
        LOG_INFO_TO(app_logger, "heartbeat loop=%lu", static_cast<unsigned long>(loop_count));
        if ((loop_count % 5u) == 0u) {
            LOG_WARN_TO(app_logger, "demo warning loop=%lu", static_cast<unsigned long>(loop_count));
        }
        if ((loop_count % 11u) == 0u) {
            LOG_ERROR_TO(app_logger, "demo error loop=%lu",
                         static_cast<unsigned long>(loop_count));
        }

        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
        ++loop_count;
    }
}
