#include BOARD_HEADER

#ifndef BOARD_DMA_HEADER
    #error "dma_probe requires BOARD_DMA_HEADER for the selected board"
#endif

#ifndef BOARD_UART_HEADER
    #error "dma_probe requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_DMA_HEADER
#include BOARD_UART_HEADER

#include <cstdint>

#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

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
    const auto uart_ready = uart.configure().is_ok();
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "dma probe ready");
    }

    [[maybe_unused]] auto tx_dma = board::make_debug_uart_tx_dma({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
        .mode = alloy::hal::dma::Mode::normal,
        .priority = alloy::hal::dma::Priority::medium,
        .data_width = alloy::hal::dma::DataWidth::bits8,
    });
    [[maybe_unused]] auto rx_dma = board::make_debug_uart_rx_dma({
        .direction = alloy::hal::dma::Direction::peripheral_to_memory,
        .mode = alloy::hal::dma::Mode::normal,
        .priority = alloy::hal::dma::Priority::medium,
        .data_width = alloy::hal::dma::DataWidth::bits8,
    });

    if (const auto result = uart.configure_tx_dma(tx_dma); result.is_err()) {
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "dma tx bind failed");
        }
        blink_error(100);
    }
    if (const auto result = uart.configure_rx_dma(rx_dma); result.is_err()) {
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "dma rx bind failed");
        }
        blink_error(150);
    }

    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "dma bindings configured");
    }

    std::uint32_t loop_count = 0u;
    while (true) {
        board::led::toggle();
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "dma loop=");
            alloy::examples::uart_console::write_unsigned(uart, loop_count);
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
#endif
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500);
        ++loop_count;
    }
}
