// The register model: a field must fit, address, and preserve exactly the
// register it names — at 8, 16 and 32 bits.
//
// Width used to be assumed. Every register was rw32, `field_t` computed its
// masks in `unsigned`, and a read-modify-write went through a 32-bit word. That
// is fine on the four architectures alloy builds for and wrong on the ones it
// wants to reach: Renesas RL78, AVR and MSP430 declare 8- and 16-bit SFRs, some
// of them explicitly rejecting wider access, and a 32-bit RMW on an 8-bit
// register writes the three registers beside it.
//
// The width is now DEDUCED from the pointer-to-member, so a field cannot
// disagree with its own register. These tests pin what that has to mean.

#include <alloy/core/mmio.hpp>

#include <cstdint>
#include <type_traits>

#include "alloy_test.hpp"

namespace {

// One block with a register of each width, laid out the way a generated IP
// header lays them out.
struct regs {
    alloy::rw8 B;
    alloy::rw16 H;
    alloy::rw32 W;
};

// ── the width comes from the register, not from the field ──────────────────
static_assert(std::is_same_v<alloy::field_t<&regs::B, 0>::word, std::uint8_t>);
static_assert(std::is_same_v<alloy::field_t<&regs::H, 0>::word, std::uint16_t>);
static_assert(std::is_same_v<alloy::field_t<&regs::W, 0>::word, std::uint32_t>);
static_assert(alloy::field_t<&regs::B, 0>::bits == 8);
static_assert(alloy::field_t<&regs::H, 0>::bits == 16);
static_assert(alloy::field_t<&regs::W, 0>::bits == 32);

// ── masks stay inside the register ─────────────────────────────────────────
static_assert(alloy::field_t<&regs::B, 7>::mask == 0x80u);
static_assert(alloy::field_t<&regs::B, 0, 8>::mask == 0xFFu);
static_assert(alloy::field_t<&regs::H, 15>::mask == 0x8000u);
static_assert(alloy::field_t<&regs::H, 0, 16>::mask == 0xFFFFu);
static_assert(alloy::field_t<&regs::H, 12, 4>::mask == 0xF000u);
// The 16-bit case that a signed promotion would get wrong: `uint16_t{1} << 15`
// promotes to int, and where int is 16 bits that shift is undefined.
static_assert(alloy::field_t<&regs::H, 15>::raw_mask == 1u);
// And the 32-bit behaviour is byte-for-byte what it was before widths existed.
static_assert(alloy::field_t<&regs::W, 31>::mask == 0x8000'0000u);
static_assert(alloy::field_t<&regs::W, 20, 4>::mask == 0x00F0'0000u);
static_assert(alloy::field_t<&regs::W, 0, 32>::raw_mask == 0xFFFF'FFFFu);

ALLOY_TEST(mmio_writes_only_the_field_it_names) {
    regs r{0xFF, 0xFFFF, 0xFFFF'FFFF};

    alloy::field_t<&regs::B, 4, 2>::write(r, 0b01u);
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.B), 0xDFu);       // bits 5:4 = 01
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.H), 0xFFFFu);     // neighbours untouched
    ALLOY_CHECK_EQ(static_cast<unsigned long>(r.W), 0xFFFF'FFFFul);

    alloy::field_t<&regs::H, 8, 4>::write(r, 0x5u);
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.H), 0xF5FFu);
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.B), 0xDFu);
}

// Aliased, because a comma inside a template argument list is a comma to the
// preprocessor and ALLOY_CHECK_EQ would see three arguments.
using byte_field = alloy::field_t<&regs::B, 5, 3>;
using half_field = alloy::field_t<&regs::H, 9, 5>;
using word_field = alloy::field_t<&regs::W, 27, 5>;

ALLOY_TEST(mmio_reads_back_what_it_wrote_at_every_width) {
    regs r{0, 0, 0};

    byte_field::write(r, 0b101u);
    ALLOY_CHECK_EQ(byte_field::read(r), 0b101u);

    half_field::write(r, 0b10110u);
    ALLOY_CHECK_EQ(half_field::read(r), 0b10110u);

    word_field::write(r, 0b11011u);
    ALLOY_CHECK_EQ(word_field::read(r), 0b11011u);
}

ALLOY_TEST(mmio_set_and_clear_leave_the_rest_alone) {
    regs r{0b0101'0101, 0xAAAA, 0x1234'5678};

    alloy::field_t<&regs::B, 1>::set(r);
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.B), 0b0101'0111u);
    alloy::field_t<&regs::B, 0>::clear(r);
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.B), 0b0101'0110u);

    alloy::field_t<&regs::H, 15>::set(r);
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.H), 0xAAAAu | 0x8000u);
    alloy::field_t<&regs::H, 15>::clear(r);
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.H), 0xAAAAu & ~0x8000u);

    ALLOY_CHECK_EQ(static_cast<unsigned long>(r.W), 0x1234'5678ul);
}

ALLOY_TEST(mmio_a_write_of_an_oversized_value_cannot_escape_the_field) {
    regs r{0, 0, 0};
    // A 3-bit field handed 0xFF must keep 3 bits and touch nothing else.
    alloy::field_t<&regs::B, 2, 3>::write(r, 0xFFu);
    ALLOY_CHECK_EQ(static_cast<unsigned>(r.B), 0b0001'1100u);
}

}  // namespace
