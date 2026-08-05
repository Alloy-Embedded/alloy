// Serial (UART) firmware-update transport — the field-update path for finished
// products: `alloy update --port ...` on the host streams a [header|payload]
// image into the inactive slot through this receiver.
//
// Sans-IO by design: the receiver owns NO uart, NO clock and NO flash — bytes
// are pushed in via on_byte(), replies come out through a Tx callable, and image
// bytes flow into a Sink (alloy::ota::updater satisfies it directly). That keeps
// it 100% host-testable and reusable from a bootloader OR a running app.
//
// Wire format (all little-endian), stop-and-wait, every frame CRC-32 guarded:
//
//   SOF 0x7E | type u8 | seq u8 | len u16 | payload[len] | crc32 u32
//                        └── crc32 covers type..payload ──┘
//
//   host -> device: HELLO (empty)          device -> host: INFO
//                   DATA  (image bytes)                    ACK / NAK
//                   FINISH (empty)                         ACK / NAK
//
//   INFO payload: proto u8 (=1) | target_slot u8 | running_version u32 | max_chunk u16
//   NAK payload:  error u8 (ota_error value; 0 = framing/sequence error)
//
// Retransmit policy is the HOST's (resend on NAK or timeout). The device is a
// pure responder: duplicate DATA seq (a retransmit after a lost ACK) is ACKed
// again WITHOUT re-writing; anything else out of order is NAKed. A new HELLO at
// any point restarts the session (host retry after power blip mid-transfer).
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/ota/crc32.hpp"
#include "alloy/ota/error.hpp"

namespace alloy::ota {

inline constexpr std::uint8_t frame_sof = 0x7E;

enum class frame_type : std::uint8_t {
    hello = 0x01,
    data = 0x02,
    finish = 0x03,
    info = 0x81,
    ack = 0x82,
    nak = 0x83,
};

// Largest DATA payload the receiver buffers (reported to the host via INFO).
inline constexpr std::uint16_t max_chunk = 256;

// Build one frame into `out` (which must hold 9 + payload.size() bytes);
// returns the encoded length. Shared by device replies and the host tests.
inline std::size_t encode_frame(frame_type type, std::uint8_t seq,
                                std::span<const std::uint8_t> payload,
                                std::uint8_t* out) {
    out[0] = frame_sof;
    out[1] = static_cast<std::uint8_t>(type);
    out[2] = seq;
    out[3] = static_cast<std::uint8_t>(payload.size() & 0xFF);
    out[4] = static_cast<std::uint8_t>(payload.size() >> 8);
    for (std::size_t i = 0; i < payload.size(); ++i) {
        out[5 + i] = payload[i];
    }
    const std::uint32_t c = crc::crc32_of({out + 1, 4 + payload.size()});
    std::uint8_t* p = out + 5 + payload.size();
    p[0] = static_cast<std::uint8_t>(c & 0xFF);
    p[1] = static_cast<std::uint8_t>((c >> 8) & 0xFF);
    p[2] = static_cast<std::uint8_t>((c >> 16) & 0xFF);
    p[3] = static_cast<std::uint8_t>((c >> 24) & 0xFF);
    return 9 + payload.size();
}

// What the receiver streams image bytes into. alloy::ota::updater satisfies
// this as-is (finish() returning Result<image_header, ...> converts to bool and
// carries .error()).
template <class S>
concept UpdateSink = requires(S s, std::span<const std::uint8_t> chunk) {
    { static_cast<bool>(s.begin()) };
    { s.begin().error() } -> std::convertible_to<ota_error>;
    { static_cast<bool>(s.write(chunk)) };
    { static_cast<bool>(s.finish()) };
};

// Session facts the INFO reply advertises (plain values — the transport stays
// board-free; the caller reads them from alloy/slots.hpp + its own header).
struct session_info {
    std::uint8_t target_slot;      // 0 = A, 1 = B: where DATA bytes will land
    std::uint32_t running_version; // image_version of the firmware answering
};

// The device-side responder. Tx is any callable taking span<const uint8_t>
// (typically a lambda writing to the uart handle).
template <UpdateSink Sink, class Tx>
class uart_receiver {
public:
    constexpr uart_receiver(Sink& sink, Tx tx, session_info info)
        : sink_(sink), tx_(tx), info_(info) {}

    // True once a FINISH was accepted — the caller (bootloader) then arms the
    // trial boot and resets; the transport itself never touches boot state.
    [[nodiscard]] bool finished() const { return finished_; }

