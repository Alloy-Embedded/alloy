// CRC-16/MODBUS (poly 0xA001 reflected, init 0xFFFF, no final xor).
//
// The 256-entry table is FOLDED AT COMPILE TIME from the polynomial — no
// 512-byte literal blob in source (nothing to typo, nothing for the address
// gate to squint at), and the check vector from the CRC catalogue is pinned
// by static_assert below, so a wrong table is a compile error, not a field
// interoperability bug. Table lookup costs 256×2 B of flash; the bitwise
// fallback would save it at 8× the per-byte cost — RTU validates a CRC per
// frame on every poll, so the table wins on an MCU too.
//
// The wire order is LITTLE-endian (lo byte first) — the one place Modbus is
// not big-endian. append_crc/check below own that fact so call sites can't
// get it backwards.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace alloy::lib::modbus {

namespace detail {

consteval std::array<std::uint16_t, 256> make_crc_table() {
    std::array<std::uint16_t, 256> table{};
    for (std::uint32_t i = 0; i < 256u; ++i) {
        std::uint16_t c = static_cast<std::uint16_t>(i);
        for (int bit = 0; bit < 8; ++bit) {
            c = ((c & 1u) != 0u) ? static_cast<std::uint16_t>(0xA001u ^ (c >> 1))
                                 : static_cast<std::uint16_t>(c >> 1);
        }
        table[i] = c;
    }
    return table;
}

inline constexpr std::array<std::uint16_t, 256> crc_table = make_crc_table();

}  // namespace detail

// Running CRC for byte-at-a-time consumers (the framer feeds as bytes arrive
// instead of re-scanning the buffer on completion).
class crc16_accumulator {
public:
    constexpr void update(std::uint8_t b) {
        crc_ = static_cast<std::uint16_t>(
            detail::crc_table[(crc_ ^ b) & 0xFFu] ^ (crc_ >> 8));
    }
    [[nodiscard]] constexpr std::uint16_t value() const { return crc_; }
    constexpr void reset() { crc_ = 0xFFFFu; }

private:
    std::uint16_t crc_{0xFFFFu};
};

[[nodiscard]] constexpr std::uint16_t crc16(std::span<const std::uint8_t> data) {
    crc16_accumulator acc;
    for (std::uint8_t b : data) {
        acc.update(b);
    }
    return acc.value();
}

// Append the CRC of out[0..len) at out[len], lo byte first (the RTU wire
// order). Caller guarantees out holds len + 2. Returns the new total.
constexpr std::size_t append_crc(std::span<std::uint8_t> out, std::size_t len) {
    const std::uint16_t c = crc16(out.first(len));
    out[len] = static_cast<std::uint8_t>(c & 0xFFu);
    out[len + 1] = static_cast<std::uint8_t>(c >> 8);
    return len + 2;
}

// True when frame's trailing two bytes are the correct CRC of what precedes
// them. frame.size() must be >= 3 (checked by the caller's minimum-length
// gate; a shorter span here is a programmer error, not wire input).
[[nodiscard]] constexpr bool crc_ok(std::span<const std::uint8_t> frame) {
    const std::size_t n = frame.size() - 2;
    const std::uint16_t wire = static_cast<std::uint16_t>(
        frame[n] | static_cast<std::uint16_t>(frame[n + 1]) << 8);
    return crc16(frame.first(n)) == wire;
}

namespace detail {
// The catalogue check value ("123456789" -> 0x4B37) and the spec's own
// example frame (unit 0x11 read 3 holding regs @0x006B -> CRC wire 76 87),
// verified BY THE COMPILER: a table regression cannot reach a build.
inline constexpr std::uint8_t crc_check_input[9] = {'1', '2', '3', '4', '5',
                                                    '6', '7', '8', '9'};
static_assert(crc16(crc_check_input) == 0x4B37u);
inline constexpr std::uint8_t crc_spec_frame[8] = {0x11, 0x03, 0x00, 0x6B,
                                                   0x00, 0x03, 0x76, 0x87};
static_assert(crc_ok(crc_spec_frame));
}  // namespace detail

}  // namespace alloy::lib::modbus
