// Hardware CRC-32, with a software twin of the identical shape.
//
//     #include <alloy/crc.hpp>
//
//     auto c = board::crc.open();          // once, anywhere
//     c.update(header);
//     c.update(payload);
//     if (c.value() != expected) { /* corrupt */ }
//
// THE NUMBER IS THE SAME NUMBER. `alloy::ota::crc::crc32_of()` — the software
// CRC-32/ISO-HDLC every alloy on-flash format already uses — returns exactly
// this value for the same bytes. That is not a coincidence to be maintained by
// care; it is checked in tests/test_crc.cpp against a model of the silicon, for
// every length from 0 to 64 bytes, and it is the ONLY reason this facade
// exists. A hardware CRC that computed a different polynomial or a different
// bit order would be a faster way to reject every image the updater ever wrote.
//
// THIS COMPILES ON EVERY BOARD, and on a chip with no CRC block `open()` hands
// back the SOFTWARE crc32 instead. Same number, and the same four methods —
// `reset`, `update`, `value`, `checksum` — so no `if constexpr` and no
// preprocessor at the call site. "Same methods" is not a promise kept by care:
// it is the `Checksum32` concept below, and it is static_assert-ed for the
// software class where it is named and for `handle<Inst>` inside `engine`, so a
// method added to one and not the other fails to compile the board that has the
// block AND the board that does not. (It was a promise kept by care once, and
// the care ran out: the software class had no `checksum()`, so examples/device_id
// built on exactly the one board of nine that has the silicon.) That symmetry is
// the whole reason a fallback is honest here and would not be for, say, a UART:
// a substitute is only allowed to be silent when it is EQUAL. `board::caps::crc`
// says which one you got, for a program whose decision depends on the speed
// rather than the value.
//
// WHAT THERE IS NO KNOB FOR, deliberately. The ST v3 block can also do CRC-16,
// CRC-8 and the 7-bit SD/MMC polynomial, with a programmable seed and a
// programmable polynomial, and none of that is offered here. There is no
// portable `config` and no `opts<Inst>`, because:
//
//   * no other value has a consumer. Every format alloy reads or writes is
//     ISO-HDLC, so a second algorithm would be a knob whose only effect is to
//     make the software fallback and the hardware disagree.
//   * a knob whose values cannot all be honoured is the lie the peripheral
//     surface exists to forbid. A portable `algorithm` enum with four values
//     and one driver that programs them is exactly the `de_enable`-shaped
//     defect that motivated the layered surface in the first place.
//   * the door is not closed. `alloy::dev::crc_t` and `alloy::ip::st::crc_v3`
//     are emitted, with POL, INIT and CR.POLYSIZE's encodings all curated, so
//     a program that wants CRC-16 writes five register lines at Layer 3 — with
//     the escape hatch's warnings, which is where an unshared choice belongs.
//
// This is the encoder build's finding applied to a facade instead of a binder:
// a parameter that carries no fact the driver programs is a dependency the
// peripheral does not have.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "alloy/core/claim.hpp"
// Only the primary template — the concrete per-IP driver is pulled in by the
// generated device.hpp for a chip that actually has the block.
#include "alloy/hal/crc/crc_impl.hpp"
#include "alloy/ota/crc32.hpp"

namespace alloy::crc {

// The running-checksum shape. Both the hardware handle and the software class
// satisfy it, which is what lets a generic verifier take either.
//
// `checksum()` is in here rather than left as a convenience on the hardware
// side, because a one-shot over a whole buffer is what the call sites this
// facade exists for actually write (ota/image.hpp verifying a slot, the
// device_id example checking a blob). A concept that stops at the incremental
// three would have declared the two types interchangeable while the fallback
// was missing the method every example used.
template <class C>
concept Checksum32 = requires(C c, std::span<const std::uint8_t> b) {
    c.reset();
    c.update(b);
    { std::as_const(c).value() } -> std::same_as<std::uint32_t>;
    { c.checksum(b) } -> std::same_as<std::uint32_t>;
};

// The software implementation, named where a user of this header can find it.
// It is not a fallback in any pejorative sense — it is the definition of the
// answer, and the hardware is an accelerator for it.
using software = alloy::ota::crc::crc32;
static_assert(Checksum32<software>);

// A claimed hardware CRC unit. Stateless and zero-size: the state is the
// silicon's DR register, and there is exactly one of it.
template <class Inst>
class handle {
public:
    void reset() { hal::crc_impl<Inst>::reset(); }
    void update(std::span<const std::uint8_t> b) {
        hal::crc_impl<Inst>::update(b.data(), b.size());
    }
    void update(const std::uint8_t* p, std::size_t n) { hal::crc_impl<Inst>::update(p, n); }
    [[nodiscard]] std::uint32_t value() const { return hal::crc_impl<Inst>::value(); }

    // One-shot over a whole buffer, the shape ota/image.hpp's call sites want.
    [[nodiscard]] std::uint32_t checksum(std::span<const std::uint8_t> b) {
        reset();
        update(b);
        return value();
    }
};

// What `board::crc` is on a chip that has the block: a zero-size opener.
template <class Inst>
class engine {
    // The board that HAS the block, asserting the shape the board that does not
    // have it will be handed. Checked when `board::crc` is instantiated, which
    // is every build of every program that opens one — so the two halves of the
    // facade cannot drift apart on a chip nobody happened to compile for.
    static_assert(Checksum32<handle<Inst>>,
                  "the hardware handle must satisfy the same shape the software "
                  "fallback does, or board.hpp's substitution is a different API");

public:
    // EXCLUSIVE, and the block is the textbook case for it. There is one shift
    // register and one remainder; two owners interleaving update() calls do not
    // race on a flag, they arithmetically corrupt each other's answer and both
    // get a plausible 32-bit number back. That is hole (A) with a checksum
    // instead of a baud rate, and it is undetectable downstream — which is
    // precisely when a claim has to be a trap rather than a comment.
    //
    // There is no release, by the same design rule as every other alloy
    // facade: whoever opens the unit owns it for the life of the image. For a
    // CRC that reads harsher than it is — a bootloader that verifies a slot
    // and then jumps has left the image, and an application with two components
    // that both want checksums should pass one handle around, which is a
    // decision worth making explicitly.
    [[nodiscard]] handle<Inst> open() const {
        alloy::claim::exclusive<Inst, alloy::claim::personality::crc>();
        hal::crc_impl<Inst>::open();
        return {};
    }
};

// What `board::crc` is on a chip with no CRC block. `open()` returns the
// software implementation — the same methods and, provably, the same number.
struct software_engine {
    [[nodiscard]] software open() const { return {}; }
};

}  // namespace alloy::crc
