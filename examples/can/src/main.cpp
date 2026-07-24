// CAN demo with a built-in self-check: the controller comes up in INTERNAL
// LOOPBACK, so every frame we transmit is delivered straight back into the RX
// FIFO with no transceiver and no bus wiring. We send a known frame, receive it,
// and print both — matching id/data is proof the whole M_CAN path (bit timing,
// TX buffer, message RAM, RX FIFO) works. Zero #ifdefs: board::can is a no-op
// stub where absent, guarded by `if constexpr (board::caps::can)`.
#include <alloy/board.hpp>

#include <cstdint>

using namespace alloy::literals;

namespace {
template <class Uart>
void print_hex(const Uart& uart, std::uint32_t v, unsigned nibbles) {
    for (int i = static_cast<int>(nibbles) - 1; i >= 0; --i) {
        const std::uint32_t d = (v >> (4u * static_cast<unsigned>(i))) & 0xFu;
        uart.write(static_cast<std::uint8_t>(d < 10u ? '0' + d : 'a' + (d - 10u)));
    }
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy can demo (internal loopback)\r\n");

    if constexpr (board::caps::can) {
        board::can.enable();
        std::uint8_t counter = 0;
        for (;;) {
            alloy::can_frame tx{};
            tx.id = 0x123;
            tx.len = 4;
            tx.data[0] = 0xDE;
            tx.data[1] = 0xAD;
            tx.data[2] = 0xBE;
            tx.data[3] = counter++;
            (void)board::can.send(tx);

            alloy::can_frame rx{};
            bool got = false;
            for (std::uint32_t i = 0; i < 200000u && !got; ++i) {
                got = board::can.receive(rx);
            }
            if (got) {
                uart.write("rx id=0x");
                print_hex(uart, rx.id, 3);
                uart.write(" len=");
                uart.write(static_cast<std::uint8_t>('0' + (rx.len & 0xFu)));
                uart.write(" data=");
                for (std::uint8_t i = 0; i < rx.len && i < 8u; ++i) {
                    print_hex(uart, rx.data[i], 2);
                    uart.write(static_cast<std::uint8_t>(' '));
                }
                uart.write("\r\n");
            } else {
                uart.write("no frame looped back\r\n");
            }
            alloy::sleep_for(1s);
        }
    } else {
        uart.write("this board declares no can role\r\n");
        for (;;) {
            alloy::sleep_for(1s);
        }
    }
}
