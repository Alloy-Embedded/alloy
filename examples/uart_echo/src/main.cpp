// Portable UART echo — identical bytes on every board with a debug_uart role.
//
// board::debug_uart is bound to the board's declared pins with compile-time
// route checking; on a board without the role it degrades to a no-op stub,
// still with zero #ifdefs here.
#include <alloy/board.hpp>

#include <cstdint>

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy uart_echo ready\r\n");
    while (true) {
        std::uint8_t byte{};
        if (uart.read(byte)) {
            uart.write(byte);
        }
    }
}
