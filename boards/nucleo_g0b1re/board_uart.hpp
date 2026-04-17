#pragma once

#include <cstdint>

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/uart.hpp"

#include "board.hpp"

namespace board {

using DebugUartTxPin = alloy::hal::pin<"PA2">;
using DebugUartRxPin = alloy::hal::pin<"PA3">;

using DebugUartConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"USART2">, alloy::device::runtime::PeripheralId::USART2,
    alloy::hal::connection::runtime_binding<alloy::hal::tx<DebugUartTxPin>,
                                            alloy::device::runtime::PinId::PA2,
                                            alloy::device::runtime::SignalId::signal_tx>,
    alloy::hal::connection::runtime_binding<alloy::hal::rx<DebugUartRxPin>,
                                            alloy::device::runtime::PinId::PA3,
                                            alloy::device::runtime::SignalId::signal_rx>>;
using DebugUart = alloy::hal::uart::port_handle<DebugUartConnector>;

inline constexpr std::uint32_t kDebugUartPeripheralClockHz =
    nucleo_g0b1re::ClockConfig::apb_clock_hz;

[[nodiscard]] inline auto make_debug_uart(alloy::hal::uart::Config config = {}) -> DebugUart {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kDebugUartPeripheralClockHz;
    }
    return DebugUart{config};
}

}  // namespace board
