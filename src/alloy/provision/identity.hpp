// Per-device FACTORY identity: the serial number, MAC and hardware revision a
// board is born with, written once through the debug probe and read by every
// firmware that ever runs on it.
//
// The whole point is that it is NOT part of the updatable image. It lives in
// alloy::slots::provision_base — outside both A/B slots, outside the boot-state
// store, and outside the bytes `alloy update` streams — so a device can be
// reflashed, updated, rolled back and reflashed again without ever forgetting
// which device it is. (The layout carves that page out of a region that already
// existed rather than adding one; emit/slots.py explains why.)
//
// Format: a flat 64-byte little-endian POD with a self-CRC, marshaled field by
// field through alloy::byteorder exactly like the OTA image header, so the
// in-memory struct layout is irrelevant and the host writer and the firmware
// reader cannot disagree about padding.
//
//   0  u32 magic         "APRV" (fast-rejects erased 0xFF flash)
//   4  u16 format_version = 1
//   6  u16 record_size    = 64 (lets the record grow without moving the page)
//   8  u8  serial[16]     ASCII, NUL-padded; NOT NUL-terminated when full
//   24 u8  mac[6]         EUI-48 in WIRE order (mac[0] goes out first)
//   30 u16 hw_revision    board/PCB revision; 0 = unspecified
//   32 u32 batch          factory lot / batch id; 0 = unspecified
//   36 u8  reserved[24]   = 0
//   60 u32 crc32          CRC-32/ISO-HDLC over bytes [0, 60)
//
// The CRC is INTEGRITY, not authenticity: it catches a torn write or a bad
// program, not an attacker with a probe. Identity is not a secret and not a
// capability — nothing here should ever be used as a key. What stops an
// attacker rewriting the page is `alloy secure` (RDP, and WRP where the page
// falls inside the write-protected bootloader region), not this CRC.
//
// NO PER-DEVICE PRIVATE KEY. It is the obvious next field and it is deliberately
// absent: alloy has no RNG/TRNG HAL on any supported part, so the keypair would
// have to be generated on the HOST — which means the factory PC held every
// device's private half, on disk, in a CSV. That is a worse security story than
// having no per-device key at all. The field arrives when a TRNG driver and an
// on-device keygen do; `reserved` is not big enough for one, and that is on
// purpose (record_size exists so v2 can be longer without moving the page).
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "alloy/ota/crc32.hpp"
#include "alloy/util/byteorder.hpp"
#include "alloy/util/result.hpp"

namespace alloy::provision {

enum class prov_error : std::uint8_t {
    blank = 1,            // erased flash — this device was never provisioned
    bad_magic,            // something is there, but it is not an identity record
    bad_crc,              // record corrupt (torn write, bad program, bit rot)
    unsupported_version,  // a newer format_version than this firmware parses
    truncated,            // the span handed in is smaller than the record
};

inline constexpr std::size_t serial_capacity = 16;
inline constexpr std::size_t mac_size = 6;

struct identity {
    static constexpr std::uint16_t format_version = 1u;
    static constexpr std::uint16_t byte_size = 64u;
    // "APRV" composed from chars (not an >=8-hex-digit literal, per the gate).
    static constexpr std::uint32_t magic =
        static_cast<std::uint32_t>('A') | (static_cast<std::uint32_t>('P') << 8) |
        (static_cast<std::uint32_t>('R') << 16) | (static_cast<std::uint32_t>('V') << 24);

    char serial[serial_capacity]{};
    std::uint8_t mac[mac_size]{};
    std::uint32_t batch{};
    std::uint16_t hw_revision{};

    // The serial without its NUL padding — safe to print directly. A serial
    // that fills all 16 bytes has no terminator, which is exactly why this
    // exists instead of handing out a const char*.
    [[nodiscard]] std::string_view serial_view() const {
        std::size_t n = 0;
        while (n < serial_capacity && serial[n] != '\0') ++n;
        return {serial, n};
    }

    // Write exactly byte_size bytes to `out`, computing the CRC itself. Present
    // on the device (not only on the host) so the record round-trips in the host
    // tests against the SAME code the firmware parses with.
    void serialize(std::uint8_t* out) const {
        using namespace alloy::byteorder;
        store_le32(out + 0, magic);
        store_le16(out + 4, format_version);
        store_le16(out + 6, byte_size);
        for (std::size_t i = 0; i < serial_capacity; ++i) {
            out[8 + i] = static_cast<std::uint8_t>(serial[i]);
        }
        for (std::size_t i = 0; i < mac_size; ++i) {
            out[24 + i] = mac[i];
        }
        store_le16(out + 30, hw_revision);
        store_le32(out + 32, batch);
        for (std::size_t i = 36; i < 60; ++i) {
            out[i] = 0u;
        }
        store_le32(out + 60, ota::crc::crc32_of({out, 60u}));
    }

    // Parse + validate a record. `raw` is typically the first bytes of the
    // provisioning page, read straight out of memory-mapped flash.
    [[nodiscard]] static Result<identity, prov_error> parse(
        std::span<const std::uint8_t> raw) {
        using namespace alloy::byteorder;
        if (raw.size() < byte_size) return prov_error::truncated;
        const std::uint8_t* p = raw.data();
        const std::uint32_t m = load_le32(p + 0);
        if (m != magic) {
            // Erased flash reads all-ones on every part in the matrix. Telling
            // "never provisioned" apart from "provisioned with garbage" is the
            // difference between a line fault and a field fault, so they get
            // different codes.
            return m == 0xFFFF'FFFFu ? prov_error::blank : prov_error::bad_magic;
        }
        if (ota::crc::crc32_of({p, 60u}) != load_le32(p + 60)) return prov_error::bad_crc;
        if (load_le16(p + 4) != format_version) return prov_error::unsupported_version;
        identity id;
        for (std::size_t i = 0; i < serial_capacity; ++i) {
            id.serial[i] = static_cast<char>(p[8 + i]);
        }
        for (std::size_t i = 0; i < mac_size; ++i) {
            id.mac[i] = p[24 + i];
        }
        id.hw_revision = load_le16(p + 30);
        id.batch = load_le32(p + 32);
        return id;
    }
};

// Read the identity out of memory-mapped flash at `base` — the one-liner an
// app calls: `alloy::provision::read(alloy::slots::provision_base)`.
//
// Deliberately a plain read with no flash HAL: on every part in the matrix the
// internal flash is memory-mapped and readable as data, and the caller (an app,
// or the bootloader) must not need a flash controller instance just to learn its
// own serial number. There is no writer here at all — the factory writes this
// page through the probe (`alloy provision write`), so no firmware bug and no
// hostile update can rewrite a device's identity.
[[nodiscard]] inline Result<identity, prov_error> read(std::uintptr_t base) {
    // NOLINTNEXTLINE(performance-no-int-to-ptr) — memory-mapped flash.
    const auto* p = reinterpret_cast<const std::uint8_t*>(base);
    return identity::parse({p, identity::byte_size});
}

}  // namespace alloy::provision
