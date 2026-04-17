#pragma once

#include <cstdint>

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/spi.hpp"

#include "board.hpp"

namespace board {

using BoardSpiSckPin = alloy::hal::pin<"PA5">;
using BoardSpiMisoPin = alloy::hal::pin<"PA6">;
using BoardSpiMosiPin = alloy::hal::pin<"PA7">;

using BoardSpiConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"SPI1">, alloy::device::runtime::PeripheralId::SPI1,
    alloy::hal::connection::runtime_binding<alloy::hal::sck<BoardSpiSckPin>,
                                            alloy::device::runtime::PinId::PA5,
                                            alloy::device::runtime::SignalId::signal_sck>,
    alloy::hal::connection::runtime_binding<alloy::hal::miso<BoardSpiMisoPin>,
                                            alloy::device::runtime::PinId::PA6,
                                            alloy::device::runtime::SignalId::signal_miso>,
    alloy::hal::connection::runtime_binding<alloy::hal::mosi<BoardSpiMosiPin>,
                                            alloy::device::runtime::PinId::PA7,
                                            alloy::device::runtime::SignalId::signal_mosi>>;
using BoardSpi = alloy::hal::spi::port_handle<BoardSpiConnector>;

inline constexpr std::uint32_t kBoardSpiPeripheralClockHz = nucleo_f401re::ClockConfig::apb2_hz;

[[nodiscard]] inline auto make_spi(alloy::hal::spi::Config config = {}) -> BoardSpi {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kBoardSpiPeripheralClockHz;
    }
    return BoardSpi{config};
}

}  // namespace board
