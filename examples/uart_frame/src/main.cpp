// The three layers of the peripheral surface, in one portable file.
//
// Same source on every board alloy ships — six UART drivers across four
// vendors — with zero preprocessor. Each layer answers a different question
// and the NAME says which one you are using:
//
//   alloy::uart::config      LAYER 1  portable, runtime, honoured everywhere
//   Role::opts               LAYER 2  vendor knobs, shaped by the IP
//   Role::inst::feat::*      DEGREE   a generated number; 0 means absent
//
// See docs/reference/peripheral-surface.md. Build it for another board and
// nothing here changes:
//
//     alloy build --board nucleo_f767zi     # usart_v3: no FIFO
//     alloy build --board esp32_devkit      # empty opts, ROM-configured port
#include <alloy/board.hpp>

#include <cstdint>

namespace {

// ── LAYER 2 ─────────────────────────────────────────────────────────────
//
// `opts` is keyed by the peripheral INSTANCE, so its members are whatever
// that IP's driver can actually program. Five of alloy's six UART drivers
// have `data_bits`; the ESP32's ROM-configured port has an empty `opts`, so
// naming the member unconditionally would not compile there.
//
// That is the portable idiom for Layer 2, and it needs no preprocessor: probe
// the member by NAME. A knob the silicon lacks is not a member, so `requires`
// is the whole capability test — there is no separate table of booleans that
// could disagree with it.
// TWO GOTCHAS, both measured on arm-none-eabi-g++ 14.2.1, and both of the
// same kind: C++ only defers a check when the thing being checked is
// DEPENDENT on a template parameter.
//
//  1. A requires-expression written inline against a concrete type —
//     `requires { board::debug_uart::opts{}.data_bits; }` — is not a
//     question, it is an error: "has no member named 'data_bits'". Name the
//     probe as a concept over a template parameter instead.
//  2. `if constexpr` only discards WITHOUT CHECKING inside a template. In a
//     plain function or lambda both branches are still parsed and looked up,
//     so `o.data_bits = 8` fails on the ESP32 even when the branch is dead.
//     Hence make_frame<O>() is a template.
//
// Neither is specific to alloy, and neither is in the design doc's sketch of
// this idiom. They are the two lines of scaffolding Layer 2 costs portable
// code, every time.
template <class O>
concept has_data_bits = requires(O o) { o.data_bits; };
template <class O>
concept has_de_timing = requires(O o) { o.de_assert_16ths; };

template <class O>
constexpr O make_frame() {
    O o{};
    if constexpr (has_data_bits<O>) {
        o.data_bits = 8;
    }
    return o;
}

constexpr auto frame = make_frame<board::debug_uart::opts>();

// ── DEGREE ──────────────────────────────────────────────────────────────
//
// Not a board cap. `board::caps::*` says which ROLES a board wired up; how
// deep this peripheral's FIFO is, is a property of the die, and two boards
// built on the same chip must never be able to disagree about it. It comes
// from the chip database keyed by instance, and ZERO MEANS ABSENT — there is
// no `has_fifo` bool beside it that could drift.
constexpr std::uint32_t rx_fifo = board::debug_uart::inst::feat::rx_fifo_depth;

constexpr const char* fifo_line() {
    if constexpr (rx_fifo == 0u) {
        return "rx-fifo: none (feat::rx_fifo_depth == 0)\r\n";
    } else if constexpr (rx_fifo >= 32u) {
        return "rx-fifo: deep (32 bytes or more)\r\n";
    } else {
        return "rx-fifo: shallow (fewer than 32 bytes)\r\n";
    }
}

constexpr const char* data_bits_line() {
    if constexpr (has_data_bits<board::debug_uart::opts>) {
        return "data-bits: Layer 2 knob (opts.data_bits), programmed by this driver\r\n";
    } else {
        return "data-bits: not offered on this IP; the boot ROM owns the frame\r\n";
    }
}

constexpr const char* de_line() {
    if constexpr (has_de_timing<board::debug_uart::opts>) {
        return "rs485-de: Layer 2 timings available (de_assert_16ths)\r\n";
    } else {
        return "rs485-de: not on this IP; a uart::de<pin> tag would not compile\r\n";
    }
}

}  // namespace

int main() {
    board::init();

    // ── LAYER 1 ─────────────────────────────────────────────────────────
    //
    // Runtime, portable, and every field is programmed by every UART driver
    // alloy ships. Written out in full here to show the surface; each field
    // has a default, which is why the 59 `open({.baud = ...})` call sites in
    // the other examples still compile untouched.
    //
    // Layer 2 rides in as a TEMPLATE argument, so an unused vendor knob is not
    // a runtime branch — it is absent from the image.
    auto uart = board::debug_uart::open<frame>({
        .baud = board::debug_uart_baud,
        .parity = alloy::uart::parity::none,
        .stop = alloy::uart::stop_bits::one,
    });

    uart.write("alloy uart_frame: this port, described by the surface\r\n");
    uart.write(fifo_line());
    uart.write(data_bits_line());
    uart.write(de_line());
    uart.write("echoing; type to see bytes come back\r\n");

    while (true) {
        std::uint8_t byte{};
        if (uart.read(byte)) {
            uart.write(byte);
        }
    }
}
