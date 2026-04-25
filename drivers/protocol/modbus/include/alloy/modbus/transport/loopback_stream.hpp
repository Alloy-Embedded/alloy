#pragma once

// LoopbackPair: in-memory paired byte_stream for host tests.
//
// Holds two ring buffers. Exposes two endpoint views (a() and b()) that are
// wired back-to-back: bytes written via a() appear on b()'s read side, and
// vice versa. Both endpoints are non-owning views into the pair's buffers so
// the pair must outlive any endpoint that refers to it.
//
// Usage:
//   LoopbackPair<> pair;
//   auto master_side = pair.a();
//   auto slave_side  = pair.b();
//   // master_side.write({...}) → slave_side.read() receives the data

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/modbus/byte_stream.hpp"
#include "core/result.hpp"

namespace alloy::modbus {

// Single-direction power-of-two ring buffer.
template <std::size_t Cap>
class RingBuffer {
    static_assert((Cap & (Cap - 1u)) == 0u, "Cap must be a power of two");

   public:
    [[nodiscard]] std::size_t available() const noexcept { return write_ - read_; }
    [[nodiscard]] std::size_t free_space() const noexcept { return Cap - available(); }
    [[nodiscard]] bool empty() const noexcept { return write_ == read_; }

    std::size_t push(std::span<const std::byte> src) noexcept {
        const std::size_t n = std::min(src.size(), free_space());
        for (std::size_t i = 0u; i < n; ++i) {
            buf_[write_++ & kMask] = src[i];
        }
        return n;
    }

    std::size_t pop(std::span<std::byte> dst) noexcept {
        const std::size_t n = std::min(dst.size(), available());
        for (std::size_t i = 0u; i < n; ++i) {
            dst[i] = buf_[read_++ & kMask];
        }
        return n;
    }

   private:
    static constexpr std::size_t kMask = Cap - 1u;
    std::array<std::byte, Cap> buf_{};
    std::size_t write_{0u};
    std::size_t read_{0u};
};

// Non-owning endpoint view into a LoopbackPair's ring buffers.
// Satisfies the ByteStream concept. Lightweight (two pointers).
template <std::size_t Cap>
class LoopbackEndpoint {
   public:
    core::Result<std::size_t, StreamError> read(std::span<std::byte> buf,
                                                 std::uint32_t /*timeout_us*/) noexcept {
        return core::Ok(rx_->pop(buf));
    }

    core::Result<void, StreamError> write(std::span<const std::byte> buf) noexcept {
        const std::size_t pushed = tx_->push(buf);
        if (pushed < buf.size()) {
            return core::Err(StreamError::Overrun);
        }
        return core::Ok();
    }

    core::Result<void, StreamError> flush(std::uint32_t /*timeout_us*/) noexcept {
        return core::Ok();
    }

    core::Result<void, StreamError> wait_idle(std::uint32_t /*timeout_us*/) noexcept {
        return core::Ok();
    }

   private:
    template <std::size_t C>
    friend class LoopbackPair;

    LoopbackEndpoint(RingBuffer<Cap>* rx, RingBuffer<Cap>* tx) noexcept
        : rx_{rx}, tx_{tx} {}

    RingBuffer<Cap>* rx_;
    RingBuffer<Cap>* tx_;
};

// Owns the two ring buffers. Non-copyable, non-movable (endpoints hold raw pointers).
// Default capacity: 512 bytes per direction (fits one max RTU frame + overhead).
template <std::size_t Cap = 512u>
class LoopbackPair {
   public:
    using Endpoint = LoopbackEndpoint<Cap>;

    LoopbackPair() = default;
    LoopbackPair(const LoopbackPair&) = delete;
    LoopbackPair& operator=(const LoopbackPair&) = delete;
    LoopbackPair(LoopbackPair&&) = delete;
    LoopbackPair& operator=(LoopbackPair&&) = delete;

    // Endpoint a(): reads from b_to_a_, writes into a_to_b_.
    [[nodiscard]] Endpoint a() noexcept { return {&b_to_a_, &a_to_b_}; }
    // Endpoint b(): reads from a_to_b_, writes into b_to_a_.
    [[nodiscard]] Endpoint b() noexcept { return {&a_to_b_, &b_to_a_}; }

   private:
    RingBuffer<Cap> a_to_b_{};
    RingBuffer<Cap> b_to_a_{};
};

static_assert(ByteStream<LoopbackEndpoint<512u>>);

}  // namespace alloy::modbus
