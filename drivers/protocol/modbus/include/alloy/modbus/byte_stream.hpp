#pragma once

// byte_stream: transport waist between the RTU framer and the UART/TCP backend.
//
// Any concrete transport (UART, loopback, TCP socket) provides this interface.
// The framer only calls read/write/flush/wait_idle; it does not know what
// physical bus it is talking to.
//
// All methods return core::Result<T, StreamError> so errors propagate without
// exceptions. `timeout_us == 0` means return immediately if no data is ready.

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/result.hpp"

namespace alloy::modbus {

// No-op critical section RAII guard. Used as the default CritSection template
// parameter for Slave and Master. Replace with a platform critical section
// (e.g. disable/enable IRQ) for multi-threaded / ISR-shared access.
namespace detail {
struct NoOpCriticalSection {
    NoOpCriticalSection() noexcept = default;
};
}  // namespace detail

enum class StreamError : std::uint8_t {
    Timeout,       // operation did not complete within the given timeout
    Overrun,       // receive buffer overrun (bytes were lost)
    HardwareError, // framing error, parity error, etc.
    Closed,        // stream was closed or is not open
};

// Concept for a byte_stream backend. Every concrete stream must satisfy this.
// The static interface (non-virtual) lets the compiler inline UART writes on
// embedded targets while loopback is used on host for testing.
template <typename T>
concept ByteStream = requires(T& s, std::span<std::byte> rw_buf,
                              std::span<const std::byte> ro_buf,
                              std::uint32_t timeout_us) {
    // Receive up to buf.size() bytes. Returns the number of bytes actually read.
    { s.read(rw_buf, timeout_us) } -> std::same_as<core::Result<std::size_t, StreamError>>;
    // Transmit all bytes in buf. Blocks until all bytes are queued in the TX FIFO.
    { s.write(ro_buf) } -> std::same_as<core::Result<void, StreamError>>;
    // Wait for the physical TX shift register to become empty (TC / drain).
    { s.flush(timeout_us) } -> std::same_as<core::Result<void, StreamError>>;
    // Wait until the line has been idle for at least `silence_us` microseconds.
    { s.wait_idle(timeout_us) } -> std::same_as<core::Result<void, StreamError>>;
};

}  // namespace alloy::modbus
