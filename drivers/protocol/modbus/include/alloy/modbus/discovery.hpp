#pragma once

// discovery.hpp: vendor-specific FC 0x65 (configurable) discovery protocol.
//
// Sub-function 0x01 (thin): enumerate all vars with address, reg_count, type,
//   access, and name.
// Sub-function 0x02 (rich): thin + unit, desc, range_min, range_max per var
//   (vars without VarMeta use empty strings and 0.0 range).
//
// PDU layout:
//   Request:  [FC][sub_fn=0x01|0x02]
//   Response: [FC][sub_fn][count_hi][count_lo] then N entries:
//     Thin entry: [addr_hi][addr_lo][reg_count][type_tag][access]
//                 [name_len][name_bytes...]
//     Rich entry: thin_entry + [unit_len][unit...][desc_len][desc...]
//                 [range_min_4B_BE][range_max_4B_BE]
//
// Decoding (master side): callback-based, no allocation.

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "alloy/modbus/pdu.hpp"
#include "alloy/modbus/registry.hpp"
#include "alloy/modbus/var.hpp"
#include "core/result.hpp"

namespace alloy::modbus {

// ============================================================================
// Discovery sub-function codes
// ============================================================================

constexpr std::uint8_t kDiscoverySubThin = 0x01u;
constexpr std::uint8_t kDiscoverySubRich = 0x02u;
constexpr std::uint8_t kDiscoveryFcDefault = 0x65u;

// ============================================================================
// Decoded entry types (master-side)
// ============================================================================

struct DiscoveryEntry {
    std::uint16_t    address;
    std::uint8_t     reg_count;
    VarType          type_tag;
    Access           access;
    std::string_view name;
    // Rich-only (empty/zero for thin responses):
    std::string_view unit;
    std::string_view desc;
    float            range_min{0.0f};
    float            range_max{0.0f};
};

// ============================================================================
// Encoding (slave side)
// ============================================================================

namespace detail {

// Write one byte; advance pos. Returns false on overflow.
[[nodiscard]] constexpr bool put_byte(std::span<std::byte> out, std::size_t& pos,
                                       std::uint8_t v) noexcept {
    if (pos >= out.size()) return false;
    out[pos++] = std::byte{v};
    return true;
}

// Write a uint16_t big-endian.
[[nodiscard]] constexpr bool put_u16(std::span<std::byte> out, std::size_t& pos,
                                      std::uint16_t v) noexcept {
    return put_byte(out, pos, static_cast<std::uint8_t>(v >> 8u)) &&
           put_byte(out, pos, static_cast<std::uint8_t>(v & 0xFFu));
}

// Write a length-prefixed string (1-byte length, then bytes). Truncates to 255.
[[nodiscard]] constexpr bool put_string(std::span<std::byte> out, std::size_t& pos,
                                         std::string_view s) noexcept {
    const auto len = static_cast<std::uint8_t>(std::min(s.size(), std::size_t{255u}));
    if (!put_byte(out, pos, len)) return false;
    for (std::uint8_t i = 0u; i < len; ++i) {
        if (!put_byte(out, pos, static_cast<std::uint8_t>(s[i]))) return false;
    }
    return true;
}

// Write a 32-bit float as 4 big-endian bytes (IEEE 754).
[[nodiscard]] constexpr bool put_f32_be(std::span<std::byte> out, std::size_t& pos,
                                         float v) noexcept {
    const auto bits = std::bit_cast<std::uint32_t>(v);
    return put_byte(out, pos, static_cast<std::uint8_t>(bits >> 24u)) &&
           put_byte(out, pos, static_cast<std::uint8_t>((bits >> 16u) & 0xFFu)) &&
           put_byte(out, pos, static_cast<std::uint8_t>((bits >> 8u)  & 0xFFu)) &&
           put_byte(out, pos, static_cast<std::uint8_t>(bits & 0xFFu));
}

}  // namespace detail

// Encode a thin discovery response (sub-function 0x01) for a registry.
// Returns the number of bytes written on success.
template <std::size_t N>
[[nodiscard]] core::Result<std::size_t, PduError> encode_discovery_thin(
    std::span<std::byte> out, std::uint8_t discovery_fc,
    const Registry<N>& registry) noexcept {
    std::size_t pos = 0u;

    // Header: [FC][sub=0x01][count_hi][count_lo]
    if (!detail::put_byte(out, pos, discovery_fc))         return core::Err(PduError::BufferTooSmall);
    if (!detail::put_byte(out, pos, kDiscoverySubThin))    return core::Err(PduError::BufferTooSmall);
    if (!detail::put_u16(out, pos, static_cast<std::uint16_t>(N))) return core::Err(PduError::BufferTooSmall);

    for (const auto& d : registry) {
        if (!detail::put_u16(out, pos, d.address))             return core::Err(PduError::BufferTooSmall);
        if (!detail::put_byte(out, pos, d.reg_count))          return core::Err(PduError::BufferTooSmall);
        if (!detail::put_byte(out, pos, static_cast<std::uint8_t>(d.type_tag)))  return core::Err(PduError::BufferTooSmall);
        if (!detail::put_byte(out, pos, static_cast<std::uint8_t>(d.access)))   return core::Err(PduError::BufferTooSmall);
        if (!detail::put_string(out, pos, d.name))             return core::Err(PduError::BufferTooSmall);
    }
    return core::Ok(std::size_t{pos});
}

// Encode a rich discovery response (sub-function 0x02).
template <std::size_t N>
[[nodiscard]] core::Result<std::size_t, PduError> encode_discovery_rich(
    std::span<std::byte> out, std::uint8_t discovery_fc,
    const Registry<N>& registry) noexcept {
    std::size_t pos = 0u;

    if (!detail::put_byte(out, pos, discovery_fc))         return core::Err(PduError::BufferTooSmall);
    if (!detail::put_byte(out, pos, kDiscoverySubRich))    return core::Err(PduError::BufferTooSmall);
    if (!detail::put_u16(out, pos, static_cast<std::uint16_t>(N))) return core::Err(PduError::BufferTooSmall);

    for (const auto& d : registry) {
        // Thin fields
        if (!detail::put_u16(out, pos, d.address))             return core::Err(PduError::BufferTooSmall);
        if (!detail::put_byte(out, pos, d.reg_count))          return core::Err(PduError::BufferTooSmall);
        if (!detail::put_byte(out, pos, static_cast<std::uint8_t>(d.type_tag)))  return core::Err(PduError::BufferTooSmall);
        if (!detail::put_byte(out, pos, static_cast<std::uint8_t>(d.access)))   return core::Err(PduError::BufferTooSmall);
        if (!detail::put_string(out, pos, d.name))             return core::Err(PduError::BufferTooSmall);
        // Rich fields
        const std::string_view unit = d.meta ? d.meta->unit : std::string_view{};
        const std::string_view desc = d.meta ? d.meta->desc : std::string_view{};
        const float range_min = d.meta ? d.meta->range_min : 0.0f;
        const float range_max = d.meta ? d.meta->range_max : 0.0f;
        if (!detail::put_string(out, pos, unit))               return core::Err(PduError::BufferTooSmall);
        if (!detail::put_string(out, pos, desc))               return core::Err(PduError::BufferTooSmall);
        if (!detail::put_f32_be(out, pos, range_min))          return core::Err(PduError::BufferTooSmall);
        if (!detail::put_f32_be(out, pos, range_max))          return core::Err(PduError::BufferTooSmall);
    }
    return core::Ok(std::size_t{pos});
}

// ============================================================================
// Decoding (master side) — callback-based, no allocation
// ============================================================================

namespace detail {

[[nodiscard]] constexpr bool get_byte(std::span<const std::byte> in,
                                       std::size_t& pos, std::uint8_t& out) noexcept {
    if (pos >= in.size()) return false;
    out = static_cast<std::uint8_t>(in[pos++]);
    return true;
}

[[nodiscard]] constexpr bool get_u16(std::span<const std::byte> in,
                                      std::size_t& pos, std::uint16_t& out) noexcept {
    std::uint8_t hi{}, lo{};
    if (!get_byte(in, pos, hi) || !get_byte(in, pos, lo)) return false;
    out = (static_cast<std::uint16_t>(hi) << 8u) | lo;
    return true;
}

// Reads a length-prefixed string; returns a string_view into the raw PDU bytes.
[[nodiscard]] constexpr bool get_string(std::span<const std::byte> in,
                                         std::size_t& pos,
                                         std::string_view& out) noexcept {
    std::uint8_t len{};
    if (!get_byte(in, pos, len)) return false;
    if (pos + len > in.size()) return false;
    out = std::string_view{
        reinterpret_cast<const char*>(in.data() + pos),
        static_cast<std::size_t>(len)};
    pos += len;
    return true;
}

[[nodiscard]] constexpr bool get_f32_be(std::span<const std::byte> in,
                                         std::size_t& pos, float& out) noexcept {
    std::uint8_t b0{}, b1{}, b2{}, b3{};
    if (!get_byte(in, pos, b0) || !get_byte(in, pos, b1) ||
        !get_byte(in, pos, b2) || !get_byte(in, pos, b3)) return false;
    const auto bits = (static_cast<std::uint32_t>(b0) << 24u) |
                      (static_cast<std::uint32_t>(b1) << 16u) |
                      (static_cast<std::uint32_t>(b2) << 8u)  |
                       static_cast<std::uint32_t>(b3);
    out = std::bit_cast<float>(bits);
    return true;
}

}  // namespace detail

// Decode a thin or rich discovery response PDU.
// Calls callback(DiscoveryEntry) for each var.
// Returns Err(Truncated) if the PDU is malformed.
template <typename Fn>
[[nodiscard]] core::Result<std::uint16_t, PduError> decode_discovery_response(
    std::span<const std::byte> pdu, Fn&& callback) noexcept {
    std::size_t pos = 0u;

    std::uint8_t fc{};
    std::uint8_t sub_fn{};
    std::uint16_t count{};

    if (!detail::get_byte(pdu, pos, fc))   return core::Err(PduError::Truncated);
    if (!detail::get_byte(pdu, pos, sub_fn)) return core::Err(PduError::Truncated);
    if (!detail::get_u16(pdu, pos, count)) return core::Err(PduError::Truncated);

    const bool is_rich = (sub_fn == kDiscoverySubRich);

    for (std::uint16_t i = 0u; i < count; ++i) {
        DiscoveryEntry entry{};

        std::uint8_t type_raw{}, access_raw{};
        if (!detail::get_u16(pdu, pos, entry.address))  return core::Err(PduError::Truncated);
        if (!detail::get_byte(pdu, pos, entry.reg_count)) return core::Err(PduError::Truncated);
        if (!detail::get_byte(pdu, pos, type_raw))       return core::Err(PduError::Truncated);
        if (!detail::get_byte(pdu, pos, access_raw))     return core::Err(PduError::Truncated);
        entry.type_tag = static_cast<VarType>(type_raw);
        entry.access   = static_cast<Access>(access_raw);
        if (!detail::get_string(pdu, pos, entry.name)) return core::Err(PduError::Truncated);

        if (is_rich) {
            if (!detail::get_string(pdu, pos, entry.unit))      return core::Err(PduError::Truncated);
            if (!detail::get_string(pdu, pos, entry.desc))      return core::Err(PduError::Truncated);
            if (!detail::get_f32_be(pdu, pos, entry.range_min)) return core::Err(PduError::Truncated);
            if (!detail::get_f32_be(pdu, pos, entry.range_max)) return core::Err(PduError::Truncated);
        }

        callback(entry);
    }
    return core::Ok(std::uint16_t{count});
}

}  // namespace alloy::modbus
