#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace alloy::examples::uart_console {

template <typename Uart>
inline auto write_text(const Uart& uart, std::string_view text) -> void {
    const auto bytes =
        std::span{reinterpret_cast<const std::byte*>(text.data()), static_cast<std::size_t>(text.size())};
    static_cast<void>(uart.write(bytes));
    static_cast<void>(uart.flush());
}

template <typename Uart>
inline auto write_line(const Uart& uart, std::string_view text) -> void {
    write_text(uart, text);
    write_text(uart, "\r\n");
}

template <typename Uart>
inline auto write_hex_byte(const Uart& uart, std::uint8_t value) -> void {
    constexpr auto kHex = "0123456789ABCDEF";
    std::array<char, 4> buffer{
        kHex[(value >> 4) & 0x0F],
        kHex[value & 0x0F],
        '\r',
        '\n',
    };
    write_text(uart, std::string_view{buffer.data(), buffer.size()});
}

template <typename Uart>
inline auto write_unsigned(const Uart& uart, std::uint32_t value) -> void {
    std::array<char, 10> digits{};
    auto index = digits.size();
    do {
        digits[--index] = static_cast<char>('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u);
    write_text(uart, std::string_view{digits.data() + index, digits.size() - index});
}

}  // namespace alloy::examples::uart_console
