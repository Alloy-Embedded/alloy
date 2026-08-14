// CRC-32/ISO-HDLC (== zlib / PKZIP / Ethernet): reflected, init all-ones, xorout
// all-ones, the standard reflected polynomial (composed from its 0xEDB8 / 0x8320
// halves below). Bytewise (8 iterations/byte, no 256-entry table) to keep firmware
// .rodata tiny — a table is a drop-in behind the same API.
// This is INTEGRITY, not security: a CRC is unkeyed, so it detects corruption but
// an attacker who rewrites an image recomputes it and passes. Authenticity needs a
// signature (see ota/verify.hpp's Verifier seam).
//
// The poly is written as two 16-bit halves and the init/xorout as the exempted
// all-ones mask, so no >=8-hex-digit literal trips the NORTH_STAR gate (a CRC
// constant is math, not a silicon address).
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace alloy::ota::crc {

inline constexpr std::uint32_t poly = (std::uint32_t{0xEDB8u} << 16) | 0x8320u;

// Incremental: feed bytes across several calls, read value() once at the end.
class crc32 {
    std::uint32_t s_ = 0xFFFF'FFFFu;

public:
    void reset() { s_ = 0xFFFF'FFFFu; }
    void update(const std::uint8_t* p, std::size_t n) {
        std::uint32_t c = s_;
        for (std::size_t i = 0; i < n; ++i) {
            c ^= p[i];
            for (int k = 0; k < 8; ++k) {
                const std::uint32_t m = 0u - (c & 1u);  // 0 or all-ones (branchless)
                c = (c >> 1) ^ (poly & m);
            }
        }
        s_ = c;
    }
    void update(std::span<const std::uint8_t> b) { update(b.data(), b.size()); }
    [[nodiscard]] std::uint32_t value() const { return s_ ^ 0xFFFF'FFFFu; }

    // One-shot over a whole buffer: reset, feed, read. Not a convenience — it
    // is the method that makes this class SUBSTITUTABLE for the hardware handle
    // in alloy/crc.hpp, which hands this class out verbatim on a chip with no
    // CRC block. A fallback missing one method is a different API wearing one
    // name, and the program that finds out is the one being ported to the chip
    // that lacks the unit. alloy::crc::Checksum32 pins the shape for both.
    [[nodiscard]] std::uint32_t checksum(std::span<const std::uint8_t> b) {
        reset();
        update(b);
        return value();
    }
};

// One-shot over a whole buffer. Spelled through checksum() so the method the
// facade depends on is the one every OTA path already exercises.
[[nodiscard]] inline std::uint32_t crc32_of(std::span<const std::uint8_t> b) {
    crc32 c;
    return c.checksum(b);
}

namespace detail {
// Folded at compile time from `poly` — no literal blob, so the table cannot
// drift from the polynomial above (the libs/modbus crc table's argument,
// applied to the 32-bit case).
consteval std::array<std::uint32_t, 256> make_crc32_table() {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t i = 0; i < 256u; ++i) {
        std::uint32_t c = i;
        for (int k = 0; k < 8; ++k) {
            c = (c >> 1) ^ (poly & (0u - (c & 1u)));
        }
        table[i] = c;
    }
    return table;
}
inline constexpr std::array<std::uint32_t, 256> crc32_table_data = make_crc32_table();
}  // namespace detail

// The same CRC-32/ISO-HDLC, one table lookup per byte instead of eight
// shifts. Same API, same numbers (a test pins them equal over every byte
// value) — this exists because a caller can hold an INTERRUPT MASK across a
// checksum, and there the eight-iteration inner loop is not a size trade,
// it is deafness: measured on a SAME70, encoding one 15-byte bus frame under
// mask took 42.8 us, which at 230400 baud is a whole received byte lost.
//
// Costs 1 KiB of .rodata, so it is NOT the default: the bootloader keeps the
// bytewise class, where the trade runs the other way and nothing is masked.
// Unreferenced, the table is dropped by --gc-sections, so a program that
// never names this class pays nothing.
class crc32_table {
    std::uint32_t s_ = 0xFFFF'FFFFu;

public:
    void reset() { s_ = 0xFFFF'FFFFu; }
    void update(const std::uint8_t* p, std::size_t n) {
        std::uint32_t c = s_;
        for (std::size_t i = 0; i < n; ++i) {
            c = detail::crc32_table_data[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
        }
        s_ = c;
    }
    void update(std::span<const std::uint8_t> b) { update(b.data(), b.size()); }
    [[nodiscard]] std::uint32_t value() const { return s_ ^ 0xFFFF'FFFFu; }
    [[nodiscard]] std::uint32_t checksum(std::span<const std::uint8_t> b) {
        reset();
        update(b);
        return value();
    }
};

[[nodiscard]] inline std::uint32_t crc32_table_of(std::span<const std::uint8_t> b) {
    crc32_table c;
    return c.checksum(b);
}

}  // namespace alloy::ota::crc
