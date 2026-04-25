#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/uart.hpp"

#include "board.hpp"

namespace board {

// ESP32-S3 DevKitC-1: UART0 TX=GPIO43 (USB-Serial TX), RX=GPIO44
using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::UART0,
    alloy::hal::connection::tx<alloy::device::PinId::GPIO43, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::GPIO44, alloy::device::SignalId::signal_rx>>;
using DebugUart = alloy::hal::uart::port_handle<DebugUartConnector>;

inline constexpr std::uint32_t kDebugUartPeripheralClockHz =
    esp32s3_devkitc::ClockConfig::pclk_freq_hz;

[[nodiscard]] inline auto make_debug_uart(alloy::hal::uart::Config config = {}) -> DebugUart {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kDebugUartPeripheralClockHz;
    }
    return DebugUart{config};
}

}  // namespace board
