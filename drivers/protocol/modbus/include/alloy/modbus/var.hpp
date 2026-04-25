#pragma once

// var.hpp: typed Modbus variable descriptor + word-order encode/decode.
//
// Supported value types: bool, int16_t, uint16_t, int32_t, uint32_t, float, double.
// Register counts: bool/int16_t/uint16_t → 1, int32_t/uint32_t/float → 2, double → 4.
//
// Word order applies to multi-register types; single-register types ignore it.
//   ABCD: big-endian (MSW at lower Modbus address)
//   CDAB: swap words (LSW first, bytes within each word unchanged)
//   BADC: swap bytes within each word (MSW first, byte-reversed)
//   DCBA: fully little-endian (LSW first, bytes reversed within each word)

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace alloy::modbus {

// ============================================================================
// Enumerations
// ============================================================================

enum class WordOrder : std::uint8_t {
    ABCD,  // big-endian, MSW first (Modbus default)
    CDAB,  // word-swapped (LSW first, bytes in each word unchanged)
    BADC,  // byte-swapped within each word (MSW first)
    DCBA,  // fully little-endian
};

enum class Access : std::uint8_t {
    ReadOnly,
    WriteOnly,
    ReadWrite,
};

// ============================================================================
// Concept: allowed value types
// ============================================================================

template <typename T>
concept VarValueType =
    std::same_as<T, bool>           ||
    std::same_as<T, std::int16_t>   ||
    std::same_as<T, std::uint16_t>  ||
    std::same_as<T, std::int32_t>   ||
    std::same_as<T, std::uint32_t>  ||
    std::same_as<T, float>          ||
    std::same_as<T, double>;

// ============================================================================
// Register count for a given type
// ============================================================================

template <VarValueType T>
[[nodiscard]] consteval std::size_t var_reg_count() noexcept {
    if constexpr (std::same_as<T, double>) { return 4u; }
    else if constexpr (sizeof(T) <= 2u)    { return 1u; }
    else                                   { return 2u; }
}

// ============================================================================
// Word-order encode / decode
// ============================================================================

namespace detail {

[[nodiscard]] constexpr std::uint16_t byte_swap16(std::uint16_t v) noexcept {
    return static_cast<std::uint16_t>((v >> 8u) | (v << 8u));
}

}  // namespace detail

// Encode a typed value to an array of Modbus registers (uint16_t, big-endian
// register ordering per Modbus wire format). Word order applied as specified.
template <VarValueType T>
[[nodiscard]] constexpr std::array<std::uint16_t, var_reg_count<T>()>
encode_words(T value, WordOrder order = WordOrder::ABCD) noexcept {
    constexpr std::size_t N = var_reg_count<T>();

    if constexpr (N == 1u) {
        // bool → 0x0001/0x0000; int16/uint16 → reinterpret as uint16.
        std::uint16_t raw{};
        if constexpr (std::same_as<T, bool>) {
            raw = value ? 0x0001u : 0x0000u;
        } else if constexpr (std::same_as<T, std::int16_t>) {
            raw = static_cast<std::uint16_t>(value);
        } else {
            raw = value;
        }
        return {raw};

    } else if constexpr (N == 2u) {
        std::uint32_t bits{};
        if constexpr (std::same_as<T, float>) {
            bits = std::bit_cast<std::uint32_t>(value);
        } else if constexpr (std::same_as<T, std::int32_t>) {
            bits = static_cast<std::uint32_t>(value);
        } else {
            bits = value;
        }
        const auto hi = static_cast<std::uint16_t>(bits >> 16u);
        const auto lo = static_cast<std::uint16_t>(bits & 0xFFFFu);

        switch (order) {
            case WordOrder::ABCD: return {hi, lo};
            case WordOrder::CDAB: return {lo, hi};
            case WordOrder::BADC: return {detail::byte_swap16(hi), detail::byte_swap16(lo)};
            case WordOrder::DCBA: return {detail::byte_swap16(lo), detail::byte_swap16(hi)};
        }
        return {hi, lo};  // unreachable

    } else {  // N == 4 (double)
        const auto bits = std::bit_cast<std::uint64_t>(value);
        const auto a = static_cast<std::uint16_t>(bits >> 48u);
        const auto b = static_cast<std::uint16_t>((bits >> 32u) & 0xFFFFu);
        const auto c = static_cast<std::uint16_t>((bits >> 16u) & 0xFFFFu);
        const auto d = static_cast<std::uint16_t>(bits & 0xFFFFu);

        switch (order) {
            case WordOrder::ABCD: return {a, b, c, d};
            case WordOrder::CDAB: return {c, d, a, b};
            case WordOrder::BADC:
                return {detail::byte_swap16(a), detail::byte_swap16(b),
                        detail::byte_swap16(c), detail::byte_swap16(d)};
            case WordOrder::DCBA:
                return {detail::byte_swap16(d), detail::byte_swap16(c),
                        detail::byte_swap16(b), detail::byte_swap16(a)};
        }
        return {a, b, c, d};  // unreachable
    }
}

// Decode Modbus registers back to a typed value. Inverse of encode_words.
template <VarValueType T>
[[nodiscard]] constexpr T decode_words(
    std::span<const std::uint16_t, var_reg_count<T>()> words,
    WordOrder order = WordOrder::ABCD) noexcept {
    constexpr std::size_t N = var_reg_count<T>();

    if constexpr (N == 1u) {
        if constexpr (std::same_as<T, bool>) {
            return words[0] != 0u;
        } else if constexpr (std::same_as<T, std::int16_t>) {
            return static_cast<std::int16_t>(words[0]);
        } else {
            return words[0];
        }

    } else if constexpr (N == 2u) {
        std::uint16_t hi{};
        std::uint16_t lo{};
        switch (order) {
            case WordOrder::ABCD: hi = words[0]; lo = words[1]; break;
            case WordOrder::CDAB: lo = words[0]; hi = words[1]; break;
            case WordOrder::BADC:
                hi = detail::byte_swap16(words[0]);
                lo = detail::byte_swap16(words[1]);
                break;
            case WordOrder::DCBA:
                lo = detail::byte_swap16(words[0]);
                hi = detail::byte_swap16(words[1]);
                break;
        }
        const auto bits = (static_cast<std::uint32_t>(hi) << 16u) | lo;
        if constexpr (std::same_as<T, float>) {
            return std::bit_cast<float>(bits);
        } else if constexpr (std::same_as<T, std::int32_t>) {
            return static_cast<std::int32_t>(bits);
        } else {
            return bits;
        }

    } else {  // N == 4 (double)
        std::uint16_t a{}, b_w{}, c{}, d{};
        switch (order) {
            case WordOrder::ABCD:
                a = words[0]; b_w = words[1]; c = words[2]; d = words[3]; break;
            case WordOrder::CDAB:
                c = words[0]; d = words[1]; a = words[2]; b_w = words[3]; break;
            case WordOrder::BADC:
                a   = detail::byte_swap16(words[0]);
                b_w = detail::byte_swap16(words[1]);
                c   = detail::byte_swap16(words[2]);
                d   = detail::byte_swap16(words[3]);
                break;
            case WordOrder::DCBA:
                d   = detail::byte_swap16(words[0]);
                c   = detail::byte_swap16(words[1]);
                b_w = detail::byte_swap16(words[2]);
                a   = detail::byte_swap16(words[3]);
                break;
        }
        const auto bits =
            (static_cast<std::uint64_t>(a)   << 48u) |
            (static_cast<std::uint64_t>(b_w) << 32u) |
            (static_cast<std::uint64_t>(c)   << 16u) |
             static_cast<std::uint64_t>(d);
        return std::bit_cast<double>(bits);
    }
}

// ============================================================================
// Var<T>: compile-time variable descriptor
// ============================================================================

template <VarValueType T>
struct Var {
    using value_type = T;
    static constexpr std::size_t kRegCount = var_reg_count<T>();

    std::uint16_t    address;
    Access           access;
    std::string_view name;
    WordOrder        word_order{WordOrder::ABCD};

    [[nodiscard]] constexpr std::array<std::uint16_t, kRegCount>
    encode(T value) const noexcept {
        return encode_words<T>(value, word_order);
    }

    [[nodiscard]] constexpr T
    decode(std::span<const std::uint16_t, kRegCount> words) const noexcept {
        return decode_words<T>(words, word_order);
    }
};

}  // namespace alloy::modbus
