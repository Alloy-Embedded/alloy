// alloy::lib::bus bridge — the properties the two-board story stands on:
// a publish lands in the local subscriber AND leaves as a frame; a frame fed
// back in republishes locally WITHOUT echoing back out (any link); the ring
// takes frames whole or not at all; unknown ids and stale layouts are
// counted, never guessed at; the one TX task drains any number of routes
// through a single await.

#include "bus.hpp"

#include <cstdint>

#include "alloy/async/executor.hpp"
#include "alloy/async/task.hpp"
#include "alloy_test.hpp"

namespace bus = alloy::lib::bus;

namespace {

struct cmd {
    std::uint8_t op;
    std::uint16_t arg;
};

struct cmd_wire {
    using message = cmd;
    static constexpr std::uint16_t id = 0x0201;
    static constexpr std::uint8_t ver = 1;
    static constexpr std::size_t size = 3;
    static void encode(const cmd& m, std::uint8_t* out) noexcept {
        out[0] = m.op;
        alloy::byteorder::store_le16(&out[1], m.arg);
    }
    static cmd decode(const std::uint8_t* in) noexcept {
        return {in[0], alloy::byteorder::load_le16(&in[1])};
    }
};

// The stale peer: same id, bumped ver — a layout our route must refuse.
struct cmd_v2_wire {
    using message = cmd;
    static constexpr std::uint16_t id = 0x0201;
    static constexpr std::uint8_t ver = 2;
    static constexpr std::size_t size = 3;
    static void encode(const cmd& m, std::uint8_t* out) noexcept { cmd_wire::encode(m, out); }
    static cmd decode(const std::uint8_t* in) noexcept { return cmd_wire::decode(in); }
};

struct telem {
    std::uint32_t ticks;
};

struct telem_wire {
    using message = telem;
    static constexpr std::uint16_t id = 0x0202;
    static constexpr std::uint8_t ver = 1;
    static constexpr std::size_t size = 4;
    static void encode(const telem& m, std::uint8_t* out) noexcept {
        alloy::byteorder::store_le32(&out[0], m.ticks);
    }
    static telem decode(const std::uint8_t* in) noexcept {
        return {alloy::byteorder::load_le32(&in[0])};
    }
};

constexpr std::size_t cmd_frame_len =
    bus::wire_frame_overhead + bus::wire_msg_header + cmd_wire::size;  // 15

alloy::async::task drain_two(alloy::async::task_storage<384>&, bus::bridge_core& link,
                             int& taken) {
    std::uint8_t staging[bus::wire_max_frame];
    for (int i = 0; i < 2; ++i) {
        co_await link.tx_pending();
        if (!link.tx_take(staging).empty()) {
            ++taken;
        }
    }
}

}  // namespace

ALLOY_TEST(bus_bridge_publish_reaches_local_and_wire) {
    bus::bridge<256> link;
    bus::bridge_route<cmd_wire> route{link};
    bus::subscriber<cmd, 4> local;

    ALLOY_CHECK(bus::publish(cmd{7, 0x1234}));

    // Local subscriber saw it…
    cmd got{};
    ALLOY_CHECK(local.try_next(got));
    ALLOY_CHECK_EQ(got.op, 7u);

    // …and the same publish left a complete, decodable frame on the link.
    std::uint8_t staging[bus::wire_max_frame];
    const auto frame = link.tx_take(staging);
    ALLOY_CHECK_EQ(frame.size(), cmd_frame_len);

    bus::wire_receiver<> rx;
    bool done = false;
    for (const std::uint8_t b : frame) {
        done = rx.feed(b, 0);
    }
    ALLOY_CHECK(done);
    bus::msg_view v{};
    ALLOY_CHECK(bus::parse_datagram(rx.payload(), v));
    cmd wire_side{};
    ALLOY_CHECK(bus::decode_as<cmd_wire>(v, wire_side));
    ALLOY_CHECK_EQ(wire_side.arg, 0x1234u);

    ALLOY_CHECK(link.tx_take(staging).empty());  // exactly one frame
}

ALLOY_TEST(bus_bridge_republish_does_not_echo) {
    // Two links forwarding the same message type — the multi-link fan-out —
    // plus a local subscriber. A frame arriving on B must reach the local
    // subscriber and must NOT leave again through EITHER link.
    bus::bridge<256> link_a;
    bus::bridge<256> link_b;
    bus::bridge_route<cmd_wire> route_a{link_a};
    bus::bridge_route<cmd_wire> route_b{link_b};
    bus::subscriber<cmd, 4> local;

    ALLOY_CHECK(bus::publish(cmd{1, 100}));  // goes out both links
    std::uint8_t staging[bus::wire_max_frame];
    std::uint8_t frame_a[bus::wire_max_frame];
    const auto fa = link_a.tx_take(frame_a);
    ALLOY_CHECK(!fa.empty());
    ALLOY_CHECK(!link_b.tx_take(staging).empty());  // drain B's copy too
    cmd got{};
    ALLOY_CHECK(local.try_next(got));  // drain the local delivery

    // The wire hands A's frame to B (this is what the uart would do).
    ALLOY_CHECK_EQ(link_b.on_bytes(fa, 0), static_cast<std::size_t>(1));

    // Delivered locally once…
    ALLOY_CHECK(local.try_next(got));
    ALLOY_CHECK_EQ(got.arg, 100u);
    ALLOY_CHECK(!local.try_next(got));
    // …and no link re-forwards it: not B (its own), not A (single hop).
    ALLOY_CHECK(link_a.tx_empty());
    ALLOY_CHECK(link_b.tx_empty());
}

