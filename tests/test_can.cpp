// CAN acceptance filters — the portable half.
//
// WHAT THIS FILE CAN AND CANNOT PROVE, stated first because the gap matters
// more than the tests. The host suite is board-free by construction: it
// compiles no generated device header and no per-IP driver, so the M_CAN word
// encoding in hal/can/st_fdcan_v1.hpp — which bit of a message-RAM word is
// SFT, which is SFID2 — is NOT exercised here and cannot be. What is exercised
// is everything above that seam: what a filter means, that the meaning is
// implementable by something which is not an FDCAN, and that the ids the
// example expects to survive its filter list actually do.
//
// The encoding is defended elsewhere and differently: the field positions come
// from alloy-devices rather than from the driver (so no bit number is written
// in src/), the controller half was cross-checked against ST's own CMSIS
// device header, and two static_asserts in the driver tie the companion's
// capacity to the controller's counting field. None of that is a test, and
// this comment is the honest place to say so.

#include <alloy/can.hpp>
#include <alloy/concepts.hpp>

#include <cstdint>

#include "alloy_test.hpp"
#include "doubles.hpp"

namespace {

using alloy::can::match;
using alloy::can::match_masked;

// ── what a filter MEANS ────────────────────────────────────────────────────
// A 1 in the mask must match, a 0 is don't-care. Everything below is a
// constant expression, so a regression here is a build failure, not a report.

// One exact identifier.
static_assert(match(0x123).matches(0x123));
static_assert(!match(0x123).matches(0x122));
static_assert(!match(0x123).matches(0x023));

// A 16-wide range: 0x200..0x20F, the second filter the can example installs.
static_assert(match_masked(0x200, 0x7F0).matches(0x200));
static_assert(match_masked(0x200, 0x7F0).matches(0x205));
static_assert(match_masked(0x200, 0x7F0).matches(0x20F));
static_assert(!match_masked(0x200, 0x7F0).matches(0x210));
static_assert(!match_masked(0x200, 0x7F0).matches(0x1FF));

// A zero mask is "every identifier", which is a filter list that accepts
// everything — a real configuration, and the reason accept_all() exists as a
// separate call rather than as `accept_only(match_masked(0, 0))`.
static_assert(match_masked(0, 0).matches(0x000));
static_assert(match_masked(0, 0).matches(0x7FF));

// match() is exactly "every bit of a standard identifier must match", and the
// width is the protocol's, not a chip's.
static_assert(match(0x7FF).mask == alloy::can::standard_id_mask);
static_assert(alloy::can::standard_id_mask == 0x7FFu);

// WHY the admission check exists, and it is not the reason its first draft
// gave. An identifier above the standard width does not produce a filter that
// matches nothing — it produces a filter that matches a DIFFERENT identifier.
// `match(0x800)` carries mask 0x7FF, so bit 11 is never compared and the
// filter is `match(0x000)` wearing another name. Silence here would be a port
// that receives the wrong traffic, which is worse than one that receives none,
// and this static_assert is the fact the diagnostic in core/admit.hpp had to
// be rewritten to state. (This is a claim about the value type, not a licence
// to build one: accept_only rejects it at compile time.)
static_assert(match(0x800).matches(0x000));
static_assert(match(0x800).id != match(0x000).id);  // …though it says otherwise

// ── the surface is implementable off-silicon ───────────────────────────────

ALLOY_TEST(can_double_accepts_everything_until_told_otherwise) {
    alloy::test::fake_can<> can;
    can.enable();
    for (std::uint32_t id : {0x123u, 0x205u, 0x321u}) {
        alloy::can_frame tx{};
        tx.id = id;
        tx.len = 1;
        ALLOY_CHECK(can.send(tx));
        alloy::can_frame rx{};
        ALLOY_CHECK(can.receive(rx));
        ALLOY_CHECK_EQ(rx.id, id);
    }
    ALLOY_CHECK_EQ(can.dropped, 0u);
}

// The exact scenario examples/can runs on the G0B1RE, in software: two
// filters, three identifiers, two survivors.
ALLOY_TEST(can_double_drops_what_the_filter_list_does_not_name) {
    alloy::test::fake_can<> can;
    can.enable();
    can.accept_only(match(0x123), match_masked(0x200, 0x7F0));

    const std::uint32_t ids[] = {0x123, 0x205, 0x321};
    const bool expected[] = {true, true, false};
    for (unsigned i = 0; i < 3u; ++i) {
        alloy::can_frame tx{};
        tx.id = ids[i];
        tx.len = 1;
        tx.data[0] = static_cast<std::uint8_t>(i);
        ALLOY_CHECK(can.send(tx));
        alloy::can_frame rx{};
        const bool got = can.receive(rx);
        ALLOY_CHECK_EQ(got, expected[i]);
        if (got) {
            ALLOY_CHECK_EQ(rx.id, ids[i]);
        }
    }
    ALLOY_CHECK_EQ(can.dropped, 1u);
}

ALLOY_TEST(can_accept_all_undoes_a_filter_list) {
    alloy::test::fake_can<> can;
    can.enable();
    can.accept_only(match(0x123));
    ALLOY_CHECK(!can.accepts(0x321));
    can.accept_all();
    ALLOY_CHECK(can.accepts(0x321));
}

// A shorter list REPLACES a longer one rather than merging with it — the
// property the driver gets from publishing a new list size, and the one a
// caller would be surprised to lose.
ALLOY_TEST(can_a_second_accept_only_replaces_the_first) {
    alloy::test::fake_can<> can;
    can.enable();
    can.accept_only(match(0x100), match(0x200), match(0x300));
    ALLOY_CHECK(can.accepts(0x300));
    can.accept_only(match(0x100));
    ALLOY_CHECK(can.accepts(0x100));
    ALLOY_CHECK(!can.accepts(0x200));
    ALLOY_CHECK(!can.accepts(0x300));
}

// ── the degree check ───────────────────────────────────────────────────────
// filters_fit is the mechanism that turns "more filters than this silicon
// holds" into a compile error naming both numbers. Its passing side is what a
// test can reach; the failing side is a build failure by design and is proven
// by the negative-control compiles recorded in the commit that added it.
static_assert(sizeof(alloy::can::detail::filters_fit<0, 28>) > 0);
static_assert(sizeof(alloy::can::detail::filters_fit<28, 28>) > 0);

// The no-CAN stand-in answers the capacity question with a number rather than
// an error, which is what lets one main.cpp compile for nine boards.
static_assert(alloy::can::null_controller::filter_capacity == 0u);

}  // namespace
