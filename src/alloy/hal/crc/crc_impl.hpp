// crc_impl<Inst> — primary template, intentionally undefined. One partial
// specialization per CRC IP version, constrained on the instance's IP tag type
// (data-driven driver selection).
//
// Plus the one piece of shared vocabulary a CRC driver actually has: the rule
// for turning a byte stream into data-register writes. It lives here rather
// than inside a vendor driver because it is the part a host test must be able
// to run — see tests/test_crc.cpp, which drives THIS function into a model of
// the silicon instead of into the silicon.

#pragma once

#include <cstddef>
#include <cstdint>

namespace alloy::hal {

template <class Inst>
struct crc_impl;

namespace crc_detail {

// The CRC-32/ISO-HDLC parameters, stated as DECISIONS rather than as bit
// patterns, in a header that is not generated so a host test can include it.
//
// The split is deliberate and it is what makes the equivalence checkable. The
// ENCODING of each decision — which two bits mean "reverse by byte", where
// POLYSIZE sits — belongs to the chip database and reaches the driver through
// the generated `IP::cr` enumerators, and alloy-devices' own tests pin it.
// The DECISION — that this checksum wants byte-granular input reversal, output
// reversal, a 32-bit polynomial, this seed and this polynomial — belongs here,
// where tests/test_crc.cpp reads the same constants into a model of the block
// and compares the result against alloy::ota::crc::crc32_of. Neither half can
// drift without something failing.
struct iso_hdlc {
    //: Polynomial width in bits (CR.POLYSIZE selects it).
    static constexpr unsigned width_bits = 32u;
    //: CR.REV_IN granularity: reverse each BYTE of whatever is written. See
    //: feed() below for why byte and not word.
    static constexpr unsigned reverse_input_bits = 8u;
    //: CR.REV_OUT: reflect the remainder on the way out.
    static constexpr bool reverse_output = true;
    //: The polynomial in NORMAL (MSB-first) form — the same polynomial as
    //: software's reflected 0xEDB8_8320, not a different one. Written as two
    //: 16-bit halves because an 8-hex-digit literal is a silicon address to
    //: the contract gate, and a CRC polynomial is math (alloy/ota/crc32.hpp
    //: spells its reflected twin the same way, for the same reason).
    static constexpr std::uint32_t polynomial = (std::uint32_t{0x04C1u} << 16) | 0x1DB7u;
    //: Seed loaded on reset, and — because no STM32 CRC block has an xorout
    //: register — also the value the driver XORs into the result. Both are
    //: all-ones for this algorithm; they are named once so the second use
    //: cannot silently become something else.
    static constexpr std::uint32_t seed = 0xFFFF'FFFFu;
};

// Feed `n` bytes at `p` to `sink`, as whole 32-bit writes while four or more
// remain and single-byte writes for the rest.
//
// WORDS ARE ASSEMBLED BIG-ENDIAN — p[0] in the TOP byte — and that is the load-
// bearing line of the whole driver. The ST block is configured with
// CR.REV_IN = BYTE, so it bit-reverses each byte lane and then consumes the
// word MSB lane first: p[0] enters the shift register first, least significant
// bit first, which is exactly what `refin = true` means for a byte stream. Get
// the assembly order wrong and the checksum is still a checksum, still stable,
// and equal to nothing anyone else computes.
//
// Two things fall out of choosing BYTE reversal over WORD reversal, and both
// are why it was chosen:
//
//   * the 1-3 byte TAIL needs no second control-register write. A byte written
//     to DR under REV_IN = BYTE is reversed and consumed on its own, so a
//     buffer whose length is not a multiple of four finishes in the same
//     configuration it started in.
//   * INCREMENTAL updates are correct across arbitrary chunk boundaries.
//     update(3 bytes) then update(4 bytes) feeds three byte-writes and one
//     word-write, and the engine sees the same seven bytes in the same order
//     as one update(7). REV_IN = WORD would have forced this function to carry
//     a partial word between calls, which is state, in a driver that otherwise
//     has none.
//
// Assembling the word explicitly rather than loading a `std::uint32_t` also
// costs nothing and buys two things: it is independent of the CPU's byte order,
// and it is safe on an unaligned buffer — an OTA payload starts wherever the
// header ended.
template <class Sink>
constexpr void feed(Sink&& sink, const std::uint8_t* p, std::size_t n) {
    for (; n >= 4u; p += 4, n -= 4) {
        sink.word((std::uint32_t{p[0]} << 24) | (std::uint32_t{p[1]} << 16) |
                  (std::uint32_t{p[2]} << 8) | std::uint32_t{p[3]});
    }
    for (; n != 0u; ++p, --n) {
        sink.byte(*p);
    }
}

}  // namespace crc_detail
}  // namespace alloy::hal
