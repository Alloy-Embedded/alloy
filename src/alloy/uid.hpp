// The hardware device identity — the bits the silicon vendor burned in at final
// test, that no firmware, debugger or option byte can change.
//
//     #include <alloy/uid.hpp>
//
//     const auto id = board::uid.read();      // 96 bits on an STM32
//     decltype(id)::hex_buffer text;          // exactly the right size, always
//     const std::size_t n = id.to_hex(text);
//     uart.write({text.data(), n});           // "0033002E-31385105-20393443"
//
// THE COMPANION TO `alloy provision`, FROM THE OTHER DIRECTION. Provisioning
// writes a serial number and a MAC through the debug probe at production time,
// and answers "which unit is this, in OUR numbering". This answers "is this the
// same physical die as last time", needs no production step, cannot be
// forgotten, cannot be cloned onto a second board by copying a flash image, and
// is readable on the very first boot of a blank part. Programs want both:
// `provision::read()` for the number on the label, `uid.read()` for the number
// that is the part.
//
// IT IS NOT A SECRET, and this is the mistake worth heading off in the header
// rather than in a wiki. The value is readable by any code on the part and by
// any debugger, it is printed in bug reports, and it is not random — two dice
// from one wafer differ in a few bits of one word. Use it to IDENTIFY, never to
// AUTHENTICATE, and never as key material. Authenticity is a signature; see
// alloy/ota/signed.hpp.
//
// A BOARD WHOSE CHIP HAS NO UID still compiles this code: `board::uid` is a
// `null_reader` whose value type has ZERO words, which is the framework's own
// "0 means absent" degree rule applied to a whole identifier. `to_hex` then
// writes nothing and returns 0, so a program that prints the ID prints an empty
// string instead of failing to build. `board::caps::uid` is the boolean to
// branch on when you need to say something about it.
//
// WHICH IS WHY THE BUFFER IS `hex_buffer` AND NOT A C ARRAY. That claim was
// false when it was first written, and the compiler said so on eight of nine
// boards: `char text[hex_chars]` with zero words is `char text[0]`, which is
// not a type — it does not convert to a span, and under -Wpedantic it is not
// even a declaration. The size that a portable program must be allowed to write
// is zero, so the buffer has to be a thing that can BE zero-sized;
// `std::array<char, 0>` is, a C array is not. Sizing from `hex_buffer` is the
// difference between "compiles on every board" as a sentence and as a fact.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for a chip that actually has the block.
#include "alloy/hal/uid/uid_impl.hpp"

namespace alloy::uid {

// A device identifier of `Words` 32-bit words. The COUNT is a template
// parameter and not a constant because it is silicon degree: 3 on every STM32
// (96 bits), 2 on an RP2040's flash id, 4 on a SAM E70. Code that sizes its
// buffer from `hex_chars` is portable across all of them with no #if.
template <unsigned Words>
struct device_id {
    static constexpr unsigned words = Words;
    static constexpr unsigned bits = Words * 32u;
    //: Exactly the buffer `to_hex` needs. No terminator — alloy's byte sinks
    //: take a span, and a caller who wants a C string adds one byte itself.
    static constexpr std::size_t hex_chars = Words == 0u ? 0u : Words * 8u + (Words - 1u);
    //: The buffer to declare for `to_hex`, and the ONLY portable way to declare
    //: it: `char[hex_chars]` is `char[0]` on a chip with no UID block, which is
    //: not a type that converts to a span. An `array` of zero is legal, has a
    //: valid `data()`, and passes to `to_hex` as an empty span — which the
    //: zero-word identifier then satisfies by writing nothing.
    using hex_buffer = std::array<char, hex_chars>;

    //: Low word first: `word[i]` is UID[32i+31 : 32i], the manual's numbering.
    std::array<std::uint32_t, Words> word{};

    friend constexpr bool operator==(const device_id&, const device_id&) = default;

    // Canonical text: HIGH word first, eight uppercase hex digits each,
    // hyphen-separated — the order a human reads a number in, which is the
    // reverse of the order the words are stored in. Returns the count written,
    // or 0 if `out` is too small (never a partial write).
    [[nodiscard]] constexpr std::size_t to_hex(std::span<char> out) const {
        if (out.size() < hex_chars) {
            return 0u;
        }
        std::size_t n = 0;
        for (unsigned i = Words; i-- > 0;) {
            if (n != 0u) {
                out[n++] = '-';
            }
            for (int nib = 7; nib >= 0; --nib) {
                const auto d = static_cast<unsigned>((word[i] >> (nib * 4)) & 0xFu);
                out[n++] = static_cast<char>(d < 10u ? '0' + d : 'A' + (d - 10u));
            }
        }
        return n;
    }

    // 32 bits derived from all of them, for the places that need a small handle
    // rather than the whole identifier: a CAN node address, a DHCP client id, a
    // default hostname suffix. XOR-fold, deliberately trivial — it is IDENTITY
    // and not integrity, it is not a hash, and on a tray of parts from one
    // wafer the folds of neighbouring dice are close together. If you need
    // collision resistance, run a real digest over `word`.
    [[nodiscard]] constexpr std::uint32_t fold32() const {
        std::uint32_t f = 0;
        for (unsigned i = 0; i < Words; ++i) {
            f ^= word[i];
        }
        return f;
    }
};

// Stateless, zero-size handle over a generated UID instance.
template <class Inst>
class reader {
public:
    using value_type = device_id<Inst::feat::id_words>;

    // NO CLAIM, and that is a decision rather than an omission. Every other
    // facade in alloy claims its instance (alloy/core/claim.hpp) because a
    // second owner would reprogram something under the first. This block has
    // nothing to program: every register is read-only, there is no clock gate
    // to enable, no mode to select, and no state that a second reader could
    // disturb. Claiming it would cost a byte of .bss and a branch to defend
    // against a conflict that cannot exist.
    [[nodiscard]] value_type read() const { return {hal::uid_impl<Inst>::read()}; }
};

// Stand-in for a chip with no unique-ID block, so `board::uid.read()` compiles
// on every board (guard #6). Zero words, by the same rule that makes a `feat`
// count of 0 mean absent — the empty identifier, not a fake one.
struct null_reader {
    using value_type = device_id<0>;
    [[nodiscard]] value_type read() const { return {}; }
};

}  // namespace alloy::uid
