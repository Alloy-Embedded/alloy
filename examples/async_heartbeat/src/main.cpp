// Portable async heartbeat — the emulation oracle for the coroutine runtime.
//
// A single coroutine prints an incrementing "beat N" line every 250 ms over the
// board's debug UART. Under Renode (headless CI) the test waits for the banner,
// then for several consecutive beats — so a green run proves the executor
// actually SCHEDULES and `co_await delay()` RESUMES repeatedly on the real core
// over virtual time, not merely that the firmware compiled and booted once.
//
// This is the runtime counterpart to async_blink (which toggles a GPIO the
// minimal CI platform doesn't model): same portable coroutine runtime, but its
// progress is observable on the one peripheral the emulator does model — the
// debug UART. No #ifdef, no heap; retarget by changing the id in alloy.toml.
#include <cstdint>

#include <alloy/async/delay.hpp>
#include <alloy/async/executor.hpp>
#include <alloy/async/task.hpp>
#include <alloy/board.hpp>

using namespace alloy::async;
using namespace alloy::literals;

namespace {

executor<4> sched;
task_storage<256> beat_frame;

// Emit an unsigned value as decimal digits — no heap, no printf. Kept as a plain
// function so it lives on the ordinary call stack, not in the coroutine frame.
template <class Uart>
void write_u32(Uart& uart, std::uint32_t v) {
    char buf[10];
    int i = static_cast<int>(sizeof buf);
    do {
        buf[--i] = static_cast<char>('0' + (v % 10));
        v /= 10;
    } while (v != 0);
    while (i < static_cast<int>(sizeof buf)) {
        uart.write(static_cast<std::uint8_t>(buf[i++]));
    }
}

task heartbeat(task_storage<256>&) {
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy async_heartbeat ready\r\n");
    for (std::uint32_t n = 1;; ++n) {
        co_await delay(250ms);
        uart.write("beat ");
        write_u32(uart, n);
        uart.write("\r\n");
    }
}

}  // namespace

int main() {
    board::init();
    sched.spawn(heartbeat(beat_frame));
    sched.run();  // cooperative superloop; never returns
}
