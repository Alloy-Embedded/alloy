#pragma once

#include <cstdint>

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/uart.hpp"

#include "board.hpp"

namespace board {

using DebugUartTxPin = alloy::hal::pin<"PB4">;
using DebugUartRxPin = alloy::hal::pin<"PA21">;

using DebugUartConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"USART1">, alloy::device::runtime::PeripheralId::USART1,
    alloy::hal::connection::runtime_binding<alloy::hal::tx<DebugUartTxPin>,
                                            alloy::device::runtime::PinId::PB4,
                                            alloy::device::runtime::SignalId::signal_txd1>,
    alloy::hal::connection::runtime_binding<alloy::hal::rx<DebugUartRxPin>,
                                            alloy::device::runtime::PinId::PA21,
                                            alloy::device::runtime::SignalId::signal_rxd1>>;
using DebugUart = alloy::hal::uart::port_handle<DebugUartConnector>;

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
