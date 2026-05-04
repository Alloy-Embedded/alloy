#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/uart.hpp"

#include "board.hpp"

namespace board {

using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART1,
    alloy::hal::connection::tx<alloy::device::PinId::PB4, alloy::device::SignalId::signal_txd1>,
    alloy::hal::connection::rx<alloy::device::PinId::PA21,
                               alloy::device::SignalId::signal_rxd1>>;
using DebugUart = alloy::hal::uart::port<DebugUartConnector>;

inline constexpr std::uint32_t kDebugUartPeripheralClockHz =
    same70_xplained::ClockConfig::pclk_freq_hz;
inline constexpr auto kDebugUartBaudrate = alloy::hal::Baudrate::e115200;

[[nodiscard]] inline auto make_debug_uart(
    alloy::hal::uart::Config config = {.baudrate = kDebugUartBaudrate}) -> DebugUart {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kDebugUartPeripheralClockHz;
    }
    return DebugUart{config};
}

}  // namespace board
