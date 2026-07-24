// Endianness + byte-array (un)packing for protocol and register work —
// replaces the rd16/wr16/load-store helpers hand-rolled inside drivers (the
// net_echo frame code). Built on C++23 <bit> (std::byteswap / std::endian).
// Deliberately uses byte-wise shifts and <=2-digit masks: an 8-hex-digit mask
// literal would trip the NORTH_STAR address gate (guard #1).

#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

namespace alloy::byteorder {

template <class T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T to_big(T v) {
    if constexpr (std::endian::native == std::endian::big) {
        return v;
    } else {
        return std::byteswap(v);
    }
}
template <class T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr T to_little(T v) {
    if constexpr (std::endian::native == std::endian::little) {
        return v;
    } else {
        return std::byteswap(v);
    }
}
// byteswap is its own inverse, so from_* is the same transform as to_*.
template <class T>
[[nodiscard]] constexpr T from_big(T v) {
    return to_big(v);
}
template <class T>
[[nodiscard]] constexpr T from_little(T v) {
    return to_little(v);
}

// Byte-wise load/store — endianness explicit, alignment-agnostic (pointers
// into packed frame buffers carry no alignment guarantee).
[[nodiscard]] inline std::uint16_t load_be16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}
[[nodiscard]] inline std::uint16_t load_le16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}
[[nodiscard]] inline std::uint32_t load_be32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) | static_cast<std::uint32_t>(p[3]);
}
[[nodiscard]] inline std::uint32_t load_le32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}
inline void store_be16(std::uint8_t* p, std::uint16_t v) {
    p[0] = static_cast<std::uint8_t>(v >> 8);
    p[1] = static_cast<std::uint8_t>(v);
}
inline void store_le16(std::uint8_t* p, std::uint16_t v) {
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8);
}
inline void store_be32(std::uint8_t* p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v >> 24);
    p[1] = static_cast<std::uint8_t>(v >> 16);
    p[2] = static_cast<std::uint8_t>(v >> 8);
    p[3] = static_cast<std::uint8_t>(v);
}
inline void store_le32(std::uint8_t* p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8);
    p[2] = static_cast<std::uint8_t>(v >> 16);
    p[3] = static_cast<std::uint8_t>(v >> 24);
}

}  // namespace alloy::byteorder
