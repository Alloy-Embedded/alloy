#pragma once

// ESP32-C3 UART0 raw MMIO write — bypasses the HAL for bring-up.
// ROM pre-configures UART0 at 115200 8N1 (TX=GPIO21, RX=GPIO20).
// Assumes ROM clock (40 MHz crystal or 80 MHz PLL) and its baud setting.

#include <cstdint>
#include <string_view>

namespace board::uart_raw {

namespace detail {

// UART0 base = 0x60000000
// FIFO   +0x000: write byte to TX FIFO
// STATUS +0x01C: bits[20:16] = TXFIFO_CNT (used slots)
inline volatile std::uint32_t& uart_fifo() noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(0x60000000u);
}
inline volatile std::uint32_t& uart_status() noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(0x6000001Cu);
}

inline void wait_tx_space() noexcept {
    while (((uart_status() >> 16u) & 0x7Fu) >= 126u) {}
}

}  // namespace detail

inline void write_char(char c) noexcept {
    detail::wait_tx_space();
    detail::uart_fifo() = static_cast<std::uint32_t>(static_cast<unsigned char>(c));
}

inline void write(std::string_view s) noexcept {
    for (char c : s) {
        write_char(c);
    }
}

inline void writeln(std::string_view s) noexcept {
    write(s);
    write_char('\r');
    write_char('\n');
}

inline void write_uint32(std::uint32_t v) noexcept {
    if (v == 0u) { write_char('0'); return; }
    char buf[10];
    int n = 0;
    while (v) { buf[n++] = '0' + static_cast<char>(v % 10u); v /= 10u; }
    for (int i = n - 1; i >= 0; --i) { write_char(buf[i]); }
}

inline void write_hex32(std::uint32_t v) noexcept {
    write("0x");
    for (int i = 28; i >= 0; i -= 4) {
        auto nibble = (v >> i) & 0xFu;
        write_char(nibble < 10u ? '0' + static_cast<char>(nibble)
                                : 'a' + static_cast<char>(nibble - 10u));
    }
}

}  // namespace board::uart_raw
