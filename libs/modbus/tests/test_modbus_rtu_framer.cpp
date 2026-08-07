// RTU framer: the regression suite for the two wire-reachable bugs
// reproduced against the reference C library (permanent wedge on an
// oversized length claim; desync on a torn frame), plus the properties the
// dual-rule design promises — length-predicted close without a clock,
// silence-framed close for unknown FCs, discard mode that always ends at one
// t3.5, and wrap-safe delta math across the 2^32 µs boundary.

#include "modbus/rtu_framer.hpp"

#include <cstdint>
#include <span>

#include "alloy_test.hpp"
#include "modbus/crc.hpp"
#include "modbus/pdu.hpp"
#include "modbus/rtu_timing.hpp"
#include "testkit/mock_wire.hpp"

namespace {
using namespace alloy::lib::modbus;
using alloy::testkit::virtual_clock;

constexpr rtu_times k19200 = rtu_times_for(19'200);  // t3.5 = 2005 µs
constexpr std::uint32_t kCharGap = 573;              // one char time, < t1.5

// Feed a whole ADU byte-by-byte with realistic inter-character gaps.
template <std::size_t Cap>
void feed_all(rtu_framer<Cap>& f, virtual_clock& clk,
              std::span<const std::uint8_t> adu) {
    for (std::uint8_t b : adu) {
        clk.advance_us(kCharGap);
        f.feed(b, clk.now_us);
    }
}

// The spec's canonical request: unit 0x11, read 3 holding regs @ 0x006B.
constexpr std::uint8_t kSpecRequest[8] = {0x11, 0x03, 0x00, 0x6B,
                                          0x00, 0x03, 0x76, 0x87};
}  // namespace

ALLOY_TEST(modbus_framer_closes_a_predicted_frame_without_any_tick) {
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;
    feed_all(f, clk, kSpecRequest);  // no tick() anywhere
    ALLOY_CHECK(f.has_frame());
    const auto frame = f.frame();
    ALLOY_CHECK_EQ(frame.size(), 6u);  // CRC stripped
    ALLOY_CHECK_EQ(frame[0], 0x11u);
    ALLOY_CHECK_EQ(frame[1], 0x03u);
}

ALLOY_TEST(modbus_framer_oversized_claim_resyncs_in_one_t35_not_forever) {
    // THE wedge regression: FC16 claiming 0xFF data bytes (ADU would be 264 >
    // capacity). The reference library pinned its buffer at capacity and
    // refused every later valid frame permanently. Here: discard until t3.5,
    // then the next valid frame goes through.
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;

    const std::uint8_t wedge[7] = {0x11, 0x10, 0x00, 0x00, 0x00, 0x7F, 0xFF};
    feed_all(f, clk, wedge);
    ALLOY_CHECK(!f.has_frame());
    ALLOY_CHECK_EQ(f.drops(), 1u);

    // Garbage keeps streaming — still quiet, still no wedge.
    const std::uint8_t junk[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    feed_all(f, clk, junk);
    ALLOY_CHECK(!f.has_frame());

    // One inter-frame silence, then the valid request must be accepted.
    clk.advance_us(k19200.t3_5_us);
    f.tick(clk.now_us);
    feed_all(f, clk, kSpecRequest);
    ALLOY_CHECK(f.has_frame());
}

ALLOY_TEST(modbus_framer_back_to_back_frames_both_deliver_after_consume) {
    // The consume() contract: re-arms immediately, no silence required —
    // the reference library's reset dropped the tail (its pipelining loss).
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;

    feed_all(f, clk, kSpecRequest);
    ALLOY_CHECK(f.has_frame());
    f.consume();

    feed_all(f, clk, kSpecRequest);  // gaps stay below t3.5 throughout
    ALLOY_CHECK(f.has_frame());
    ALLOY_CHECK_EQ(f.drops(), 0u);
}

ALLOY_TEST(modbus_framer_fc04_response_reaches_the_parser) {
    // FC04 was unreachable over RTU in the reference library (its length
    // table refused the function). Core here: response 11 04 02 AA BB + CRC.
    rtu_framer f{direction::response, k19200};
    virtual_clock clk;

    std::uint8_t adu[7] = {0x11, 0x04, 0x02, 0xAA, 0xBB, 0, 0};
    append_crc(adu, 5);
    feed_all(f, clk, adu);
    ALLOY_CHECK(f.has_frame());

    std::uint16_t regs[1] = {};
    const auto r = parse_read_registers_response(f.frame().subspan(1),
                                                 function::read_input_registers, regs);
    ALLOY_CHECK(r.has_value());
    ALLOY_CHECK_EQ(regs[0], 0xAABBu);
}

ALLOY_TEST(modbus_framer_crc_corruption_drops_then_recovers_after_silence) {
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;

    std::uint8_t bad[8];
    for (std::size_t i = 0; i < 8; ++i) {
        bad[i] = kSpecRequest[i];
    }
    bad[3] ^= 0x40u;  // payload corrupted, CRC now wrong
    feed_all(f, clk, bad);
    ALLOY_CHECK(!f.has_frame());  // spec: a bad frame gets NO reaction
    ALLOY_CHECK_EQ(f.drops(), 1u);

    clk.advance_us(k19200.t3_5_us);
    f.tick(clk.now_us);
    feed_all(f, clk, kSpecRequest);
    ALLOY_CHECK(f.has_frame());
}

ALLOY_TEST(modbus_framer_unknown_fc_closes_by_silence_with_crc_gate) {
    // Vendor FC 0x41 — no length rule exists, so only the t3.5 rule can
    // close it. This is what keeps the framer generic beyond the core eight.
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;

    std::uint8_t adu[6] = {0x11, 0x41, 0xCA, 0xFE, 0, 0};
    append_crc(adu, 4);
    feed_all(f, clk, adu);
    ALLOY_CHECK(!f.has_frame());  // nothing predicted, still assembling

    clk.advance_us(k19200.t3_5_us);
    f.tick(clk.now_us);
    ALLOY_CHECK(f.has_frame());
    ALLOY_CHECK_EQ(f.frame().size(), 4u);
    ALLOY_CHECK_EQ(f.frame()[1], 0x41u);
}

ALLOY_TEST(modbus_framer_torn_frame_flushes_at_the_boundary_byte) {
    // A frame that dies mid-air (5 of 8 bytes), then silence, then a fresh
    // frame whose FIRST BYTE arrives before any tick() ran: the boundary
    // must be detected inside feed() itself.
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;

    feed_all(f, clk, std::span{kSpecRequest}.first(5));
    ALLOY_CHECK(!f.has_frame());

    clk.advance_us(k19200.t3_5_us);  // silence — but NO tick call
    feed_all(f, clk, kSpecRequest);
    ALLOY_CHECK(f.has_frame());
    ALLOY_CHECK_EQ(f.frame().size(), 6u);
}

ALLOY_TEST(modbus_framer_noise_shorter_than_a_frame_is_dropped_silently) {
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;

    const std::uint8_t blip[2] = {0x11, 0x03};  // dies after two bytes
    feed_all(f, clk, blip);
    clk.advance_us(k19200.t3_5_us);
    f.tick(clk.now_us);
    ALLOY_CHECK(!f.has_frame());
    ALLOY_CHECK_EQ(f.drops(), 1u);

    feed_all(f, clk, kSpecRequest);  // and the line is immediately usable
    ALLOY_CHECK(f.has_frame());
}

ALLOY_TEST(modbus_framer_byte_arriving_while_a_frame_is_held_is_refused) {
    // Half-duplex: nothing may arrive while the consumer holds a frame. A
    // byte that does is noise/collision — drop it, resync, and only after
    // consume + silence does the next frame go through.
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;

    feed_all(f, clk, kSpecRequest);
    ALLOY_CHECK(f.has_frame());

    clk.advance_us(kCharGap);
    f.feed(0x55, clk.now_us);   // collision while held
    ALLOY_CHECK(f.has_frame());  // the held frame is untouched
    ALLOY_CHECK_EQ(f.frame().size(), 6u);
    ALLOY_CHECK_EQ(f.drops(), 1u);

    f.consume();
    clk.advance_us(k19200.t3_5_us);
    f.tick(clk.now_us);
    feed_all(f, clk, kSpecRequest);
    ALLOY_CHECK(f.has_frame());
}

ALLOY_TEST(modbus_framer_delta_math_survives_the_u32_wrap) {
    // Same property test_uptime_us pins for the clock, pinned here for the
    // consumer: a frame fed across the 2^32 µs boundary is one frame.
    rtu_framer f{direction::request, k19200};
    virtual_clock clk;
    clk.now_us = 0xFFFF'FFFFu - 2u * kCharGap;  // wraps mid-frame

    feed_all(f, clk, kSpecRequest);
    ALLOY_CHECK(f.has_frame());
    ALLOY_CHECK_EQ(f.drops(), 0u);
}

ALLOY_TEST(modbus_framer_overflow_of_a_small_capacity_resyncs_cleanly) {
    // A shrunken framer (Cap=8) hit with a silence-framed vendor frame
    // longer than its buffer: overflow → discard → silence → usable again.
    rtu_framer<8> f{direction::request, k19200};
    virtual_clock clk;

    std::uint8_t big[10] = {0x11, 0x41, 1, 2, 3, 4, 5, 6, 7, 8};
    feed_all(f, clk, big);
    ALLOY_CHECK(!f.has_frame());
    ALLOY_CHECK_EQ(f.drops(), 1u);

    clk.advance_us(k19200.t3_5_us);
    f.tick(clk.now_us);
    feed_all(f, clk, kSpecRequest);
    ALLOY_CHECK(f.has_frame());
}
