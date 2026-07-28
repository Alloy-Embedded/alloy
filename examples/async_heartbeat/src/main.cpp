// Portable async heartbeat — the emulation oracle for the coroutine runtime.
//
// TWO independent coroutines share one core through alloy's executor: `heartbeat`
// prints "beat N" every 250 ms, `ticker` prints "tick" every 400 ms — different
// cadences, no shared state, straight-line code. Under Renode (headless CI) the
// test waits for the banner, three successive beats, then a tick, so a green run
// proves two things about the runtime running on the real ISA, not just that it
// compiled:
//   1. the executor + timer wheel RESUME a task repeatedly over virtual time
//      (the three beats, each gated behind a `co_await delay(250ms)`), and
//   2. two schedules run CONCURRENTLY on one stack with no preemption and no
//      heap (the tick keeps arriving while the beats advance) — the thing a
//      plain superloop with a single sleep_for() can't express.
//
// This is the runtime counterpart to async_blink (which toggles a GPIO the
// minimal CI platform doesn't model); progress is observable on the one
// peripheral the emulator does model — the debug UART. No #ifdef, no heap;
// retarget by changing the id in alloy.toml.
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
task_storage<256> tick_frame;

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

// Both tasks share ONE UART opened in main(): opening the same peripheral twice
// is a runtime trap (the single-owner guard), so the handle is passed in by
// reference. It outlives the coroutines because run() never returns.
template <class Uart>
task heartbeat(task_storage<256>&, Uart& uart) {
    uart.write("alloy async_heartbeat ready\r\n");
    for (std::uint32_t n = 1;; ++n) {
        co_await delay(250ms);
        uart.write("beat ");
        write_u32(uart, n);
        uart.write("\r\n");
    }
}

template <class Uart>
task ticker(task_storage<256>&, Uart& uart) {
    for (;;) {
        co_await delay(400ms);
        uart.write("tick\r\n");
    }
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    sched.spawn(heartbeat(beat_frame, uart));
    sched.spawn(ticker(tick_frame, uart));
    sched.run();  // cooperative superloop; never returns
}
