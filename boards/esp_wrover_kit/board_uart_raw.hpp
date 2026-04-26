#pragma once

// ESP32 UART0 raw MMIO — bypasses HAL for bring-up.
// Bootloader pre-configures UART0 at 115200 8N1 (TX=GPIO1, RX=GPIO3).

#include <cstdint>
#include <string_view>

namespace board::uart_raw {

namespace detail {

// UART0: base 0x3FF40000
// FIFO   +0x000 : write byte to TX FIFO
// STATUS +0x01C : bits[22:16] = TXFIFO_CNT (used TX slots, max 127)
inline volatile std::uint32_t& uart_fifo() noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(0x3FF40000u);
}
inline volatile std::uint32_t& uart_status() noexcept {
    return *reinterpret_cast<volatile std::uint32_t*>(0x3FF4001Cu);
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
    for (char c : s) { write_char(c); }
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
        auto n = (v >> i) & 0xFu;
        write_char(n < 10u ? '0' + static_cast<char>(n)
                           : 'a' + static_cast<char>(n - 10u));
    }
}

}  // namespace board::uart_raw
