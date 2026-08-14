// alloy::lib::bus::watch — latest-value semantics: get() sees the NEWEST
// write (a depth-1 queue would hand back the oldest — the exact confusion
// this cell exists to avoid), false before the first write, and watch_route
// feeds a cell from a topic alongside queued subscribers.

#include "bus.hpp"

#include <cstdint>

#include "alloy_test.hpp"

namespace {

struct level {
    std::uint32_t pct;
};

}  // namespace

ALLOY_TEST(bus_watch_is_false_until_first_set) {
    alloy::lib::bus::watch<level> w;
    level out{};
    ALLOY_CHECK(!w.get(out));
    ALLOY_CHECK_EQ(w.updates(), 0u);
}

ALLOY_TEST(bus_watch_returns_newest_value) {
    alloy::lib::bus::watch<level> w;
    w.set(level{10});
    w.set(level{20});
    w.set(level{30});

    level out{};
    ALLOY_CHECK(w.get(out));
    ALLOY_CHECK_EQ(out.pct, 30u);  // newest, not oldest
    ALLOY_CHECK_EQ(w.updates(), 3u);

    // get() does not consume: the cell still answers.
    ALLOY_CHECK(w.get(out));
    ALLOY_CHECK_EQ(out.pct, 30u);
}

ALLOY_TEST(bus_watch_route_feeds_cell_from_topic) {
    alloy::lib::bus::watch<level> w;
    alloy::lib::bus::subscriber<level, 2> queued;
    {
        alloy::lib::bus::watch_route<level> route{w};

        ALLOY_CHECK(alloy::lib::bus::publish(level{40}));
        ALLOY_CHECK(alloy::lib::bus::publish(level{55}));
        // A full CELL never drops — it overwrites. Only the queue can fill:
        // depth 2, so a third publish reports the queued subscriber's drop
        // while the cell still tracks the newest value.
        ALLOY_CHECK(!alloy::lib::bus::publish(level{70}));

        level out{};
        ALLOY_CHECK(w.get(out));
        ALLOY_CHECK_EQ(out.pct, 70u);
        ALLOY_CHECK_EQ(w.updates(), 3u);
        ALLOY_CHECK_EQ(queued.missed(), 1u);
    }
    // Route destroyed -> unlinked: a publish still reaches the queued
    // subscriber (drained first, so it has room) but no longer the cell.
    level out{};
    ALLOY_CHECK(queued.try_next(out));
    ALLOY_CHECK(queued.try_next(out));
    ALLOY_CHECK(alloy::lib::bus::publish(level{99}));
    ALLOY_CHECK(queued.try_next(out));
    ALLOY_CHECK_EQ(out.pct, 99u);
    ALLOY_CHECK(w.get(out));
    ALLOY_CHECK_EQ(out.pct, 70u);
    ALLOY_CHECK_EQ(w.updates(), 3u);
}
