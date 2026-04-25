// ESP32-C3-DevKitM-1 bring-up example — direct boot, no second-stage bootloader.
//
// Expected UART output (115200-8-N-1 on GPIO21/TX):
//   [alloy] hello from ESP32-C3
//   [alloy] loop=1
//   [alloy] loop=2
//   ...
//
// GPIO8 (WS2812 data) toggles at ~1 Hz as a visual heartbeat.
// Connect: USB-serial adapter GPIO21(TX)->RXD, GND->GND.

#include "boards/esp32c3_devkitm/board.hpp"
#include "boards/esp32c3_devkitm/board_uart_raw.hpp"

#include <cstdint>

namespace {

inline void delay_cycles(std::uint32_t n) noexcept {
    std::uint32_t i = 0u;
    while (i < n) {
        asm volatile("nop");
        ++i;
    }
    asm volatile("" ::: "memory");  // prevent loop from being optimized away
}

// ~500 ms at 40 MHz ROM clock (no PLL configured yet)
inline void delay_500ms() noexcept {
    delay_cycles(20'000'000u);
}

}  // namespace

int main() {
    board::init();

    board::uart_raw::writeln("[alloy] hello from ESP32-C3");

    std::uint32_t loop = 0u;

    while (true) {
        board::led::on();
        delay_500ms();
        board::led::off();
        delay_500ms();

        ++loop;
        board::uart_raw::write("[alloy] loop=");
        board::uart_raw::write_uint32(loop);
        board::uart_raw::write_char('\r');
        board::uart_raw::write_char('\n');
    }
}
