#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/spi.hpp"

#include "board.hpp"

namespace board {

using BoardSpiConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::SPI1,
    alloy::hal::connection::sck<alloy::device::PinId::PA5, alloy::device::SignalId::signal_sck>,
    alloy::hal::connection::miso<alloy::device::PinId::PA6,
                                 alloy::device::SignalId::signal_miso>,
    alloy::hal::connection::mosi<alloy::device::PinId::PA7,
                                 alloy::device::SignalId::signal_mosi>>;
using BoardSpi = alloy::hal::spi::port_handle<BoardSpiConnector>;

inline constexpr std::uint32_t kBoardSpiPeripheralClockHz = nucleo_f401re::ClockConfig::apb2_hz;

[[nodiscard]] inline auto make_spi(alloy::hal::spi::Config config = {}) -> BoardSpi {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kBoardSpiPeripheralClockHz;
    }
    return BoardSpi{config};
}

}  // namespace board
