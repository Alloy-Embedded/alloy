// Portable hello — blink AND echo at once, so a freshly flashed board never
// looks dead: the LED proves the clock/timebase, the echo proves the UART.
// Identical bytes on every supported board; zero #ifdefs.
#include <alloy/board.hpp>

#include <cstdint>

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy hello: blinking + echoing\r\n");

    std::uint32_t last_toggle = alloy::uptime_ms();
    while (true) {
        std::uint8_t byte{};
        if (uart.read(byte)) {
            uart.write(byte);
        }
        // Unsigned delta keeps working across timer wrap.
        if (alloy::uptime_ms() - last_toggle >= 500u) {
            board::led.toggle();
            last_toggle = alloy::uptime_ms();
        }
    }
}
