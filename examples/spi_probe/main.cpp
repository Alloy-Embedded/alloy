#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_SPI_HEADER
    #error "spi_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_SPI_HEADER

#ifdef BOARD_UART_HEADER
    #include BOARD_UART_HEADER
#endif

#include "hal/systick.hpp"

namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

#ifdef BOARD_UART_HEADER
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
#endif

}  // namespace

int main() {
    board::init();

#ifdef BOARD_UART_HEADER
    auto uart = board::make_debug_uart();
    const auto uart_ready = uart.configure().is_ok();
#else
    constexpr auto uart_ready = false;
#endif

    auto bus = board::make_spi();
    if (const auto result = bus.configure(); result.is_err()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            write_uart_text(uart, "spi configure failed\r\n");
        }
#endif
        blink_error(200);
    }

#ifdef BOARD_UART_HEADER
    if (uart_ready) {
        write_uart_text(uart, "spi probe ready\r\n");
    }
#endif

    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
