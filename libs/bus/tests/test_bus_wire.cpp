// alloy::lib::bus wire layer — the properties the bridge will stand on:
// encode->feed round-trips every v1 field type; a 0x7E inside a body is
// legal (length-driven delimitation, no escaping); garbage and corruption
// cost exactly the frame they hit; a stalled half frame is abandoned by the
// injected clock, never by delimitation; seq gaps are counted, not repaired.

#include "bus/wire.hpp"

#include <bit>
#include <cstdint>
#include <cstring>

#include "alloy_test.hpp"

namespace bus = alloy::lib::bus;

namespace {

// Every scalar type the bus.toml v1 schema will admit, in one message.
struct kitchen {
    std::uint8_t a;
    std::int8_t b;
    std::uint16_t c;
    std::int16_t d;
    std::uint32_t e;
    std::int32_t f;
    float g;
    bool h;
};

struct kitchen_wire {
    using message = kitchen;
    static constexpr std::uint16_t id = 0x0101;
    static constexpr std::uint8_t ver = 1;
    static constexpr std::size_t size = 19;
    static void encode(const kitchen& m, std::uint8_t* out) noexcept {
        namespace bo = alloy::byteorder;
        out[0] = m.a;
        out[1] = static_cast<std::uint8_t>(m.b);
        bo::store_le16(&out[2], m.c);
        bo::store_le16(&out[4], static_cast<std::uint16_t>(m.d));
        bo::store_le32(&out[6], m.e);
        bo::store_le32(&out[10], static_cast<std::uint32_t>(m.f));
        bo::store_le32(&out[14], std::bit_cast<std::uint32_t>(m.g));
        out[18] = m.h ? 1 : 0;
    }
    static kitchen decode(const std::uint8_t* in) noexcept {
        namespace bo = alloy::byteorder;
        kitchen m{};
        m.a = in[0];
        m.b = static_cast<std::int8_t>(in[1]);
        m.c = bo::load_le16(&in[2]);
        m.d = static_cast<std::int16_t>(bo::load_le16(&in[4]));
        m.e = bo::load_le32(&in[6]);
        m.f = static_cast<std::int32_t>(bo::load_le32(&in[10]));
        m.g = std::bit_cast<float>(bo::load_le32(&in[14]));
        m.h = in[18] != 0;
        return m;
    }
};

// Same layout under a different identity — the cross-decode refusal case.
struct other_wire {
    using message = kitchen;
    static constexpr std::uint16_t id = 0x0102;
    static constexpr std::uint8_t ver = 1;
    static constexpr std::size_t size = 19;
    static void encode(const kitchen& m, std::uint8_t* out) noexcept {
        kitchen_wire::encode(m, out);
    }
    static kitchen decode(const std::uint8_t* in) noexcept { return kitchen_wire::decode(in); }
};

constexpr kitchen sample{0x7E, -5, 0x7E7E, -1234, 0x11223344u, -56789, 3.25f, true};

// Feed a whole buffer; return how many complete frames came out, leaving the
// last frame's view in the receiver's accessors.
template <std::size_t N>
int feed_all(bus::wire_receiver<N>& rx, const std::uint8_t* p, std::size_t n,
             std::uint32_t now_us) {
    int frames = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (rx.feed(p[i], now_us)) {
            ++frames;
        }
    }
    return frames;
}

}  // namespace

ALLOY_TEST(bus_wire_round_trips_every_field_type) {
    std::uint8_t frame[64];
    const std::size_t n = bus::encode_datagram<kitchen_wire>(sample, 7, frame);
    ALLOY_CHECK_EQ(n, bus::wire_frame_overhead + bus::wire_msg_header + kitchen_wire::size);

    bus::wire_receiver<> rx;
    ALLOY_CHECK_EQ(feed_all(rx, frame, n, 0), 1);
    ALLOY_CHECK_EQ(rx.type(), bus::wire_type_datagram);
    ALLOY_CHECK_EQ(rx.seq(), 7u);

    bus::msg_view v{};
    ALLOY_CHECK(bus::parse_datagram(rx.payload(), v));
    ALLOY_CHECK_EQ(v.id, kitchen_wire::id);
    ALLOY_CHECK_EQ(v.ver, kitchen_wire::ver);

    kitchen out{};
    ALLOY_CHECK(bus::decode_as<kitchen_wire>(v, out));
    ALLOY_CHECK_EQ(out.a, sample.a);
    ALLOY_CHECK_EQ(out.b, sample.b);
    ALLOY_CHECK_EQ(out.c, sample.c);  // contains 0x7E7E — SOF inside a body is legal
    ALLOY_CHECK_EQ(out.d, sample.d);
    ALLOY_CHECK_EQ(out.e, sample.e);
    ALLOY_CHECK_EQ(out.f, sample.f);
    ALLOY_CHECK(out.g == sample.g);
    ALLOY_CHECK_EQ(out.h, sample.h);

    // The wrong binding refuses the view instead of misreading it.
    ALLOY_CHECK(!bus::decode_as<other_wire>(v, out));
}

ALLOY_TEST(bus_wire_encode_refuses_a_short_buffer) {
    std::uint8_t tiny[8];
    ALLOY_CHECK_EQ(bus::encode_datagram<kitchen_wire>(sample, 0, tiny),
                   static_cast<std::size_t>(0));
}

