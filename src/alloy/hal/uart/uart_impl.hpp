// uart_impl<Inst> — primary template, intentionally undefined.
//
// One partial specialization exists per UART IP version (st_usart_v4.hpp, ...),
// constrained on the instance's IP tag type.

#pragma once

#include <cstdint>

namespace alloy::hal {

// Full serial line shape, for drivers that can program more than a baud rate
// (capability-gated at the facade: only drivers implementing configure() get
// it). Lives HERE — the one header both the facade and every driver include —
// so the vocabulary is shared without the facade depending on any driver.
enum class parity : std::uint8_t { none, even, odd };

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
