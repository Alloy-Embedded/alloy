// alloy::lib::bus — topic fan-out and lifetime tests. The properties that
// carry the design: one publish reaches every live subscriber; a destroyed
// subscriber is unlinked (a later publish must not touch its memory — ASan
// is the witness on host); publish's return tells the truth about drops.

#include "bus.hpp"

#include <cstdint>

#include "alloy_test.hpp"

namespace {

struct tick {
    std::uint32_t n;
};

}  // namespace

ALLOY_TEST(bus_publish_with_no_subscribers_is_ok) {
    // An unobserved event is not an error — true, nothing delivered, no walk.
    ALLOY_CHECK(alloy::lib::bus::publish(tick{1}));
}

ALLOY_TEST(bus_publish_fans_out_to_three_subscribers) {
    alloy::lib::bus::subscriber<tick, 4> a;
    alloy::lib::bus::subscriber<tick, 4> b;
    alloy::lib::bus::subscriber<tick, 4> c;

    ALLOY_CHECK(alloy::lib::bus::publish(tick{7}));

    tick out{};
    ALLOY_CHECK(a.try_next(out));
    ALLOY_CHECK_EQ(out.n, 7u);
    ALLOY_CHECK(b.try_next(out));
    ALLOY_CHECK_EQ(out.n, 7u);
    ALLOY_CHECK(c.try_next(out));
    ALLOY_CHECK_EQ(out.n, 7u);

    // Exactly one message each — not one total, not duplicates.
    ALLOY_CHECK(!a.try_next(out));
    ALLOY_CHECK(!b.try_next(out));
    ALLOY_CHECK(!c.try_next(out));
}

ALLOY_TEST(bus_destroyed_subscriber_is_unlinked) {
    alloy::lib::bus::subscriber<tick, 4> stays;
    {
        alloy::lib::bus::subscriber<tick, 4> gone;
        ALLOY_CHECK(alloy::lib::bus::publish(tick{1}));
        tick out{};
        ALLOY_CHECK(gone.try_next(out));
    }
    // The dead node must be off the list: this publish walks only `stays`.
    // Under ASan a stale link would light up as a use-after-scope here.
    ALLOY_CHECK(alloy::lib::bus::publish(tick{2}));

    tick out{};
    ALLOY_CHECK(stays.try_next(out));
    ALLOY_CHECK_EQ(out.n, 1u);
    ALLOY_CHECK(stays.try_next(out));
    ALLOY_CHECK_EQ(out.n, 2u);
    ALLOY_CHECK(!stays.try_next(out));
}

ALLOY_TEST(bus_publish_reports_a_full_subscriber) {
    alloy::lib::bus::subscriber<tick, 2> narrow;
    alloy::lib::bus::subscriber<tick, 4> wide;

    ALLOY_CHECK(alloy::lib::bus::publish(tick{1}));
    ALLOY_CHECK(alloy::lib::bus::publish(tick{2}));
    // narrow is now full; wide still has room. The publish must deliver to
    // wide AND return false (someone dropped).
    ALLOY_CHECK(!alloy::lib::bus::publish(tick{3}));

    ALLOY_CHECK_EQ(narrow.missed(), 1u);
    ALLOY_CHECK_EQ(wide.missed(), 0u);

    tick out{};
    ALLOY_CHECK(wide.try_next(out));
    ALLOY_CHECK_EQ(out.n, 1u);
    ALLOY_CHECK(wide.try_next(out));
    ALLOY_CHECK_EQ(out.n, 2u);
    ALLOY_CHECK(wide.try_next(out));
    ALLOY_CHECK_EQ(out.n, 3u);  // wide got what narrow dropped
}

ALLOY_TEST(bus_distinct_types_are_distinct_topics) {
    struct other {
        std::uint32_t n;
    };
    alloy::lib::bus::subscriber<tick, 4> ticks;
    alloy::lib::bus::subscriber<other, 4> others;

    ALLOY_CHECK(alloy::lib::bus::publish(other{9}));

    tick t{};
    ALLOY_CHECK(!ticks.try_next(t));  // a tick subscriber never sees `other`
    other o{};
    ALLOY_CHECK(others.try_next(o));
    ALLOY_CHECK_EQ(o.n, 9u);
}
