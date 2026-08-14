// alloy::lib::bus — the wire boundary. Local topics are zero-config; a
// message that CROSSES A WIRE to another board needs a declared, stable
// identity. This header is the sans-IO half of that boundary: the frame
// codec, the byte-at-a-time receive machine, and the WireBinding contract a
// message's encoder/decoder satisfies (hand-written in this phase; the
// bus.toml generator emits the identical shape later, so nothing migrates).
//
// Frame — the house length-prefixed convention, reused from the OTA
// transport byte for byte in spirit (SOF, no escaping, length before
// payload, CRC-32/ISO-HDLC, resync = hunt for SOF):
//
//   off  0     1      2     3..4         5 .. 5+len-1     last 4
//       0x7E  type   seq   len u16 LE    payload          crc32 LE
//                                        └ crc32 covers type..payload ┘
//
//   type 0x01 = bus datagram. Peers are symmetric — no master/slave
//   direction bit. Datagram payload: msg_id u16 LE | ver u8 | body, body
//   fields in declared order, little-endian, via alloy::byteorder — never
//   struct layout (padding and endianness are not a protocol).
//
// Semantics are AT-MOST-ONCE: no ack, no retry, no timers on the device
// (the OTA asymmetry, both ends). seq is a per-link TX counter; the
// receiver's lost() counts the gaps — a witness, not a repair. End-to-end
// reliability, where needed, belongs to an application protocol above.
//
// Sans-IO in the rtu_framer mold: this file owns no uart, no clock, no bus.
// Bytes are pushed via feed(b, now_us); the injected clock exists ONLY to
// abandon a stalled partial frame (tick) — delimitation itself is length-
// driven and works with a frozen clock, which is what keeps it provable
// under emulation. A frame that goes bad costs exactly itself: one
// bad_frames_ tick and a fresh SOF hunt, never the next frame.

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "alloy/ota/crc32.hpp"
#include "alloy/util/byteorder.hpp"

