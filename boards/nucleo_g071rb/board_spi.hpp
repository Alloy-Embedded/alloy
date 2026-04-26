#pragma once

// SPI1 on Nucleo-G071RB (Arduino-compatible header, 3-wire mode).
//
// Pin mapping (SPI1):
//   PA5  SCK   — Arduino D13 (also shared with LED LD4; disable LED before use)
//   PA6  MISO  — Arduino D12
//   PA7  MOSI  — Arduino D11
//
// Chip select is NOT part of this connector; manage it separately via a GPIO
// pin (e.g. PA4 = D10) or hardware NSS.  Use GpioCsPolicy<pin_handle<PA4>>
// with drivers that require explicit CS management (e.g. SdCard).

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

inline constexpr std::uint32_t kBoardSpiPeripheralClockHz =
    nucleo_g071rb::ClockConfig::apb_clock_hz;

[[nodiscard]] inline auto make_spi(alloy::hal::spi::Config config = {}) -> BoardSpi {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kBoardSpiPeripheralClockHz;
    }
    return BoardSpi{config};
}

}  // namespace board
