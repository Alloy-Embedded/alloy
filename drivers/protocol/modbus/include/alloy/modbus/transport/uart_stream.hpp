#pragma once

// uart_stream.hpp: alloy::hal::uart::port_handle → ByteStream adapter.
//
// Wraps any port_handle<Connector> as a Modbus ByteStream so that Slave and
// Master can drive a physical RS-232/RS-485 UART.
//
// Template parameters:
//   Handle  — alloy::hal::uart::port_handle<Connector>; must be valid.
//   NowFn   — callable () -> uint64_t returning microseconds (same pattern as
//             Master's NowFn).  Used to implement read() timeout and
//             wait_idle() silence window independently of CPU speed.
//
// Limitations of the current implementation:
//   - The HAL read_uart() busy-polls the RXNE flag with a fixed 1 000 000
//     iteration limit (~6.7 ms at 150 MHz).  read() calls it one byte at a
//     time; if NowFn is not used between poll bursts, the effective timeout
//     granularity is bounded by that burst duration, not by timeout_us alone.
//     On high-baud in-frame reads (back-to-back bytes) this is not observable
//     in practice because RXNE asserts within one character time.
//   - flush(timeout_us) ignores timeout_us; the HAL flush() uses its own
//     internal TC-flag polling with the same 1 000 000 iteration cap.
//   - wait_idle() drains the HAL RX buffer then waits the silence window
//     using a NowFn busy-wait.  It does not re-sample RXNE during the wait;
//     bytes that arrive during the silence window are silently dropped.
//
// Usage:
//   auto uart = board::make_modbus_uart();
//   uart.configure();
//   auto now = []() noexcept -> std::uint64_t { return board::systick_us(); };
//   alloy::modbus::UartStream stream{uart, now};
//   alloy::modbus::Slave slave{stream, 0x01u, registry};

#include <cstddef>
#include <cstdint>
#include <span>

#include "alloy/modbus/byte_stream.hpp"
#include "core/error_code.hpp"
#include "core/result.hpp"

namespace alloy::modbus {

template <typename Handle, typename NowFn>
class UartStream {
   public:
    // Handle must outlive UartStream.
    UartStream(Handle& uart, NowFn now_us) noexcept
        : uart_{uart}, now_us_{now_us} {}

    UartStream(const UartStream&) = delete;
    UartStream& operator=(const UartStream&) = delete;

    // -----------------------------------------------------------------------
    // ByteStream::read
    // Reads up to buf.size() bytes. Returns the byte count actually read.
    // Blocks until at least one byte arrives, or until timeout_us elapses.
    // After the first byte is received, subsequent bytes are read greedily
    // (the HAL RXNE poll is fast for back-to-back in-frame data).
    // -----------------------------------------------------------------------
    [[nodiscard]] core::Result<std::size_t, StreamError>
    read(std::span<std::byte> buf, std::uint32_t timeout_us) noexcept {
        if (buf.empty()) return core::Ok(std::size_t{0u});

        const std::uint64_t deadline =
            (timeout_us > 0u) ? (now_us_() + timeout_us) : now_us_();
        std::size_t n = 0u;

        while (n < buf.size()) {
            std::byte b[1];
            const auto r = uart_.read(std::span<std::byte>{b, 1u});

            if (r.is_ok() && r.unwrap() == 1u) {
                buf[n++] = b[0];
                // Greedy: keep reading without re-checking deadline — next byte
                // is expected to follow within one character time.
                continue;
            }

            // HAL reported no byte (Timeout after ~1M polls) — check deadline.
            if (timeout_us == 0u || now_us_() >= deadline) break;
        }

        return core::Ok(std::size_t{n});
    }

    // -----------------------------------------------------------------------
    // ByteStream::write
    // Queues all bytes in the TX FIFO. Blocks until the last byte is accepted.
    // -----------------------------------------------------------------------
    [[nodiscard]] core::Result<void, StreamError>
    write(std::span<const std::byte> buf) noexcept {
        if (buf.empty()) return core::Ok();

        const auto r = uart_.write(buf);
        if (r.is_err()) return core::Err(map_error(r.unwrap_err()));
        if (r.unwrap() < buf.size()) return core::Err(StreamError::HardwareError);
        return core::Ok();
    }

    // -----------------------------------------------------------------------
    // ByteStream::flush
    // Waits for the TX shift register to drain (TC flag).
    // timeout_us is noted for API symmetry; the HAL uses its own internal cap.
    // -----------------------------------------------------------------------
    [[nodiscard]] core::Result<void, StreamError>
    flush([[maybe_unused]] std::uint32_t timeout_us) noexcept {
        const auto r = uart_.flush();
        if (r.is_err()) return core::Err(map_error(r.unwrap_err()));
        return core::Ok();
    }

    // -----------------------------------------------------------------------
    // ByteStream::wait_idle
    // Drains any pending RX data, then waits until the line has been quiet
    // for at least silence_us microseconds (NowFn busy-wait).
    // -----------------------------------------------------------------------
    [[nodiscard]] core::Result<void, StreamError>
    wait_idle(std::uint32_t silence_us) noexcept {
        // Drain: read and discard bytes until HAL reports no data.
        {
            std::byte b[1];
            while (true) {
                const auto r = uart_.read(std::span<std::byte>{b, 1u});
                if (r.is_err() || r.unwrap() == 0u) break;
            }
        }

        // Silence window: busy-wait for silence_us with the caller's clock.
        const std::uint64_t end = now_us_() + silence_us;
        while (now_us_() < end) { /* busy-wait */ }

        return core::Ok();
    }

   private:
    Handle& uart_;
    NowFn   now_us_;

    [[nodiscard]] static constexpr StreamError
    map_error(core::ErrorCode ec) noexcept {
        switch (ec) {
            case core::ErrorCode::Timeout:       return StreamError::Timeout;
            case core::ErrorCode::HardwareError: return StreamError::HardwareError;
            case core::ErrorCode::BufferFull:    return StreamError::Overrun;
            default:                             return StreamError::HardwareError;
        }
    }
};

// Deduction guide: UartStream uart_stream{uart, now_fn};
template <typename Handle, typename NowFn>
UartStream(Handle&, NowFn) -> UartStream<Handle, NowFn>;

}  // namespace alloy::modbus
