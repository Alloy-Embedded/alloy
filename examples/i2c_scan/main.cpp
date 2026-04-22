#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "i2c_scan requires BOARD_UART_HEADER for the selected board"
#endif

#ifndef BOARD_I2C_HEADER
    #error "i2c_scan requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"
namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

auto init_uart() -> decltype(board::make_debug_uart()) {
    auto uart = board::make_debug_uart();
    if (const auto result = uart.configure(); result.is_err()) {
        blink_error(100);
    }
    return uart;
}

}  // namespace

int main() {
    board::init();
    auto uart = init_uart();

    auto bus = board::make_i2c();
    if (const auto result = bus.configure(); result.is_err()) {
        alloy::examples::uart_console::write_line(uart, "i2c configure failed");
        blink_error(200);
    }

    alloy::examples::uart_console::write_line(uart, "i2c scan ready");

    std::array<std::uint8_t, 16> found{};
    while (true) {
        const auto scan_result = bus.scan_bus(found);
        if (scan_result.is_err()) {
            alloy::examples::uart_console::write_line(uart, "i2c scan failed");
            board::led::toggle();
            alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
            continue;
        }

        const auto count = scan_result.unwrap();
        if (count == 0u) {
            alloy::examples::uart_console::write_line(uart, "i2c scan none");
            board::led::toggle();
            alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(2000);
            continue;
        }

        alloy::examples::uart_console::write_line(uart, "i2c scan addresses:");
        for (std::size_t index = 0; index < count; ++index) {
            alloy::examples::uart_console::write_text(uart, "0x");
            alloy::examples::uart_console::write_hex_byte(uart, found[index]);
            alloy::examples::uart_console::write_text(uart, " ");
        }
        alloy::examples::uart_console::write_text(uart, "\r\n");

        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(2000);
    }
}
