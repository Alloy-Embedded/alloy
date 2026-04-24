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

    // Manual scan so progress is visible per address (avoids looking hung).
    std::array<std::uint8_t, 0> empty_buf{};
    while (true) {
        alloy::examples::uart_console::write_line(uart, "i2c scan: probing 0x08..0x77");
        std::uint8_t found_count = 0u;
        for (std::uint8_t addr = 0x08; addr <= 0x77; ++addr) {
            alloy::examples::uart_console::write_text(uart, "  0x");
            alloy::examples::uart_console::write_hex_byte(uart, addr);
            const auto r = bus.write(addr, std::span<const std::uint8_t>{empty_buf});
            if (r.is_ok()) {
                alloy::examples::uart_console::write_line(uart, " ACK");
                ++found_count;
            } else {
                alloy::examples::uart_console::write_text(uart, " .\r\n");
            }
        }
        alloy::examples::uart_console::write_text(uart, "i2c scan done: ");
        alloy::examples::uart_console::write_hex_byte(uart, found_count);
        alloy::examples::uart_console::write_line(uart, " device(s)");

        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
    }
}
