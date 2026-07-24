// GPIO bus demo with a self-check: drive three same-port pins as one 3-bit bus
// and read them straight back. A push-pull output's level is reflected in the
// input register, so write(v) followed by read() returns v with no external
// wiring — proof the atomic multi-pin store lands on exactly the right pins.
// Zero #ifdefs: board::gpio_bus is a no-op stub where absent, guarded by
// `if constexpr (board::caps::gpio_bus)`.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
template <class Uart>
void print_bits(const Uart& uart, std::uint32_t v, unsigned n) {
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        uart.write(static_cast<std::uint8_t>('0' + ((v >> static_cast<unsigned>(i)) & 1u)));
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy gpio_bus demo\r\n");

    if constexpr (board::caps::gpio_bus) {
        board::gpio_bus.init();
        uart.write("3-bit bus: write a value -> read it back from the pins\r\n");
        for (;;) {
            for (std::uint32_t v = 0; v < 8u; ++v) {
                board::gpio_bus.write(v);
                // The input register lags a just-driven output by a read cycle,
                // so the first read-back is stale — discard it, then sample.
                (void)board::gpio_bus.read();
                const std::uint32_t rb = board::gpio_bus.read();
                uart.write("wrote ");
                print_bits(uart, v, 3);
                uart.write(" read ");
                print_bits(uart, rb, 3);
                uart.write(v == rb ? " ok\r\n" : " MISMATCH\r\n");
                alloy::sleep_for(500ms);
            }
        }
    } else {
        uart.write("this board declares no gpio_bus role\r\n");
        for (;;) {
            alloy::sleep_for(1s);
        }
    }
}