    // Push one received byte through the frame parser; replies are emitted
    // through tx_ as complete frames. Call from the read loop (or an ISR ring).
    void on_byte(std::uint8_t b) {
        switch (state_) {
        case st::sof:
            if (b == frame_sof) { state_ = st::type; }
            return;
        case st::type:
            type_ = b;
            state_ = st::seq;
            return;
        case st::seq:
            seq_ = b;
            state_ = st::len0;
            return;
        case st::len0:
            len_ = b;
            state_ = st::len1;
            return;
        case st::len1:
            len_ |= static_cast<std::uint16_t>(b) << 8;
            if (len_ > max_chunk) {  // hostile/corrupt length: drop before buffering
                nak(0);
                state_ = st::sof;
                return;
            }
            got_ = 0;
            state_ = len_ == 0 ? st::crc0 : st::payload;
            return;
        case st::payload:
            buf_[got_++] = b;
            if (got_ == len_) { state_ = st::crc0; }
            return;
        case st::crc0: crc_ = b; state_ = st::crc1; return;
        case st::crc1: crc_ |= static_cast<std::uint32_t>(b) << 8; state_ = st::crc2; return;
        case st::crc2: crc_ |= static_cast<std::uint32_t>(b) << 16; state_ = st::crc3; return;
        case st::crc3:
            crc_ |= static_cast<std::uint32_t>(b) << 24;
            state_ = st::sof;
            dispatch();
            return;
        }
    }

    // Drop any half-parsed frame (caller-side idle timeout): the parser can
    // never wedge on a torn frame — the next SOF starts clean.
    void reset_frame() { state_ = st::sof; }

private:
    enum class st : std::uint8_t { sof, type, seq, len0, len1, payload, crc0, crc1, crc2, crc3 };

    void dispatch() {
        // Verify the frame CRC over type|seq|len|payload, reconstructed exactly
        // as encode_frame laid it out.
        std::uint8_t hdr[4] = {type_, seq_,
                               static_cast<std::uint8_t>(len_ & 0xFF),
                               static_cast<std::uint8_t>(len_ >> 8)};
        auto c = crc::crc32{};
        c.update({hdr, 4});
        c.update({buf_, len_});
        if (c.value() != crc_) {
            nak(0);
            return;
        }
        switch (static_cast<frame_type>(type_)) {
        case frame_type::hello: {
            // (Re)start the session: erase the target slot, advertise facts.
            if (auto r = sink_.begin(); !r) {
                nak(static_cast<std::uint8_t>(r.error()));
                return;
            }
            expected_ = 0;
            finished_ = false;
            std::uint8_t p[8];
            p[0] = 1;  // protocol version
            p[1] = info_.target_slot;
            p[2] = static_cast<std::uint8_t>(info_.running_version & 0xFF);
            p[3] = static_cast<std::uint8_t>((info_.running_version >> 8) & 0xFF);
            p[4] = static_cast<std::uint8_t>((info_.running_version >> 16) & 0xFF);
            p[5] = static_cast<std::uint8_t>((info_.running_version >> 24) & 0xFF);
            p[6] = static_cast<std::uint8_t>(max_chunk & 0xFF);
            p[7] = static_cast<std::uint8_t>(max_chunk >> 8);
            reply(frame_type::info, 0, {p, 8});
            return;
        }
        case frame_type::data: {
            if (seq_ == static_cast<std::uint8_t>(expected_ - 1u)) {
                reply(frame_type::ack, seq_, {});  // retransmit of an ACKed frame
                return;
            }
            if (seq_ != expected_) {
                nak(0);
                return;
            }
            if (auto r = sink_.write({buf_, len_}); !r) {
                nak(static_cast<std::uint8_t>(r.error()));
                return;
            }
            reply(frame_type::ack, seq_, {});
            ++expected_;
            return;
        }
        case frame_type::finish: {
            if (auto r = sink_.finish(); !r) {
                nak(static_cast<std::uint8_t>(r.error()));
                return;
            }
            finished_ = true;
            reply(frame_type::ack, seq_, {});
            return;
        }
        default:
            nak(0);  // a reply type (or garbage) sent AT the device
            return;
        }
    }

    void reply(frame_type t, std::uint8_t seq, std::span<const std::uint8_t> payload) {
        std::uint8_t out[9 + 8];  // replies carry at most the 8-byte INFO
        const std::size_t n = encode_frame(t, seq, payload, out);
        tx_({out, n});
    }

    void nak(std::uint8_t err) {
        const std::uint8_t p[1] = {err};
        reply(frame_type::nak, 0, {p, 1});
    }

    Sink& sink_;
    Tx tx_;
    session_info info_;
    st state_{st::sof};
    std::uint8_t type_{}, seq_{}, expected_{};
    std::uint16_t len_{}, got_{};
    std::uint32_t crc_{};
    bool finished_{};
    std::uint8_t buf_[max_chunk];
};

}  // namespace alloy::ota
