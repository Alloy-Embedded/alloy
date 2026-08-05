// The alloy OTA image format: a 32-byte header (little-endian) followed by the
// firmware payload. Two independent CRCs: a header self-CRC (so a corrupt header
// is rejected BEFORE its payload_length is trusted) and a payload CRC (integrity
// of the firmware itself). Fields are marshaled explicitly via alloy::byteorder,
// so the in-memory struct layout is irrelevant and the format is endian-stable.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/ota/crc32.hpp"
#include "alloy/ota/error.hpp"
#include "alloy/util/byteorder.hpp"
#include "alloy/util/result.hpp"

namespace alloy::ota {

// Byte layout (offsets):
//   0  u32 magic            "ALOY" (fast-rejects erased 0xFF flash)
//   4  u16 header_version    = 1
//   6  u16 header_size       = 32 (offset to payload; lets the header grow later)
//   8  u32 image_version     MONOTONIC — higher is newer (newest-select / anti-rollback)
//   12 u32 payload_length    exact payload bytes to CRC (never CRC the whole slot)
//   16 u32 payload_crc32     CRC-32/ISO-HDLC over exactly payload_length bytes
//   20 u16 flags             reserved; bit0 = "signed" (crypto seam); 0 in v1
//   22 u16 reserved          = 0
//   24 u32 reserved2         = 0 (future: load/entry address)
//   28 u32 header_crc32      CRC-32 over bytes [0, 28)
//   32 payload begins
struct image_header {
    static constexpr std::uint16_t format_version = 1u;
    static constexpr std::uint16_t byte_size = 32u;
    // "ALOY" composed from chars (not an >=8-hex-digit literal, per the gate).
    static constexpr std::uint32_t magic =
        static_cast<std::uint32_t>('A') | (static_cast<std::uint32_t>('L') << 8) |
        (static_cast<std::uint32_t>('O') << 16) | (static_cast<std::uint32_t>('Y') << 24);

    std::uint32_t image_version{};
    std::uint32_t payload_length{};
    std::uint32_t payload_crc32{};
    std::uint16_t header_version{format_version};
    std::uint16_t header_size{byte_size};
    std::uint16_t flags{};

    // Write exactly byte_size bytes to `out`, computing header_crc32 itself.
    void serialize(std::uint8_t* out) const {
        using namespace alloy::byteorder;
        store_le32(out + 0, magic);
        store_le16(out + 4, header_version);
        store_le16(out + 6, header_size);
        store_le32(out + 8, image_version);
        store_le32(out + 12, payload_length);
        store_le32(out + 16, payload_crc32);
        store_le16(out + 20, flags);
        store_le16(out + 22, 0u);  // reserved
        store_le32(out + 24, 0u);  // reserved2
        store_le32(out + 28, crc::crc32_of({out, 28u}));
    }

    // Parse + validate the HEADER only (magic + header CRC + version). The payload
    // is checked separately, after this hands back a trusted payload_length.
    [[nodiscard]] static Result<image_header, ota_error> parse(std::span<const std::uint8_t> raw) {
        using namespace alloy::byteorder;
        if (raw.size() < byte_size) return ota_error::truncated;
        const std::uint8_t* p = raw.data();
        if (load_le32(p + 0) != magic) return ota_error::bad_magic;
        if (crc::crc32_of({p, 28u}) != load_le32(p + 28)) return ota_error::bad_header_crc;
        image_header h;
        h.header_version = load_le16(p + 4);
        h.header_size = load_le16(p + 6);
        h.image_version = load_le32(p + 8);
        h.payload_length = load_le32(p + 12);
        h.payload_crc32 = load_le32(p + 16);
        h.flags = load_le16(p + 20);
        if (h.header_version != format_version) return ota_error::unsupported_version;
        return h;
    }

    // Payload integrity: `payload` must hold at least payload_length bytes whose
    // CRC matches payload_crc32.
    [[nodiscard]] Result<void, ota_error> verify_payload(std::span<const std::uint8_t> payload) const {
        if (payload.size() < payload_length) return ota_error::truncated;
        if (crc::crc32_of(payload.first(payload_length)) != payload_crc32) {
            return ota_error::bad_payload_crc;
        }
        return alloy::ok<ota_error>();
    }
};

}  // namespace alloy::ota
