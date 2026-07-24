// RTC demo: set the wall clock once, then watch it tick. The clock lives in the
// backup domain and keeps running on the LSI even while the core is reset, so
// on the first boot we set it and on later boots the time has ADVANCED past
// where we left off — the proof it kept time across the reset. Zero #ifdefs:
// board::rtc exists on every board (a no-op stub where absent), guarded by
// `if constexpr (board::caps::rtc)`.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
template <class Uart>
void print2(const Uart& uart, std::uint8_t v) {  // zero-padded 2 digits
    uart.write(static_cast<std::uint8_t>('0' + (v / 10u) % 10u));
    uart.write(static_cast<std::uint8_t>('0' + v % 10u));
}

template <class Uart>
void print_time(const Uart& uart, const alloy::datetime& t) {
    print2(uart, t.hour);
    uart.write(static_cast<std::uint8_t>(':'));
    print2(uart, t.minute);
    uart.write(static_cast<std::uint8_t>(':'));
    print2(uart, t.second);
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy rtc demo\r\n");

    if constexpr (board::caps::rtc) {
        if (!board::rtc.initialized()) {
            board::rtc.set({.hour = 12, .minute = 0, .second = 0,
                            .day = 24, .month = 7, .year = 26});
            uart.write("clock was unset -> set to 12:00:00\r\n");
        } else {
            uart.write("clock already running -> it kept time across the reset\r\n");
        }
        for (;;) {
            uart.write("time ");
            print_time(uart, board::rtc.now());
            uart.write("\r\n");
            alloy::sleep_for(1s);
        }
    } else {
        uart.write("this board declares no rtc role\r\n");
        for (;;) {
            alloy::sleep_for(1s);
        }
    }
}
