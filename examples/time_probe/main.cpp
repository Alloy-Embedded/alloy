#include BOARD_HEADER

#ifdef BOARD_UART_HEADER
    #include BOARD_UART_HEADER
#endif

#include <cstdint>

#include "examples/common/uart_console.hpp"
#include "time.hpp"

namespace {

using BoardTime = alloy::time::source<board::BoardSysTick>;
using Duration = alloy::time::Duration;
using Deadline = alloy::time::Deadline;

constexpr auto kHeartbeatPeriod = Duration::from_millis(500);
constexpr auto kReportWindow = Duration::from_seconds(2);

}  // namespace

int main() {
    board::init();

#ifdef BOARD_UART_HEADER
    auto uart = board::make_debug_uart();
    const auto uart_ready = uart.configure().is_ok();
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "time probe ready");
    }
#else
    constexpr auto uart_ready = false;
#endif

    auto next_tick = BoardTime::deadline_after(kHeartbeatPeriod);
    auto report_deadline = BoardTime::deadline_after(kReportWindow);
    std::uint32_t heartbeat_count = 0u;
    std::uint32_t timeout_count = 0u;

    while (true) {
        BoardTime::sleep_until(next_tick);
        next_tick = Deadline::at(next_tick.instant() + kHeartbeatPeriod);
        board::led::toggle();
        ++heartbeat_count;

        if (!BoardTime::expired(report_deadline)) {
            continue;
        }

        report_deadline = Deadline::at(report_deadline.instant() + kReportWindow);
        ++timeout_count;

#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "timeout window=");
            alloy::examples::uart_console::write_unsigned(uart, timeout_count);
            alloy::examples::uart_console::write_text(uart, " heartbeat_count=");
            alloy::examples::uart_console::write_unsigned(
                uart, heartbeat_count);
            alloy::examples::uart_console::write_text(uart, " uptime_ms=");
            alloy::examples::uart_console::write_unsigned(
                uart, static_cast<std::uint32_t>(BoardTime::uptime().as_millis()));
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
#endif
    }
}
