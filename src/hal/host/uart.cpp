/// Host UART implementation
///
/// Uses stdin/stdout for UART simulation on host machines.

#include "hal/host/uart.hpp"
#include <iostream>
#include <sys/select.h>
#include <unistd.h>

namespace alloy::hal::host {

Uart::Uart()
    : configured_(false)
    , config_(core::baud_rates::Baud115200) {
}

core::Result<void> Uart::configure(UartConfig config) {
    // Validate configuration parameters
    if (config.baud_rate.value() == 0) {
        return core::Result<void>::error(core::ErrorCode::InvalidParameter);
    }

    config_ = config;
    configured_ = true;
    return core::Result<void>::ok();
}

core::Result<core::u8> Uart::read_byte() {
    if (!configured_) {
        return core::Result<core::u8>::error(core::ErrorCode::NotInitialized);
    }

    // Check if data is available without blocking
    if (available() == 0) {
        return core::Result<core::u8>::error(core::ErrorCode::Timeout);
    }

    // Read one byte from stdin
    char ch;
    if (std::cin.get(ch)) {
        return core::Result<core::u8>::ok(static_cast<core::u8>(ch));
    }

    return core::Result<core::u8>::error(core::ErrorCode::HardwareError);
}

core::Result<void> Uart::write_byte(core::u8 byte) {
    if (!configured_) {
        return core::Result<void>::error(core::ErrorCode::NotInitialized);
    }

    // Write byte to stdout
    std::cout.put(static_cast<char>(byte));
    std::cout.flush();  // Ensure immediate output

    if (std::cout.good()) {
        return core::Result<void>::ok();
    }

    return core::Result<void>::error(core::ErrorCode::HardwareError);
}

core::usize Uart::available() const {
    // Use select() with zero timeout to check if data is available
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    timeval timeout = {0, 0};  // Zero timeout = non-blocking

    int result = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);

    if (result > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
        // Data is available
        return 1;  // Simplified: return 1 if any data available
    }

    return 0;
}

} // namespace alloy::hal::host
