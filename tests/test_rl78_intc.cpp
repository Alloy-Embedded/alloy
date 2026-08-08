// The RL78 interrupt-controller address map.
//
// A vector number picks a bank and a bit, and the bank layout is not uniform:
// banks 0 and 1 interleave inside one block while bank 2 sits somewhere else
// entirely. Getting that wrong masks the wrong source — silently, because every
// address in the range is a real register that accepts the write.
//
// This is pure arithmetic over architectural constants, so it runs on the host
// even though nothing else in arch/rl78 can. The addresses come from the RL78
// architecture (the same in every part), not from any chip file.

#include <alloy/arch/rl78/intc.hpp>

#include <cstdint>

#include "alloy_test.hpp"

namespace {

using namespace alloy::arch::rl78;

// ── the near-address mapping ───────────────────────────────────────────────
//
// uintptr_t is 16 bits on this architecture and an SFR address is 20. near()
// keeps the manual's number in the source and returns what the CPU takes; the
// mapping is only valid because the window covers 0xF0000..0xFFFFF.
static_assert(near(0xFFFE0) == 0xFFE0);
static_assert(near(0xFFFAB) == 0xFFAB);
static_assert(near(0xF0000) == 0x0000);

// ── which register holds which vector ──────────────────────────────────────
// bank 0: vectors 0..15
static_assert(reg_of(plane::request, 0) == near(0xFFFE0));
static_assert(reg_of(plane::mask, 15) == near(0xFFFE4));
static_assert(reg_of(plane::prio_low, 0) == near(0xFFFE8));
static_assert(reg_of(plane::prio_high, 0) == near(0xFFFEC));
// bank 1: two bytes along, INTERLEAVED with bank 0 rather than after it
static_assert(reg_of(plane::request, 16) == near(0xFFFE2));
static_assert(reg_of(plane::mask, 31) == near(0xFFFE6));
static_assert(reg_of(plane::prio_low, 16) == near(0xFFFEA));
static_assert(reg_of(plane::prio_high, 16) == near(0xFFFEE));
// bank 2: a different block entirely — the one a stride-based guess gets wrong
static_assert(reg_of(plane::request, 32) == near(0xFFFD0));
static_assert(reg_of(plane::mask, 47) == near(0xFFFD4));
static_assert(reg_of(plane::prio_low, 40) == near(0xFFFD8));
static_assert(reg_of(plane::prio_high, 40) == near(0xFFFDC));

// A stride-based layout would put bank 2's mask at 0xFFFE8 — which is PR00,
// the priority register. Masking a line would change its priority instead.
static_assert(reg_of(plane::mask, 32) != reg_of(plane::prio_low, 0));

// ── which bit inside it ────────────────────────────────────────────────────
static_assert(bit_of(0) == 0x0001);
static_assert(bit_of(15) == 0x8000);
static_assert(bit_of(16) == 0x0001);  // wraps at the bank boundary
static_assert(bit_of(40) == 0x0100);

ALLOY_TEST(rl78_every_vector_lands_in_exactly_one_place) {
    // No two vectors may share a (register, bit) pair: that would be two
    // sources masked by one write.
    for (unsigned a = 0; a < kMaxVector; ++a) {
        for (unsigned b = a + 1; b < kMaxVector; ++b) {
            const bool same_reg = reg_of(plane::mask, a) == reg_of(plane::mask, b);
            const bool same_bit = bit_of(a) == bit_of(b);
            ALLOY_CHECK(!(same_reg && same_bit));
        }
    }
}

ALLOY_TEST(rl78_the_four_planes_never_alias) {
    // Request, mask and the two priority bits must be four distinct registers
    // for every vector — an overlap means enabling a line also reprioritises it.
    for (unsigned n = 0; n < kMaxVector; ++n) {
        const auto req = reg_of(plane::request, n);
        const auto msk = reg_of(plane::mask, n);
        const auto p0 = reg_of(plane::prio_low, n);
        const auto p1 = reg_of(plane::prio_high, n);
        ALLOY_CHECK(req != msk && req != p0 && req != p1);
        ALLOY_CHECK(msk != p0 && msk != p1);
        ALLOY_CHECK(p0 != p1);
    }
}

ALLOY_TEST(rl78_a_vector_past_the_last_bank_is_not_silently_folded) {
    // kMaxVector is the contract; irq_line_enable() checks it. What must not
    // happen is a wrap that lands on a real register belonging to vector 0.
    ALLOY_CHECK_EQ(kMaxVector, 48u);
    ALLOY_CHECK_EQ(bit_of(48), bit_of(0));  // the wrap the bounds check exists for
}

}  // namespace
