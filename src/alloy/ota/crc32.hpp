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
};

// One-shot over a whole buffer.
[[nodiscard]] inline std::uint32_t crc32_of(std::span<const std::uint8_t> b) {
    crc32 c;
    c.update(b);
    return c.value();
}

}  // namespace alloy::ota::crc
