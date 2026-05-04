#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/uart.hpp"

#include "board.hpp"

namespace board {

using DebugUartConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::UART0,
    alloy::hal::connection::tx<alloy::device::PinId::GP0, alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::GP1, alloy::device::SignalId::signal_rx>>;
using DebugUart = alloy::hal::uart::port<DebugUartConnector>;

inline constexpr std::uint32_t kDebugUartPeripheralClockHz =
    raspberry_pi_pico::ClockConfig::pclk_freq_hz;

[[nodiscard]] inline auto make_debug_uart(alloy::hal::uart::Config config = {}) -> DebugUart {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kDebugUartPeripheralClockHz;
    }
    return DebugUart{config};
}

}  // namespace board
