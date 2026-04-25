#pragma once

// Rs485DeStream: ByteStream decorator for RS-485 half-duplex DE pin control.
//
// Usage pattern (write then flush):
//   rs485.write(frame);   // DE asserted high before first byte is queued
//   rs485.flush(timeout); // waits for TC, then de-asserts DE low
//
// Lifetime: both Stream and Pin must outlive this decorator.

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/modbus/byte_stream.hpp"
#include "core/result.hpp"

namespace alloy::modbus {

// Concept for a DE GPIO pin. Any type with set_high() and set_low() satisfies it.
// Return values are ignored — DE is best-effort; stream errors are primary.
template <typename T>
concept DePin = requires(T& p) {
    p.set_high();
    p.set_low();
};

// Decorator: wraps an inner ByteStream and gates a DE pin around write + flush.
// Stores references (non-owning). Template params inferred via CTAD or explicit.
template <ByteStream Stream, DePin Pin>
class Rs485DeStream {
   public:
    Rs485DeStream(Stream& stream, Pin& de_pin) noexcept
        : stream_{stream}, de_{de_pin} {}

    Rs485DeStream(const Rs485DeStream&) = delete;
    Rs485DeStream& operator=(const Rs485DeStream&) = delete;

    // Read: pass through unchanged; DE is not relevant on RX.
    [[nodiscard]] core::Result<std::size_t, StreamError> read(
        std::span<std::byte> buf, std::uint32_t timeout_us) noexcept {
        return stream_.read(buf, timeout_us);
    }

    // Write: assert DE high, then forward. DE stays high until flush().
    [[nodiscard]] core::Result<void, StreamError> write(
        std::span<const std::byte> buf) noexcept {
        de_.set_high();
        return stream_.write(buf);
    }

    // Flush: wait for TX drain on inner stream, then de-assert DE.
    [[nodiscard]] core::Result<void, StreamError> flush(
        std::uint32_t timeout_us) noexcept {
        auto result = stream_.flush(timeout_us);
        de_.set_low();
        return result;
    }

    // wait_idle: pass through; line idle detection is on the inner stream.
    [[nodiscard]] core::Result<void, StreamError> wait_idle(
        std::uint32_t timeout_us) noexcept {
        return stream_.wait_idle(timeout_us);
    }

   private:
    Stream& stream_;
    Pin& de_;
};

}  // namespace alloy::modbus
