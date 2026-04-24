// Canonical completion+timeout example for the runtime async model.
//
// Demonstrates three distinguishable outcomes on the same UART+DMA path:
//   1. success  — completion fires before a generous deadline
//   2. timeout  — completion is waited on with a zero deadline and reports Timeout
//   3. recovery — after a timeout, waiting again with a generous deadline completes cleanly
//
// The example never hard-faults on timeout: it treats Timeout as a first-class
// recoverable outcome, counts each outcome, and reports running totals over the
// board's debug UART so the user can observe that async completion+timeout is a
// usable, bounded primitive.

#include BOARD_HEADER

#ifndef BOARD_DMA_HEADER
    #error "async_uart_timeout requires BOARD_DMA_HEADER for the selected board"
#endif
#ifndef BOARD_UART_HEADER
    #error "async_uart_timeout requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_DMA_HEADER
#include BOARD_UART_HEADER

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/error_code.hpp"
#include "event.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"
#include "time.hpp"

namespace {

using BoardTime = alloy::time::source<board::BoardSysTick>;
using Duration = alloy::time::Duration;
using Deadline = alloy::time::Deadline;
using TxCompletion =
    alloy::dma_event::token<board::DebugUartTxDma::peripheral_id,
                            board::DebugUartTxDma::signal_id>;

constexpr auto kHeartbeatPeriod = Duration::from_millis(500);
constexpr auto kSuccessPeriod = Duration::from_seconds(2);
constexpr auto kTimeoutPeriod = Duration::from_seconds(5);
constexpr auto kGenerousDeadline = Duration::from_millis(50);
constexpr auto kZeroDeadline = Duration::from_millis(0);
constexpr auto kRecoveryDeadline = Duration::from_millis(50);
constexpr char kPayload[] = "async uart timeout payload\r\n";

[[nodiscard]] auto payload_span() -> std::span<const std::byte> {
    return {reinterpret_cast<const std::byte*>(kPayload), sizeof(kPayload) - 1u};
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
        alloy::examples::uart_console::write_line(uart, "async uart timeout ready");
    }

    auto tx_dma = board::make_debug_uart_tx_dma({
        .direction = alloy::hal::dma::Direction::memory_to_peripheral,
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

    auto next_tick = BoardTime::deadline_after(kHeartbeatPeriod);
    auto next_success = BoardTime::deadline_after(kSuccessPeriod);
    auto next_timeout_drill = BoardTime::deadline_after(kTimeoutPeriod);

    std::uint32_t success_count = 0u;
    std::uint32_t timeout_count = 0u;
    std::uint32_t recovered_count = 0u;

    while (true) {
        BoardTime::sleep_until(next_tick);
        next_tick = Deadline::at(next_tick.instant() + kHeartbeatPeriod);
        board::led::toggle();

        // Outcome 1: success — generous deadline.
        if (BoardTime::expired(next_success)) {
            next_success = Deadline::at(next_success.instant() + kSuccessPeriod);
            TxCompletion::reset();
            if (uart.write_dma(tx_dma, payload_span()).is_err()) {
                if (uart_ready) {
                    alloy::examples::uart_console::write_line(uart,
                                                              "async tx kickoff failed");
                }
                blink_error(100);
            }
            const auto result = TxCompletion::wait_for<BoardTime>(kGenerousDeadline);
            if (result.is_ok()) {
                ++success_count;
                if (uart_ready) {
                    alloy::examples::uart_console::write_text(uart, "async success=");
                    alloy::examples::uart_console::write_unsigned(uart, success_count);
                    alloy::examples::uart_console::write_text(uart, "\r\n");
                }
            } else if (uart_ready) {
                alloy::examples::uart_console::write_line(uart,
                                                          "async unexpected non-ok on success path");
            }
        }

        // Outcome 2+3: timeout, then recovery on the same waiting path.
        if (BoardTime::expired(next_timeout_drill)) {
            next_timeout_drill = Deadline::at(next_timeout_drill.instant() + kTimeoutPeriod);
            TxCompletion::reset();
            if (uart.write_dma(tx_dma, payload_span()).is_err()) {
                if (uart_ready) {
                    alloy::examples::uart_console::write_line(uart,
                                                              "async tx kickoff failed (drill)");
                }
                blink_error(100);
            }

            // Deliberately wait with a zero deadline — the transfer has had no time
            // to complete. wait_for must return Timeout, not Ok.
            const auto timeout_result = TxCompletion::wait_for<BoardTime>(kZeroDeadline);
            const bool got_timeout =
                timeout_result.is_err() &&
                timeout_result.error() == alloy::core::ErrorCode::Timeout;
            if (got_timeout) {
                ++timeout_count;
                if (uart_ready) {
                    alloy::examples::uart_console::write_text(uart, "async timeout=");
                    alloy::examples::uart_console::write_unsigned(uart, timeout_count);
                    alloy::examples::uart_console::write_text(uart, "\r\n");
                }
            } else if (uart_ready) {
                alloy::examples::uart_console::write_line(uart,
                                                          "async drill: zero deadline did not report timeout");
            }

            // Recovery: same token, same in-flight transfer — wait again with a
            // generous deadline. The canonical path must keep working after a
            // timeout; completion still fires and is still observable.
            const auto recovery_result =
                TxCompletion::wait_for<BoardTime>(kRecoveryDeadline);
            if (recovery_result.is_ok()) {
                ++recovered_count;
                if (uart_ready) {
                    alloy::examples::uart_console::write_text(uart, "async recovered=");
                    alloy::examples::uart_console::write_unsigned(uart, recovered_count);
                    alloy::examples::uart_console::write_text(uart, "\r\n");
                }
            } else if (uart_ready) {
                alloy::examples::uart_console::write_line(uart,
                                                          "async recovery wait did not complete");
            }
        }

        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "async loop success=");
            alloy::examples::uart_console::write_unsigned(uart, success_count);
            alloy::examples::uart_console::write_text(uart, " timeout=");
            alloy::examples::uart_console::write_unsigned(uart, timeout_count);
            alloy::examples::uart_console::write_text(uart, " recovered=");
            alloy::examples::uart_console::write_unsigned(uart, recovered_count);
            alloy::examples::uart_console::write_text(uart, " uptime_ms=");
            alloy::examples::uart_console::write_unsigned(
                uart, static_cast<std::uint32_t>(BoardTime::uptime().as_millis()));
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
    }
}
