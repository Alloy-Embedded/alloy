// The watchdog that also catches feeding too EARLY.
//
// An independent watchdog asks one question: is anybody still running? A loop
// that has lost its timebase and is spinning ten times too fast answers yes,
// enthusiastically, and the IWDG is satisfied. The window watchdog asks the
// harder question — is anybody running AT THE RIGHT RATE — by resetting the
// part on a feed that arrives before the window opens as readily as on one
// that never arrives.
//
// On the Nucleo-G0B1RE this prints:
//
//     alloy window_watchdog ready
//     this board declares a window watchdog
//     asked for  5000..20000 us
//     programmed 4608..20480 us     <- the counter cannot land on your numbers
//     block range 64..524288 us     <- and it cannot reach a second at all
//     10 healthy cycles, feeding mid-window...
//     .........
//     now going quiet — the dog should bite
//     LAST GASP
//     <reset; the banner comes round again>
//
// "LAST GASP" is the early-wakeup interrupt: it fires one counter tick before
// the reset, on a machine that is still running, which is the only moment a
// dying firmware can still say anything. It is the natural partner of
// alloy::fault (src/alloy/fault.hpp) — same rule for the handler (touch
// nothing that could itself fail), same reader (the next boot).
//
// WHAT THIS EXAMPLE DELIBERATELY DOES NOT DO is demonstrate the early bound by
// tripping it. A too-early feed resets instantly with no interrupt and no
// evidence, so the demo would be a device that reboots and never says why —
// which teaches nothing. The early bound is shown instead where it can be
// seen: in the programmed window printed above, and in the compile error you
// get for a window that cannot exist (try `.earliest = 30ms` below).
//
// On the seven boards with no WWDG this compiles and runs unchanged: the
// generated `board::window_watchdog` is a null stub, the `if constexpr` on the
// cap removes the reporting, and the loop is a plain heartbeat.
#include <alloy/board.hpp>
#include <alloy/delay.hpp>

#include <cstdint>

namespace {

alloy::delay timer{};

// The open port, reachable from the last-gasp handler. A uart handle is
// move-only and cannot be constructed a second time (that is the double-open
// guard doing its job), so the handler borrows the one main() holds — main
// never returns, so the pointee outlives every use.
using uart_handle = decltype(board::debug_uart::open());
const uart_handle* g_uart = nullptr;

void put_u32(auto& uart, std::uint32_t v) {
    char buf[10];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + (v % 10u));
        v /= 10u;
    } while (v != 0u && n < 10);
    while (n-- > 0) {
        uart.write(static_cast<std::uint8_t>(buf[n]));
    }
}

void put_range(auto& uart, const char* what, std::uint32_t lo, std::uint32_t hi) {
    uart.write(what);
    put_u32(uart, lo);
    uart.write("..");
    put_u32(uart, hi);
    uart.write(" us\r\n");
}

// The last-gasp handler. It runs one tick before the reset, so it does the
// least it can: one short blocking write on a port that is already open.
// Nothing here allocates, reprograms a clock or takes a lock.
void last_gasp() {
    if (g_uart != nullptr) {
        g_uart->write("LAST GASP\r\n");
    }
}

}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    g_uart = &uart;
    uart.write("alloy window_watchdog ready\r\n");

    // The window this program wants: a control step that must complete no
    // faster than 5 ms and no slower than 20 ms. Both bounds are real: the
    // fast one catches a loop whose timebase died, the slow one catches a loop
    // that stopped.
    constexpr alloy::wwdt::config wanted{.deadline = std::chrono::microseconds{20'000},
                                         .earliest = std::chrono::microseconds{5'000}};

    if constexpr (board::caps::window_watchdog) {
        uart.write("this board declares a window watchdog\r\n");
        board::window_watchdog.on_early_wakeup(&last_gasp);
        const auto got = board::window_watchdog.start(wanted);
        put_range(uart, "asked for  ", static_cast<std::uint32_t>(wanted.earliest.count()),
                  static_cast<std::uint32_t>(wanted.deadline.count()));
        put_range(uart, "programmed ", static_cast<std::uint32_t>(got.earliest.count()),
                  static_cast<std::uint32_t>(got.deadline.count()));
        put_range(uart, "block range ",
                  static_cast<std::uint32_t>(board::window_watchdog.shortest.count()),
                  static_cast<std::uint32_t>(board::window_watchdog.longest.count()));
    } else {
        uart.write("this board has no window watchdog — heartbeat only\r\n");
    }

    uart.write("10 healthy cycles, feeding mid-window...\r\n");
    for (int i = 0; i < 10; ++i) {
        timer.delay_ms(10);  // inside 5..20 ms: late enough, early enough
        board::window_watchdog.feed();
        uart.write(".");
    }
    uart.write("\r\nnow going quiet — the dog should bite\r\n");

    for (;;) {
        // No feed. On a board with the WWDG the early-wakeup handler speaks
        // and the part resets about a tick later; everywhere else this
        // simply idles.
        timer.delay_ms(200);
    }
}
