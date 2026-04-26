// ESP32-DevKit bring-up example — ESP-IDF bootloader, IRAM-only app.
//
// Expected UART output on GPIO1/TX at 115200-8-N-1:
//   [alloy] hello from ESP32
//   [alloy] loop=1
//   [alloy] loop=2
//   ...
//
// GPIO2 (blue LED) blinks at ~1 Hz.
//
// Flash (after build):
//   esptool.py --chip esp32 --port /dev/cu.usbserial-* elf2image hello_esp32
//   esptool.py --chip esp32 --port /dev/cu.usbserial-* \
//       write_flash 0x10000 hello_esp32.bin

#include "boards/esp32_devkit/board.hpp"
#include "boards/esp32_devkit/board_uart_raw.hpp"

#include <cstdint>

namespace {

// ~500 ms at 80 MHz APB clock (bootloader default).
// Each loop iteration ≈ 4 cycles (Xtensa pipeline).
inline void delay_500ms() noexcept {
    for (std::uint32_t i = 0u; i < 10'000'000u; ++i) {
        asm volatile("nop");
    }
}

}  // namespace

int main() {
    board::init();

    board::uart_raw::writeln("[alloy] hello from ESP32");

    std::uint32_t loop = 0u;

    while (true) {
        board::uart_raw::writeln("[alloy] led on");
        board::led::on();
        board::uart_raw::writeln("[alloy] delay1");
        delay_500ms();
        board::uart_raw::writeln("[alloy] led off");
        board::led::off();
        board::uart_raw::writeln("[alloy] delay2");
        delay_500ms();

        ++loop;
        board::uart_raw::write("[alloy] loop=");
        board::uart_raw::write_uint32(loop);
        board::uart_raw::write_char('\r');
        board::uart_raw::write_char('\n');
    }
}
