#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/spi.hpp"

#include "board.hpp"

namespace board {

using BoardSpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI0,
    alloy::hal::connection::sck<alloy::device::PinId::PD22, alloy::device::SignalId::signal_spck>,
    alloy::hal::connection::miso<alloy::device::PinId::PD20,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PD21,
                                 alloy::device::SignalId::signal_mosi>>;
using BoardSpi = alloy::hal::spi::port_handle<BoardSpiConnector>;

inline constexpr std::uint32_t kBoardSpiPeripheralClockHz =
    same70_xplained::ClockConfig::pclk_freq_hz;

[[nodiscard]] inline auto make_spi(alloy::hal::spi::Config config = {}) -> BoardSpi {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kBoardSpiPeripheralClockHz;
    }
    return BoardSpi{config};
}

}  // namespace board
