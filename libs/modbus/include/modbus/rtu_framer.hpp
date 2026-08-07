// RTU frame assembler — sans-IO in the alloy::ota::uart_receiver shape: owns
// NO uart and NO clock. Bytes are pushed via feed(b, now_us), idle time is
// reported via tick(now_us), complete CRC-valid frames come out as a borrowed
// span. 100% host-testable against a virtual clock.
//
// TWO framing rules run together, and the combination is the design:
//
//  1. LENGTH PREDICTION (primary): for the eight core function codes the ADU
//     length is computable from the first few bytes (pdu.hpp's
//     expected_adu_length), so a frame closes the instant its last byte
//     arrives — no clock involved, which is what makes the protocol provable
//     under emulation where sub-millisecond gaps don't exist.
//  2. t3.5 SILENCE (resynchronizer): any function code — including vendor
//     extensions the table doesn't know — closes when the bus goes quiet, and
//     EVERY error path (CRC mismatch, absurd claimed length, overflow) parks
//     the framer in discard mode that only a t3.5 gap exits. Silence is the
//     one signal a misaligned byte stream cannot fake.
//
// The two failure modes this shape exists to prevent, both reproduced against
// the reference C library: a frame claiming more bytes than the buffer wedged
// its engine PERMANENTLY (rx_length pinned at capacity, every later valid
// frame refused), and a torn/partial frame desynchronized its framer with no
// recovery path. Here both land in discard mode and cost exactly one t3.5.
//
// Concurrency contract: feed/tick/frame/consume all run in ONE context (the
// poll loop). Feeding from an ISR requires the deferred ISR-timestamp path —
// see the library README's support claim.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "modbus/crc.hpp"
#include "modbus/pdu.hpp"
#include "modbus/rtu_timing.hpp"

namespace alloy::lib::modbus {

// Cap is the largest ADU accepted; the spec maximum is also the default. A
// server that only ever answers small reads may shrink its footprint — the
// floor is the largest fixed-size ADU (8) plus nothing else.
template <std::size_t Cap = max_adu>
class rtu_framer {
    static_assert(Cap >= 8 && Cap <= max_adu,
                  "rtu_framer capacity must fit at least a fixed-size ADU and "
                  "never exceed the spec's 256-byte RTU ADU");

public:
    constexpr rtu_framer(direction dir, rtu_times t) : t_(t), dir_(dir) {}

    // Push one received byte stamped with its arrival time (alloy::uptime_us
    // or a test clock; wrap-safe deltas). Never blocks, never allocates.
    void feed(std::uint8_t b, std::uint32_t now_us) {
        const bool boundary = seen_any_ && gap_elapsed(now_us);
        last_rx_ = now_us;
        seen_any_ = true;

        if (boundary) {
            on_silence();
        }
        if (ready_) {
            // Half-duplex says nothing may arrive while a frame awaits its
            // consumer; a byte here is noise or a bus collision. Drop it and
            // resync so a torn stream can't be misparsed as a frame.
            ++drops_;
            discarding_ = true;
            return;
        }
        if (discarding_) {
            return;  // counting silence from last_rx_; bytes just extend it
        }
        if (len_ == Cap) {
            ++drops_;
            discarding_ = true;
            len_ = 0;
            return;
        }
        buf_[len_++] = b;

        const std::size_t expected = expected_adu_length(dir_, {buf_, len_});
        if (expected == 0 || expected == length_unknown) {
            return;  // need more bytes / silence will close it
        }
        if (expected > Cap) {
            // The claimed length can never fit: resync NOW instead of
            // accumulating toward a wedge (the reference library's bug).
            ++drops_;
            discarding_ = true;
            len_ = 0;
            return;
        }
        if (len_ == expected) {
            close_frame();
        }
    }

    // Report idle time from the poll loop: closes a silence-framed frame and
    // exits discard mode once the bus has been quiet for t3.5.
    void tick(std::uint32_t now_us) {
        if (!seen_any_ || !gap_elapsed(now_us)) {
            return;
        }
        on_silence();
    }

    // A complete, CRC-valid ADU is waiting.
    [[nodiscard]] bool has_frame() const { return ready_; }

    // The frame as [unit | pdu...] — CRC already verified and stripped.
    // Borrowed from the assembly buffer: valid until consume() or reset().
    [[nodiscard]] std::span<const std::uint8_t> frame() const {
        return {buf_, ready_ ? len_ - 2u : 0u};
    }

    void consume() {
        ready_ = false;
        len_ = 0;
    }

    void reset() {
        ready_ = false;
        discarding_ = false;
        seen_any_ = false;
        len_ = 0;
    }

    // Frames/bytes abandoned on CRC failure, overflow, oversize claim or
    // arrival-while-held — a diagnostics counter, not an error channel (the
    // spec's answer to a bad frame is exactly nothing).
    [[nodiscard]] std::uint32_t drops() const { return drops_; }

private:
    [[nodiscard]] bool gap_elapsed(std::uint32_t now_us) const {
        return now_us - last_rx_ >= t_.t3_5_us;  // unsigned: wrap-safe
    }

    // A t3.5 boundary passed: whatever was pending is decided here, and the
    // silence ITSELF is the realignment proof — discard mode always ends at a
    // boundary, so a bad frame costs exactly one t3.5, never the next frame.
    void on_silence() {
        if (!ready_ && !discarding_ && len_ != 0u) {
            close_frame();  // silence-framed (unknown-length) delivery
        }
        discarding_ = false;
        if (!ready_) {
            len_ = 0;
        }
    }

    // Frame complete by either rule: gate on minimum length + CRC. On failure
    // the stream may be misaligned mid-frame, so park in discard mode — only
    // proven silence gets the framer talking again.
    void close_frame() {
        if (len_ >= 4u && crc_ok({buf_, len_})) {
            ready_ = true;
            return;
        }
        ++drops_;
        discarding_ = true;
        len_ = 0;
    }

    rtu_times t_;
    std::uint32_t last_rx_{0};
    std::uint32_t drops_{0};
    std::size_t len_{0};
    direction dir_;
    bool seen_any_{false};
    bool discarding_{false};
    bool ready_{false};
    std::uint8_t buf_[Cap];
};

}  // namespace alloy::lib::modbus
