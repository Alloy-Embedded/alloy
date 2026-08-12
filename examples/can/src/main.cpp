// CAN demo with a built-in self-check: the controller comes up in INTERNAL
// LOOPBACK, so every frame we transmit is delivered straight back into the RX
// FIFO with no transceiver and no bus wiring. We send a known frame, receive it,
// and print both — matching id/data is proof the whole M_CAN path (bit timing,
// TX buffer, message RAM, RX FIFO) works. Zero #ifdefs: board::can is a no-op
// stub where absent, guarded by `if constexpr (board::caps::can)`.
//
// The second half is the ACCEPTANCE FILTER self-check, and loopback is what
// makes it observable without a bus: a frame the controller transmits is
// acceptance-filtered on the way back in, exactly as a frame off the wire
// would be. Three ids are sent every round; only two of them should return.
//
// Note where the filters actually live. `accept_only` writes the match
// elements into a SECOND peripheral — the FDCAN's companion message RAM — and
// then publishes the list size in the controller. One line of user code, two
// blocks of silicon, and the user names neither.
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

// Send one frame and report whether it came back. Under the accept-all default
// every id returns; under a filter list only the matching ones do.
template <class Uart, class Can>
void probe_id(const Uart& uart, const Can& can, std::uint32_t id, std::uint8_t tag) {
    alloy::can_frame tx{};
    tx.id = id;
    tx.len = 1;
    tx.data[0] = tag;
    (void)can.send(tx);

    alloy::can_frame rx{};
    bool got = false;
    for (std::uint32_t i = 0; i < 200000u && !got; ++i) {
        got = can.receive(rx);
    }
    uart.write("  id=0x");
    print_hex(uart, id, 3);
    if (got) {
        uart.write(" ACCEPTED rx=0x");
        print_hex(uart, rx.id, 3);
        uart.write(" data=");
        print_hex(uart, rx.data[0], 2);
    } else {
        uart.write(" dropped");
    }
    uart.write("\r\n");
}
}  // namespace

int main() {
    board::init();
    auto uart = board::debug_uart::open({.baud = board::debug_uart_baud});
    uart.write("\r\nalloy can demo (internal loopback)\r\n");

    if constexpr (board::caps::can) {
        board::can.enable();

        uart.write("accept all:\r\n");
        probe_id(uart, board::can, 0x123, 0x11);
        probe_id(uart, board::can, 0x205, 0x22);
        probe_id(uart, board::can, 0x321, 0x33);

        // One exact id, and one 16-wide range (0x200..0x20F). Everything else
        // is dropped by the controller before it reaches the FIFO.
        board::can.accept_only(alloy::can::match(0x123),
                               alloy::can::match_masked(0x200, 0x7F0));

        uart.write("accept only 0x123 and 0x200/0x7f0:\r\n");
        probe_id(uart, board::can, 0x123, 0x11);
        probe_id(uart, board::can, 0x205, 0x22);
        probe_id(uart, board::can, 0x321, 0x33);

        uart.write("can filter demo done\r\n");
        for (;;) {
            alloy::sleep_for(1s);
        }
    } else {
        uart.write("this board declares no can role\r\n");
        for (;;) {
            alloy::sleep_for(1s);
        }
    }
}
