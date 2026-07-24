// Watchdog demo: arm the hardware watchdog, feed it a few times, then stop —
// letting it bite so the MCU resets and the banner reprints. That visible
// reboot loop is the proof the watchdog is real. Zero #ifdefs: board::watchdog
// exists on every board (a no-op stub where absent), and the demo path is
// guarded by `if constexpr (board::caps::watchdog)`.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy watchdog demo\r\n");

    if constexpr (board::caps::watchdog) {
        board::watchdog.start(4s);  // reset the MCU if not fed within 4 s
        uart.write("armed: 4s timeout, feeding 6x then starving it\r\n");
        for (int i = 0; i < 6; ++i) {
            board::watchdog.feed();
            uart.write("fed\r\n");
            alloy::sleep_for(1s);
        }
        uart.write("stop feeding -> reset in ~4s (watch this banner reappear)\r\n");
    } else {
        uart.write("this board declares no watchdog role\r\n");
    }

    // On a watchdog board we never feed again here, so the dog bites and the
    // MCU reboots; on others this is just an idle loop.
    while (true) {
        alloy::sleep_for(1s);
    }
}