ALLOY_TEST(bus_bridge_ring_takes_frames_whole_or_not_at_all) {
    // cmd frame is 15 B + 1 length prefix = 16 B in the ring; 150 B holds 9.
    bus::bridge<150> link;
    bus::bridge_route<cmd_wire> route{link};

    int accepted = 0;
    for (int i = 0; i < 12; ++i) {
        if (bus::publish(cmd{static_cast<std::uint8_t>(i), 0})) {
            ++accepted;
        }
    }
    ALLOY_CHECK_EQ(accepted, 9);
    ALLOY_CHECK_EQ(link.tx_missed(), 3u);

    // Every queued frame comes out whole and in order.
    std::uint8_t staging[bus::wire_max_frame];
    bus::wire_receiver<> rx;
    int frames = 0;
    for (;;) {
        const auto f = link.tx_take(staging);
        if (f.empty()) {
            break;
        }
        bool done = false;
        for (const std::uint8_t b : f) {
            done = rx.feed(b, 0);
        }
        ALLOY_CHECK(done);
        ++frames;
    }
    ALLOY_CHECK_EQ(frames, 9);
    ALLOY_CHECK_EQ(rx.bad_frames(), 0u);
    ALLOY_CHECK_EQ(rx.lost(), 0u);  // 0..8 arrived gapless

    // Dropped frames consumed seq ON PURPOSE: the next frame that does get
    // through carries the gap, so the peer's lost() witnesses the 3 messages
    // this side never queued.
    ALLOY_CHECK(bus::publish(cmd{99, 99}));
    const auto f = link.tx_take(staging);
    bool done = false;
    for (const std::uint8_t b : f) {
        done = rx.feed(b, 0);
    }
    ALLOY_CHECK(done);
    ALLOY_CHECK_EQ(rx.lost(), 3u);
}

ALLOY_TEST(bus_bridge_counts_unknown_and_stale) {
    bus::bridge<256> link;
    bus::bridge_route<cmd_wire> route{link};
    bus::subscriber<cmd, 4> local;

    // A frame whose id no route on this link claims.
    std::uint8_t frame[bus::wire_max_frame];
    std::size_t n = bus::encode_datagram<telem_wire>(telem{42}, 0, frame);
    ALLOY_CHECK_EQ(link.on_bytes({frame, n}, 0), static_cast<std::size_t>(0));
    ALLOY_CHECK_EQ(link.rx_unknown(), 1u);

    // Same id, newer layout version: refused and counted, never misread.
    n = bus::encode_datagram<cmd_v2_wire>(cmd{9, 9}, 1, frame);
    ALLOY_CHECK_EQ(link.on_bytes({frame, n}, 0), static_cast<std::size_t>(0));
    ALLOY_CHECK_EQ(link.rx_dropped(), 1u);

    cmd got{};
    ALLOY_CHECK(!local.try_next(got));  // neither reached the local bus
    ALLOY_CHECK_EQ(link.rx_frames(), 2u);  // both frames were VALID wire
}

ALLOY_TEST(bus_bridge_one_tx_task_drains_all_routes) {
    alloy::async::executor<8> ex;
    alloy::async::task_storage<384> st;
    bus::bridge<256> link;
    bus::bridge_route<cmd_wire> cmds{link};
    bus::bridge_route<telem_wire> telems{link};
    int taken = 0;

    ex.spawn(drain_two(st, link, taken));
    ex.run_once();  // parks on tx_pending — nothing queued yet
    ALLOY_CHECK_EQ(taken, 0);

    // Two different topics, one link, one waiter.
    ALLOY_CHECK(bus::publish(cmd{1, 2}));
    ALLOY_CHECK(bus::publish(telem{3}));
    ex.run_once();
    ALLOY_CHECK_EQ(taken, 2);
    ALLOY_CHECK(!st.in_use);  // task completed and retired
    ALLOY_CHECK(link.tx_empty());
}

ALLOY_TEST(bus_bridge_route_dtor_unlinks_both_directions) {
    bus::bridge<256> link;
    {
        bus::bridge_route<cmd_wire> route{link};
        ALLOY_CHECK(bus::publish(cmd{1, 1}));
        std::uint8_t staging[bus::wire_max_frame];
        ALLOY_CHECK(!link.tx_take(staging).empty());
    }
    // Outbound: a publish no longer reaches this link.
    ALLOY_CHECK(bus::publish(cmd{2, 2}));
    ALLOY_CHECK(link.tx_empty());

    // Inbound: the id is unknown again.
    std::uint8_t frame[bus::wire_max_frame];
    const std::size_t n = bus::encode_datagram<cmd_wire>(cmd{3, 3}, 7, frame);
    ALLOY_CHECK_EQ(link.on_bytes({frame, n}, 0), static_cast<std::size_t>(0));
    ALLOY_CHECK_EQ(link.rx_unknown(), 1u);
}
