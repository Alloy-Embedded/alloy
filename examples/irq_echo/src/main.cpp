// Interrupt-driven echo: the UART echo runs ENTIRELY inside the RX
// interrupt callback while the foreground loop only blinks the LED — the
// LED proves main() never blocks on the UART. Zero #ifdefs.
//
// The generic-lambda wrapper keeps the on_receive call dependent, so
// boards whose UART driver has no interrupt path yet (ESP32 v1) still
// compile and honestly report it.
#include <alloy/board.hpp>

#include <cstdint>
#include <type_traits>

using namespace alloy::literals;

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("alloy irq_echo — eco na ISR, LED no laço\r\n");

    bool armed = false;
    if constexpr (board::caps::irq && board::caps::debug_uart) {
        [&armed](auto& u) {
            if constexpr (requires { u.on_receive(nullptr, nullptr); }) {
                using Uart = std::remove_reference_t<decltype(u)>;
                u.on_receive(
                    +[](void* ctx, std::uint8_t byte) {
                        static_cast<const Uart*>(ctx)->write(byte);
                    },
                    const_cast<void*>(static_cast<const void*>(&u)));
                armed = true;
            }
        }(uart);
    }
    uart.write(armed ? "(eco armado por interrupcao)\r\n"
                     : "(sem camada de interrupcao nesta board)\r\n");

    while (true) {
        if constexpr (board::caps::led) {
            board::led.toggle();
        }
        alloy::sleep_for(500ms);
    }
}
