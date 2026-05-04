#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/uart.hpp"

#include "board.hpp"

namespace board {

using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART2,
    alloy::hal::connection::tx<alloy::device::PinId::PA2, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA3, alloy::device::SignalId::signal_rx>>;
using DebugUart = alloy::hal::uart::port<DebugUartConnector>;

inline constexpr std::uint32_t kDebugUartPeripheralClockHz =
    nucleo_g071rb::ClockConfig::apb_clock_hz;

[[nodiscard]] inline auto make_debug_uart(alloy::hal::uart::Config config = {}) -> DebugUart {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kDebugUartPeripheralClockHz;
    }
    return DebugUart{config};
}

}  // namespace board
