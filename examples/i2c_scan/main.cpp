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

#include "hal/systick.hpp"

namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

auto write_uart_text(const decltype(board::make_debug_uart())& uart, const char* text) -> void {
    auto length = std::size_t{0};
    while (text[length] != '\0') {
        ++length;
    }

    const auto bytes =
        std::span{reinterpret_cast<const std::byte*>(text), static_cast<std::size_t>(length)};
    static_cast<void>(uart.write(bytes));
    static_cast<void>(uart.flush());
}

auto write_hex_byte(const decltype(board::make_debug_uart())& uart, std::uint8_t value) -> void {
    constexpr auto kHex = "0123456789ABCDEF";
    char buffer[] = {'0', 'x', kHex[(value >> 4) & 0x0F], kHex[value & 0x0F], '\r', '\n', '\0'};
    write_uart_text(uart, buffer);
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
        write_uart_text(uart, "i2c configure failed\r\n");
        blink_error(200);
    }

    write_uart_text(uart, "i2c scan ready\r\n");

    std::array<std::uint8_t, 16> found{};
    while (true) {
        const auto scan_result = bus.scan_bus(found);
        if (scan_result.is_err()) {
            write_uart_text(uart, "i2c scan failed\r\n");
            board::led::toggle();
            alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
            continue;
        }

        const auto count = scan_result.unwrap();
        write_uart_text(uart, "i2c scan addresses:\r\n");
        for (std::size_t index = 0; index < count; ++index) {
            write_hex_byte(uart, found[index]);
        }

        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(2000);
    }
}