namespace alloy::lib::bus {

inline constexpr std::uint8_t wire_sof = 0x7E;
inline constexpr std::uint8_t wire_type_datagram = 0x01;

// Body cap: bounds the encode a bridge performs under the publish mask, and
// the RAM a receiver buffers. Slow-plane messages are small structs; a
// payload this size at 115200 baud is already ~12 ms of wire.
inline constexpr std::size_t wire_max_body = 128;
inline constexpr std::size_t wire_msg_header = 3;  // msg_id u16 LE + ver u8
inline constexpr std::size_t wire_frame_overhead = 9;
inline constexpr std::size_t wire_max_payload = wire_msg_header + wire_max_body;
inline constexpr std::size_t wire_max_frame = wire_frame_overhead + wire_max_payload;

// ID doctrine (the nvm-key rules, verbatim): explicit u16, never
// auto-assigned, never deleted, never reused. 0x0000 is refused as a
// sentinel; 0xFF00.. is reserved for framework frames (a future hello /
// manifest-hash exchange).
[[nodiscard]] constexpr bool wire_id_reserved(std::uint16_t id) {
    return id == 0 || id >= 0xFF00;
}

// The contract a message's wire form satisfies. Hand-written today,
// generator-emitted tomorrow — same shape either way:
//
//   struct temp_reading_wire {
//       using message = temp_reading;
//       static constexpr std::uint16_t id  = 0x0101;
//       static constexpr std::uint8_t  ver = 1;
//       static constexpr std::size_t  size = 3;
//       static void encode(const temp_reading&, std::uint8_t* out) noexcept;
//       static temp_reading decode(const std::uint8_t* in) noexcept;
//   };
//
// Same id => same layout, forever. A layout change is a NEW id (retire the
// old one); ver is a runtime guard against a stale peer, not a license to
// change the layout under an id.
template <class B>
concept WireBinding =
    std::is_trivially_copyable_v<typename B::message> &&
    requires(const typename B::message& m, std::uint8_t* out, const std::uint8_t* in) {
        { B::id } -> std::convertible_to<std::uint16_t>;
        { B::ver } -> std::convertible_to<std::uint8_t>;
        { B::size } -> std::convertible_to<std::size_t>;
        { B::encode(m, out) } -> std::same_as<void>;
        { B::decode(in) } -> std::same_as<typename B::message>;
    };

// Build one complete datagram frame into `out`. Returns the frame length,
// or 0 when `out` cannot hold it (limit before trusting — the caller's
// buffer is the caller's claim, not a fact).
template <WireBinding B>
std::size_t encode_datagram(const typename B::message& m, std::uint8_t seq,
                            std::span<std::uint8_t> out) noexcept {
    static_assert(B::size <= wire_max_body,
                  "wire body exceeds the slow-plane cap — split the message");
    static_assert(!wire_id_reserved(B::id),
                  "0x0000 and 0xFF00.. are not assignable message ids");
    constexpr std::size_t payload = wire_msg_header + B::size;
    constexpr std::size_t total = wire_frame_overhead + payload;
    if (out.size() < total) {
        return 0;
    }
    out[0] = wire_sof;
    out[1] = wire_type_datagram;
    out[2] = seq;
    byteorder::store_le16(&out[3], static_cast<std::uint16_t>(payload));
    byteorder::store_le16(&out[5], B::id);
    out[7] = B::ver;
    B::encode(m, &out[8]);
    // Table CRC deliberately: a bridge encodes THIS inside publish()'s irq
    // mask, and the bytewise loop there is not a size trade but deafness —
    // measured, it is a whole received byte at 230400 baud.
    const std::uint32_t c = ota::crc::crc32_table_of(out.subspan(1, 4 + payload));
    byteorder::store_le32(&out[5 + payload], c);
    return total;
}

// A parsed datagram payload: the id/ver header plus a borrowed body span.
struct msg_view {
    std::uint16_t id = 0;
    std::uint8_t ver = 0;
    std::span<const std::uint8_t> body{};
};

[[nodiscard]] inline bool parse_datagram(std::span<const std::uint8_t> payload,
                                         msg_view& out) noexcept {
    if (payload.size() < wire_msg_header) {
        return false;
    }
    out.id = byteorder::load_le16(payload.data());
    out.ver = payload[2];
    out.body = payload.subspan(wire_msg_header);
    return true;
}

// Decode a view as B's message. False when this view is not B's: wrong id,
// stale ver (a peer built from an older bus.toml), or a body whose size
// disagrees with the declared layout. The caller's dispatch counts which.
template <WireBinding B>
[[nodiscard]] bool decode_as(const msg_view& v, typename B::message& out) noexcept {
    if (v.id != B::id || v.ver != B::ver || v.body.size() != B::size) {
        return false;
    }
    out = B::decode(v.body.data());
    return true;
}

// Byte-at-a-time receive machine. feed() returns true when a complete,
// CRC-valid frame is ready; type()/seq()/payload() are then valid until the
// next feed() or tick(). Thread-context only, like every sans-IO core here.
//
// Witnesses, not repairs: frames() counts good frames, bad_frames() counts
// everything that started and died (CRC mismatch, oversize length, stall
// abandonment), lost() counts seq gaps once synced. A frame whose length
// field exceeds MaxPayload is dropped BEFORE buffering (the OTA rule).
template <std::size_t MaxPayload = wire_max_payload>
class wire_receiver {
    static_assert(MaxPayload >= wire_msg_header && MaxPayload <= 0xFFFF);

