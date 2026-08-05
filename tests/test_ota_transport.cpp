// Host tests for the UART firmware-update transport (alloy/ota/uart_transport):
// the sans-IO receiver is driven byte-by-byte exactly as a bootloader's read
// loop would, with a recording sink and a captured reply stream — the whole
// protocol proves out with no uart, no flash and no hardware.
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "alloy/ota/error.hpp"
#include "alloy/ota/uart_transport.hpp"
#include "alloy/util/result.hpp"
#include "alloy_test.hpp"

namespace {

using namespace alloy::ota;
using alloy::Result;

// ── Fakes ──────────────────────────────────────────────────────────────────

struct fake_sink {
    using rvoid = Result<void, ota_error>;
    std::vector<std::uint8_t> written;
    int begins = 0, finishes = 0;
    ota_error fail_write_with = static_cast<ota_error>(0);
    ota_error fail_finish_with = static_cast<ota_error>(0);

    rvoid begin() {
        ++begins;
        written.clear();
        return {};
    }
    rvoid write(std::span<const std::uint8_t> chunk) {
        if (static_cast<int>(fail_write_with) != 0) { return fail_write_with; }
        written.insert(written.end(), chunk.begin(), chunk.end());
        return {};
    }
    rvoid finish() {
        ++finishes;
        if (static_cast<int>(fail_finish_with) != 0) { return fail_finish_with; }
        return {};
    }
};

struct harness {
    fake_sink sink;
    std::vector<std::uint8_t> replies;
    uart_receiver<fake_sink, void (*)(std::span<const std::uint8_t>)> rx;

    static inline std::vector<std::uint8_t>* reply_sink = nullptr;
    static void capture(std::span<const std::uint8_t> f) {
        reply_sink->insert(reply_sink->end(), f.begin(), f.end());
    }

    explicit harness(session_info info = {.target_slot = 1, .running_version = 7})
        : rx(sink, &capture, info) {
        reply_sink = &replies;
    }

    void feed(std::span<const std::uint8_t> bytes) {
        for (auto b : bytes) { rx.on_byte(b); }
    }
    void feed_frame(frame_type t, std::uint8_t seq, std::span<const std::uint8_t> payload) {
        std::uint8_t buf[9 + max_chunk];
        feed({buf, encode_frame(t, seq, payload, buf)});
    }
    // Pop the single reply accumulated since the last call; asserts exactly one.
    std::vector<std::uint8_t> take_reply() {
        auto r = replies;
        replies.clear();
        return r;
    }
};

frame_type reply_type(const std::vector<std::uint8_t>& f) {
    return static_cast<frame_type>(f.at(1));
}

// ── Tests ──────────────────────────────────────────────────────────────────

ALLOY_TEST(transport_hello_answers_info_and_starts_session) {
    harness h;
    h.feed_frame(frame_type::hello, 0, {});
    auto r = h.take_reply();
    ALLOY_CHECK_EQ(h.sink.begins, 1);
    ALLOY_CHECK(reply_type(r) == frame_type::info);
    // INFO payload: proto=1, slot=1, version=7 (LE), max_chunk (LE)
    ALLOY_CHECK_EQ(r.at(5), 1u);
    ALLOY_CHECK_EQ(r.at(6), 1u);
    ALLOY_CHECK_EQ(r.at(7), 7u);
    ALLOY_CHECK_EQ(r.at(11) | (r.at(12) << 8), max_chunk);
}

ALLOY_TEST(transport_streams_data_in_order_and_acks) {
    harness h;
    h.feed_frame(frame_type::hello, 0, {});
    (void)h.take_reply();
    const std::uint8_t a[3] = {1, 2, 3}, b[2] = {4, 5};
    h.feed_frame(frame_type::data, 0, a);
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::ack);
    h.feed_frame(frame_type::data, 1, b);
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::ack);
    ALLOY_CHECK_EQ(h.sink.written.size(), 5u);
    ALLOY_CHECK_EQ(h.sink.written[3], 4u);
}

