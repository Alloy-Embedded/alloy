// uart_impl<Inst> — primary template, intentionally undefined.
//
// One partial specialization exists per UART IP version (st_usart_v4.hpp, ...),
// constrained on the instance's IP tag type.
//
// This header also carries the SHARED VOCABULARY — the one file both the
// facade and every driver include — so the words are common without the
// facade depending on any driver. Two layers live here, and the name says
// which (docs/reference/peripheral-surface.md):
//
//   LAYER 1  uart_config     runtime, portable, frozen at the knobs EVERY
//                            shipped driver programs. A field is admitted
//                            only when all six honour it.
//   LAYER 2  uart_opts<Inst> compile-time, and its MEMBERS are declared
//                            beside the driver whose IP has the register
//                            field. A knob the silicon lacks is not a
//                            member, so it cannot be asked for.
//
// Layer 3 is alloy::dev:: — raw registers — and it is the only door below
// Layer 2. There is no generic command door.

#pragma once

#include <cstdint>

namespace alloy::hal {

// ── Layer 1 vocabulary ──────────────────────────────────────────────────
enum class parity : std::uint8_t { none, even, odd };

// ONE and TWO only. ST's STOP field also encodes 0.5 and 1.5 stop bits
// (smartcard/IrDA); the PL011, the SAM USART and the ESP32 UART have no
// such encoding, so those values fail Layer 1's admission test and would
// have to be Layer 2 knobs on the ST IPs if anyone ever needs them.
enum class stop_bits : std::uint8_t { one, two };

// The portable line shape. Every field here is programmed by every
// uart_impl<> in the tree — st_usart_v2/v3/v4, microchip_usart_v1,
// raspberrypi_uart_pl011, espressif_uart_v1 — which is the ONLY admission
// test. Adding a field costs teaching six drivers, and that friction is
// what makes the promise mean anything.
//
// NOT here, and the reasons are the rule in miniature:
//   * data bits — every driver programs a character length, but the value
//     DOMAINS do not intersect beyond 8 (v2 has {8,9}, v3/v4 {7,8,9},
//     PL011 and ESP32 {5..8}, SAM {5..8}). A runtime field whose value one
//     driver must reject, trap on, or ignore is exactly the lie this
//     replaces, so data_bits is a Layer-2 knob with the domain its own IP
//     actually has.
//   * RS-485 DE, RTS, CTS — they need a PIN, so they are binder tags.
//   * DE assert/deassert time — measured in 16ths of a bit because one
//     vendor's register field is five bits wide. A vendor artefact is a
//     Layer-2 name forever.
struct uart_config {
    std::uint32_t baud = 115'200;
    hal::parity parity = parity::none;
    hal::stop_bits stop = stop_bits::one;
};

// Layer 1 with the rate REMOVED — the argument type for a call that already
// states the baud another way (bind::open_checked<Baud>()). Having it means a
// call cannot carry two disagreeing spellings of the same fact:
// `open_checked<115'200_baud>({.baud = 9'600})` used to compile and run at
// 115 200, with the loser never mentioned.
//
// `baud` is still a MEMBER, of a type nothing converts to, because that is
// what makes the diagnostic one line that says why instead of twenty lines of
// overload-resolution notes. The name of the type IS the error message — the
// same idiom core/units.hpp uses for an out-of-range frequency literal.
namespace detail {
struct baud_is_the_template_argument_of_open_checked {};
}  // namespace detail

struct uart_frame {
    detail::baud_is_the_template_argument_of_open_checked baud = {};
    hal::parity parity = parity::none;
    hal::stop_bits stop = stop_bits::one;
};

// ── Layer 2 ─────────────────────────────────────────────────────────────
//
// The primary is EMPTY and always usable, so `uart_opts<Inst>{}` compiles
// for an IP nobody has taught any vendor knobs. Each driver header declares
// its own partial specialization, constrained on the same Inst::ip the
// driver is, listing exactly the knobs whose register field that IP has.
//
// PORTABILITY BY DISCIPLINE, NOT BY CONSTRUCTION: the same silicon feature
// must get the same member name and the same unit in every driver that has
// it, so portable code can probe by name —
//
//     if constexpr (requires { O{}.de_assert_16ths; }) { ... }
//
// — but nothing checks that rule. The grep that would is NOT BUILT.
template <class Inst>
struct uart_opts {};

// ── Deprecated ──────────────────────────────────────────────────────────
//
// The nine-field struct one driver honoured and five ignored. Kept for the
// stability window (docs/reference/stability.md); nothing in the tree calls
// it. Replaced by uart_config (Layer 1) + uart_opts<Inst> (Layer 2).
struct serial_config {
    std::uint32_t baud = 19'200;
    hal::parity parity = parity::none;
    std::uint8_t stop_bits = 1;      // 1 or 2
    bool data9 = false;              // 9 data bits (8 + parity uses 9-bit frame)
    bool invert_tx = false;          // line-level inversion (usart_v3 TXINV)
    bool invert_rx = false;
    bool de_enable = false;          // hardware driver-enable on the RTS pin
    std::uint8_t de_assert_16ths = 8;    // DE lead/tail, in 16ths of a bit
    std::uint8_t de_deassert_16ths = 8;
};

template <class Inst>
struct uart_impl;

}  // namespace alloy::hal