    enum class st : std::uint8_t { sof, type, seq, len_lo, len_hi, payload, crc0, crc1, crc2, crc3 };

public:
    // stall_us: how long a PARTIAL frame may sit before tick()/feed()
    // abandons it. Generous by default — it exists so a half frame from a
    // peer that died mid-send cannot wedge the machine, not to delimit.
    explicit wire_receiver(std::uint32_t stall_us = 50'000u) : stall_us_(stall_us) {}
    wire_receiver(const wire_receiver&) = delete;
    wire_receiver& operator=(const wire_receiver&) = delete;

    [[nodiscard]] bool feed(std::uint8_t b, std::uint32_t now_us) noexcept {
        abandon_if_stalled(now_us);
        last_byte_us_ = now_us;
        switch (state_) {
            case st::sof:
                if (b == wire_sof) {
                    state_ = st::type;
                }
                return false;  // hunting: garbage between frames is silent
            case st::type:
                type_ = b;
                crc_.reset();
                crc_.update(&b, 1);
                state_ = st::seq;
                return false;
            case st::seq:
                seq_ = b;
                crc_.update(&b, 1);
                state_ = st::len_lo;
                return false;
            case st::len_lo:
                len_ = b;
                crc_.update(&b, 1);
                state_ = st::len_hi;
                return false;
            case st::len_hi:
                len_ |= static_cast<std::uint16_t>(b) << 8;
                crc_.update(&b, 1);
                if (len_ > MaxPayload) {
                    ++bad_frames_;  // dropped before buffering
                    state_ = st::sof;
                } else {
                    fill_ = 0;
                    state_ = (len_ == 0) ? st::crc0 : st::payload;
                }
                return false;
            case st::payload:
                buf_[fill_++] = b;
                crc_.update(&b, 1);
                if (fill_ == len_) {
                    state_ = st::crc0;
                }
                return false;
            case st::crc0:
            case st::crc1:
            case st::crc2:
                rx_crc_ |= static_cast<std::uint32_t>(b)
                           << (8u * (static_cast<unsigned>(state_) - static_cast<unsigned>(st::crc0)));
                state_ = static_cast<st>(static_cast<std::uint8_t>(state_) + 1);
                return false;
            case st::crc3:
                rx_crc_ |= static_cast<std::uint32_t>(b) << 24u;
                state_ = st::sof;
                if (rx_crc_ != crc_.value()) {
                    rx_crc_ = 0;
                    ++bad_frames_;  // this frame paid; the next starts clean
                    return false;
                }
                rx_crc_ = 0;
                account_seq();
                ++frames_;
                return true;
        }
        return false;  // unreachable
    }

    // Abandon a stalled partial frame without consuming a byte. Call from a
    // periodic housekeeping point; feed() also applies it on arrival.
    void tick(std::uint32_t now_us) noexcept { abandon_if_stalled(now_us); }

    [[nodiscard]] std::uint8_t type() const noexcept { return type_; }
    [[nodiscard]] std::uint8_t seq() const noexcept { return seq_; }
    [[nodiscard]] std::span<const std::uint8_t> payload() const noexcept {
        return {buf_.data(), len_};
    }

    [[nodiscard]] std::uint32_t frames() const noexcept { return frames_; }
    [[nodiscard]] std::uint32_t bad_frames() const noexcept { return bad_frames_; }
    [[nodiscard]] std::uint32_t lost() const noexcept { return lost_; }

private:
    void abandon_if_stalled(std::uint32_t now_us) noexcept {
        // Wrap-safe unsigned delta, the rtu_framer discipline.
        if (state_ != st::sof && (now_us - last_byte_us_) > stall_us_) {
            state_ = st::sof;
            rx_crc_ = 0;
            ++bad_frames_;  // a half frame is evidence of loss, count it
        }
    }

    void account_seq() noexcept {
        if (synced_) {
            lost_ += static_cast<std::uint8_t>(seq_ - expected_);
        }
        expected_ = static_cast<std::uint8_t>(seq_ + 1u);
        synced_ = true;
    }

    std::array<std::uint8_t, MaxPayload> buf_{};
    // Same table as the encoder above: it is already in the image, so the
    // faster inner loop is free here, and a receive path that keeps up
    // matters on the same links that made the encoder's window matter.
    ota::crc::crc32_table crc_;
    std::uint32_t stall_us_;
    std::uint32_t last_byte_us_ = 0;
    std::uint32_t rx_crc_ = 0;
    std::uint32_t frames_ = 0;
    std::uint32_t bad_frames_ = 0;
    std::uint32_t lost_ = 0;
    std::uint16_t len_ = 0;
    std::uint16_t fill_ = 0;
    std::uint8_t type_ = 0;
    std::uint8_t seq_ = 0;
    std::uint8_t expected_ = 0;
    bool synced_ = false;
    st state_ = st::sof;
};

}  // namespace alloy::lib::bus
