#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "spi_probe requires BOARD_UART_HEADER for the selected board"
#endif

#ifndef BOARD_SPI_HEADER
    #error "spi_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

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

auto write_hex_frame(const decltype(board::make_debug_uart())& uart,
                     std::span<const std::uint8_t> frame) -> void {
    constexpr auto kHex = "0123456789ABCDEF";
    char line[3 * 4 + 3]{};
    auto write = std::size_t{0};

    for (const auto value : frame) {
        line[write++] = kHex[(value >> 4) & 0x0F];
        line[write++] = kHex[value & 0x0F];
        line[write++] = ' ';
    }
    line[write++] = '\r';
    line[write++] = '\n';
    line[write] = '\0';
    write_uart_text(uart, line);
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

    auto bus = board::make_spi();
    if (const auto result = bus.configure(); result.is_err()) {
        write_uart_text(uart, "spi configure failed\r\n");
        blink_error(200);
    }

    write_uart_text(uart, "spi probe ready\r\n");

    std::array<std::uint8_t, 4> tx{0x9Fu, 0x00u, 0x00u, 0x00u};
    std::array<std::uint8_t, 4> rx{};

    while (true) {
        const auto transfer_result = bus.transfer(tx, rx);
        if (transfer_result.is_err()) {
            write_uart_text(uart, "spi transfer failed\r\n");
        } else {
            write_uart_text(uart, "spi rx: ");
            write_hex_frame(uart, rx);
        }

        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
    }
}