ALLOY_TEST(bus_wire_resyncs_after_garbage) {
    // Garbage that includes a stray SOF opening a bogus frame with an absurd
    // length: dropped before buffering, then the real frame parses whole.
    std::uint8_t frame[64];
    const std::size_t n = bus::encode_datagram<kitchen_wire>(sample, 0, frame);

    bus::wire_receiver<> rx;
    const std::uint8_t junk[] = {0x00, 0x7E, 0x01, 0x00, 0xFF, 0xFF, 0x55, 0xAA};
    ALLOY_CHECK_EQ(feed_all(rx, junk, sizeof junk, 0), 0);
    ALLOY_CHECK_EQ(rx.bad_frames(), 1u);  // the bogus oversize frame, counted

    ALLOY_CHECK_EQ(feed_all(rx, frame, n, 0), 1);
    ALLOY_CHECK_EQ(rx.frames(), 1u);
}

ALLOY_TEST(bus_wire_bad_crc_costs_exactly_one_frame) {
    std::uint8_t frame[64];
    const std::size_t n = bus::encode_datagram<kitchen_wire>(sample, 0, frame);

    std::uint8_t corrupt[64];
    std::memcpy(corrupt, frame, n);
    corrupt[10] ^= 0x40;  // one body bit lies

    bus::wire_receiver<> rx;
    ALLOY_CHECK_EQ(feed_all(rx, corrupt, n, 0), 0);
    ALLOY_CHECK_EQ(rx.bad_frames(), 1u);

    ALLOY_CHECK_EQ(feed_all(rx, frame, n, 0), 1);  // the NEXT frame is untouched
    ALLOY_CHECK_EQ(rx.frames(), 1u);
    ALLOY_CHECK_EQ(rx.bad_frames(), 1u);
}

ALLOY_TEST(bus_wire_stalled_partial_frame_is_abandoned_by_tick) {
    std::uint8_t frame[64];
    const std::size_t n = bus::encode_datagram<kitchen_wire>(sample, 3, frame);

    bus::wire_receiver<> rx{50'000u};
    ALLOY_CHECK_EQ(feed_all(rx, frame, n / 2, 1'000u), 0);  // half a frame, then silence
    rx.tick(10'000u);                                       // within the stall window
    ALLOY_CHECK_EQ(rx.bad_frames(), 0u);                    // still waiting, not abandoned
    rx.tick(60'000u);                                       // past it
    ALLOY_CHECK_EQ(rx.bad_frames(), 1u);

    // The machine is hunting again: the same full frame now parses.
    ALLOY_CHECK_EQ(feed_all(rx, frame, n, 70'000u), 1);
    ALLOY_CHECK_EQ(rx.frames(), 1u);
}

ALLOY_TEST(bus_wire_seq_gaps_are_counted_once_synced) {
    std::uint8_t f0[64];
    std::uint8_t f1[64];
    std::uint8_t f3[64];
    const std::size_t n0 = bus::encode_datagram<kitchen_wire>(sample, 0, f0);
    const std::size_t n1 = bus::encode_datagram<kitchen_wire>(sample, 1, f1);
    const std::size_t n3 = bus::encode_datagram<kitchen_wire>(sample, 3, f3);

    bus::wire_receiver<> rx;
    ALLOY_CHECK_EQ(feed_all(rx, f0, n0, 0), 1);
    ALLOY_CHECK_EQ(rx.lost(), 0u);  // first frame syncs, it cannot be a gap
    ALLOY_CHECK_EQ(feed_all(rx, f1, n1, 0), 1);
    ALLOY_CHECK_EQ(rx.lost(), 0u);
    ALLOY_CHECK_EQ(feed_all(rx, f3, n3, 0), 1);  // 2 went missing on the wire
    ALLOY_CHECK_EQ(rx.lost(), 1u);
    ALLOY_CHECK_EQ(rx.frames(), 3u);
}

ALLOY_TEST(bus_wire_survives_fuzz_and_recovers) {
    // Deterministic LCG spray: the machine must neither wedge nor overrun
    // (ASan is the witness), and after a stall abandon it parses cleanly.
    bus::wire_receiver<> rx{50'000u};
    std::uint32_t lcg = 0x1234'5678u;  // contract-ok: fuzz seed, not an address
    for (int i = 0; i < 20'000; ++i) {
        lcg = lcg * 1664525u + 1013904223u;
        (void)rx.feed(static_cast<std::uint8_t>(lcg >> 24), 100u);
    }
    rx.tick(200'000u);  // abandon whatever half-frame the spray left behind

    std::uint8_t frame[64];
    const std::size_t n = bus::encode_datagram<kitchen_wire>(sample, 9, frame);
    ALLOY_CHECK_EQ(feed_all(rx, frame, n, 200'100u), 1);

    bus::msg_view v{};
    ALLOY_CHECK(bus::parse_datagram(rx.payload(), v));
    kitchen out{};
    ALLOY_CHECK(bus::decode_as<kitchen_wire>(v, out));
    ALLOY_CHECK_EQ(out.e, sample.e);
}
