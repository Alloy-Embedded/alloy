#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/uart.hpp"

#include "board.hpp"

namespace board {

// ESP32-C3 DevKitM-1: UART0 TX=GPIO21, RX=GPIO20
using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::UART0,
    alloy::hal::connection::tx<alloy::device::PinId::GPIO21, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::GPIO20, alloy::device::SignalId::signal_rx>>;
using DebugUart = alloy::hal::uart::port_handle<DebugUartConnector>;

inline constexpr std::uint32_t kDebugUartPeripheralClockHz =
    esp32c3_devkitm::ClockConfig::pclk_freq_hz;

[[nodiscard]] inline auto make_debug_uart(alloy::hal::uart::Config config = {}) -> DebugUart {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kDebugUartPeripheralClockHz;
    }
    return DebugUart{config};
}

}  // namespace board
