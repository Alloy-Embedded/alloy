#include BOARD_HEADER

#ifndef BOARD_DMA_HEADER
    #error "dma_probe requires BOARD_DMA_HEADER for the selected board"
#endif

#ifndef BOARD_UART_HEADER
    #error "dma_probe requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_DMA_HEADER
#include BOARD_UART_HEADER

#include <cstddef>
#include <cstdint>

#include "event.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"
#include "time.hpp"

namespace {

using BoardTime = alloy::time::source<board::BoardSysTick>;
using Duration = alloy::time::Duration;
using Deadline = alloy::time::Deadline;
using DebugUartTxCompletion =
    alloy::dma_event::token<board::DebugUartTxDma::peripheral_id, board::DebugUartTxDma::signal_id>;

constexpr auto kHeartbeatPeriod = Duration::from_millis(500);
constexpr auto kTransferPeriod = Duration::from_seconds(2);
constexpr auto kTransferTimeout = Duration::from_millis(50);
constexpr char kDmaPayload[] = "dma completion observed\r\n";

[[nodiscard]] auto dma_payload() -> std::span<const std::byte> {
    return {reinterpret_cast<const std::byte*>(kDmaPayload), sizeof(kDmaPayload) - 1u};
}

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

    auto next_tick = BoardTime::deadline_after(kHeartbeatPeriod);
    auto next_transfer = BoardTime::deadline_after(kTransferPeriod);
    std::uint32_t completion_count = 0u;

    while (true) {
        BoardTime::sleep_until(next_tick);
        next_tick = Deadline::at(next_tick.instant() + kHeartbeatPeriod);
        board::led::toggle();

        if (!BoardTime::expired(next_transfer)) {
            continue;
        }

        next_transfer = Deadline::at(next_transfer.instant() + kTransferPeriod);
        DebugUartTxCompletion::reset();

        if (const auto tx_result = uart.write_dma(tx_dma, dma_payload()); tx_result.is_err()) {
#ifdef BOARD_UART_HEADER
            if (uart_ready) {
                alloy::examples::uart_console::write_line(uart, "dma transfer start failed");
            }
#endif
            blink_error(100);
        }

        if (const auto completion = DebugUartTxCompletion::wait_for<BoardTime>(kTransferTimeout);
            completion.is_err()) {
#ifdef BOARD_UART_HEADER
            if (uart_ready) {
                alloy::examples::uart_console::write_line(uart, "dma completion timeout");
            }
#endif
            blink_error(150);
        }

        ++completion_count;

#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "dma completion count=");
            alloy::examples::uart_console::write_unsigned(uart, completion_count);
            alloy::examples::uart_console::write_text(uart, " uptime_ms=");
            alloy::examples::uart_console::write_unsigned(
                uart, static_cast<std::uint32_t>(BoardTime::uptime().as_millis()));
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
#endif
    }
}
