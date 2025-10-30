/// Host UART implementation
///
/// Provides UART simulation using stdin/stdout for testing without hardware.

#ifndef ALLOY_HAL_HOST_UART_HPP
#define ALLOY_HAL_HOST_UART_HPP

#include "hal/interface/uart.hpp"

namespace alloy::hal::host {

/// Host UART implementation using stdin/stdout
///
/// This implementation allows testing UART-based code on a host machine
/// without requiring actual hardware. It uses:
/// - stdin for read operations (non-blocking when possible)
/// - stdout for write operations
///
/// Configuration (baud rate, data bits, etc.) is accepted but has no effect
/// on host, as the OS terminal handles serial settings.
class Uart {
public:
    /// Construct host UART (initially not configured)
    Uart();

    /// Configure UART parameters
    ///
    /// On host, this validates the configuration but does not apply it
    /// (terminal settings are controlled by the OS).
    [[nodiscard]] core::Result<void> configure(UartConfig config);

    /// Read a single byte from stdin
    ///
    /// Returns ErrorCode::Timeout if no data is available.
    /// Uses non-blocking read when possible.
    [[nodiscard]] core::Result<core::u8> read_byte();

    /// Write a single byte to stdout
    ///
    /// Always succeeds unless stdout is closed.
    [[nodiscard]] core::Result<void> write_byte(core::u8 byte);

    /// Check how many bytes are available in stdin
    ///
    /// Returns the number of bytes that can be read without blocking.
    [[nodiscard]] core::usize available() const;

private:
    bool configured_;
    UartConfig config_;
};

} // namespace alloy::hal::host

// Validate that host UART satisfies UartDevice concept
static_assert(alloy::hal::UartDevice<alloy::hal::host::Uart>,
              "Host Uart must satisfy UartDevice concept");

#endif // ALLOY_HAL_HOST_UART_HPP
