// examples/modbus_master_poll/main.cpp
//
// Modbus RTU master — polling 3 variables from a remote slave.
//
// Wiring: connect this board's UART TX/RX to the slave board (or RS-485 bus)
// via a loopback wire or two-board back-to-back cable.
//
// Polled variables (must match the slave's registry):
//   temperature (float,  addr 0x00, slave 0x01) — every 500 ms
//   status_word (uint16, addr 0x04, slave 0x01) — every 200 ms
//   position    (int32,  addr 0x08, slave 0x01) — every 1 s
//
// Mirror values are updated automatically by the master scheduler. The main
// loop reads them for display / control without locking (single-threaded).
//
// Stale callback: fired when the slave doesn't respond within the timeout.
// Hook it up to an alarm output or watchdog to handle communication faults.
//
// Note: requires alloy/modbus/transport/uart_stream.hpp (task 4.2).

#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "modbus_master_poll requires BOARD_UART_HEADER for the selected board"
#endif
#include BOARD_UART_HEADER

#include "alloy/modbus/master.hpp"
#include "alloy/modbus/transport/uart_stream.hpp"
#include "alloy/modbus/var.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace {

using namespace alloy::modbus;
using namespace alloy::examples::uart_console;

// ============================================================================
// Variable descriptors (must match the slave's declarations)
// ============================================================================

constexpr Var<float>         kTemp    {.address=0x00u, .access=Access::ReadOnly,  .name="temperature"};
constexpr Var<std::uint16_t> kStatus  {.address=0x04u, .access=Access::ReadOnly,  .name="status_word"};
constexpr Var<std::int32_t>  kPosition{.address=0x08u, .access=Access::ReadOnly,  .name="position"};

// ============================================================================
// Mirror storage (updated by master on each successful poll)
// ============================================================================

float         g_temp_mirror{0.0f};
std::uint16_t g_status_mirror{0u};
std::int32_t  g_position_mirror{0};

// ============================================================================
// Stale-data handler
// ============================================================================

void on_stale(std::uint16_t reg_addr, std::uint8_t slave_id) noexcept {
    // Called when a poll times out. In production: trigger alarm / watchdog.
    (void)reg_addr;
    (void)slave_id;
    board::led::toggle();
}

// ============================================================================
// Monotonic microsecond clock (board-provided SysTick)
// ============================================================================

std::uint64_t now_us() noexcept {
    return static_cast<std::uint64_t>(
        alloy::hal::SysTickTimer::now_us<board::BoardSysTick>());
}

}  // namespace

int main() {
    board::init();

    auto console_uart = board::make_debug_uart();
    if (console_uart.configure().is_err()) {
        while (true) { board::led::toggle(); }
    }
    write_line(console_uart, "modbus master: starting");

    // RS-485 UART used for the Modbus bus (may be the same port if half-duplex).
    auto bus_uart = board::make_modbus_uart();
    if (bus_uart.configure().is_err()) {
        write_line(console_uart, "modbus master: bus uart configure failed");
        while (true) { board::led::toggle(); }
    }
    alloy::modbus::UartStream stream{bus_uart};

    // Master: up to 3 polls, 1 ms response timeout per request.
    Master<alloy::modbus::UartStream, 3u, decltype(&now_us)>
        master{stream, &now_us};

    master.set_stale_callback(on_stale);

    // Register the three variables for periodic polling.
    (void)master.add_poll(kTemp,     g_temp_mirror,     0x01u, 500'000u);  // 500 ms
    (void)master.add_poll(kStatus,   g_status_mirror,   0x01u, 200'000u);  // 200 ms
    (void)master.add_poll(kPosition, g_position_mirror, 0x01u, 1'000'000u);// 1 s

    write_line(console_uart, "modbus master: polling");

    while (true) {
        // Drive the scheduler — sends one FC03 request if anything is due,
        // then waits up to 1 ms for the response.
        (void)master.poll_once(1'000u);

        // Use the mirror values (updated by the master on success).
        // In a real application: feed into a control loop, log to flash, etc.
        (void)g_temp_mirror;
        (void)g_status_mirror;
        (void)g_position_mirror;
    }
}