ALLOY_TEST(transport_retransmitted_frame_acked_but_not_rewritten) {
    harness h;
    h.feed_frame(frame_type::hello, 0, {});
    (void)h.take_reply();
    const std::uint8_t a[2] = {9, 9};
    h.feed_frame(frame_type::data, 0, a);
    (void)h.take_reply();
    h.feed_frame(frame_type::data, 0, a);  // host missed the ACK and resent
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::ack);
    ALLOY_CHECK_EQ(h.sink.written.size(), 2u);  // NOT doubled
}

ALLOY_TEST(transport_out_of_order_seq_naks) {
    harness h;
    h.feed_frame(frame_type::hello, 0, {});
    (void)h.take_reply();
    const std::uint8_t a[1] = {1};
    h.feed_frame(frame_type::data, 3, a);  // expected 0
    auto r = h.take_reply();
    ALLOY_CHECK(reply_type(r) == frame_type::nak);
    ALLOY_CHECK_EQ(h.sink.written.size(), 0u);
}

ALLOY_TEST(transport_corrupt_crc_naks_then_recovers) {
    harness h;
    h.feed_frame(frame_type::hello, 0, {});
    (void)h.take_reply();
    const std::uint8_t a[2] = {7, 8};
    std::uint8_t buf[9 + max_chunk];
    const auto n = encode_frame(frame_type::data, 0, a, buf);
    buf[n - 1] ^= 0xFF;  // corrupt the CRC
    h.feed({buf, n});
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::nak);
    h.feed_frame(frame_type::data, 0, a);  // clean retransmit succeeds
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::ack);
    ALLOY_CHECK_EQ(h.sink.written.size(), 2u);
}

ALLOY_TEST(transport_oversized_len_dropped_without_overflow) {
    harness h;
    // Hand-build a header claiming max_chunk+1 payload bytes.
    const std::uint16_t bad = max_chunk + 1;
    const std::uint8_t hdr[5] = {frame_sof, 0x02, 0,
                                 static_cast<std::uint8_t>(bad & 0xFF),
                                 static_cast<std::uint8_t>(bad >> 8)};
    h.feed(hdr);
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::nak);
}

ALLOY_TEST(transport_finish_reports_sink_error_and_success) {
    harness h;
    h.feed_frame(frame_type::hello, 0, {});
    (void)h.take_reply();
    h.sink.fail_finish_with = ota_error::bad_magic;
    h.feed_frame(frame_type::finish, 0, {});
    auto r = h.take_reply();
    ALLOY_CHECK(reply_type(r) == frame_type::nak);
    ALLOY_CHECK_EQ(r.at(5), static_cast<std::uint8_t>(ota_error::bad_magic));
    ALLOY_CHECK(!h.rx.finished());

    h.sink.fail_finish_with = static_cast<ota_error>(0);
    h.feed_frame(frame_type::finish, 0, {});
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::ack);
    ALLOY_CHECK(h.rx.finished());
}

ALLOY_TEST(transport_new_hello_restarts_a_wedged_session) {
    harness h;
    h.feed_frame(frame_type::hello, 0, {});
    (void)h.take_reply();
    const std::uint8_t a[1] = {1};
    h.feed_frame(frame_type::data, 0, a);
    (void)h.take_reply();
    // Host power-blipped and starts over: HELLO again -> fresh session, seq 0.
    h.feed_frame(frame_type::hello, 0, {});
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::info);
    ALLOY_CHECK_EQ(h.sink.begins, 2);
    h.feed_frame(frame_type::data, 0, a);
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::ack);
    ALLOY_CHECK_EQ(h.sink.written.size(), 1u);  // begin() cleared the first byte
}

ALLOY_TEST(transport_torn_frame_reset_recovers) {
    harness h;
    const std::uint8_t torn[3] = {frame_sof, 0x02, 0};  // header only, line went idle
    h.feed(torn);
    h.rx.reset_frame();  // bootloader idle timeout
    h.feed_frame(frame_type::hello, 0, {});
    ALLOY_CHECK(reply_type(h.take_reply()) == frame_type::info);
}

}  // namespace
