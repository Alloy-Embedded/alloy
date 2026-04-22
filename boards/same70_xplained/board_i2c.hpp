#pragma once

#include <cstdint>

#include "device/runtime.hpp"
#include "hal/connect/connector.hpp"
#include "hal/i2c.hpp"

#include "board.hpp"

namespace board {

using BoardI2cConnector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::TWIHS0,
    alloy::hal::connection::scl<alloy::device::PinId::PA4, alloy::device::SignalId::signal_twck0>,
    alloy::hal::connection::sda<alloy::device::PinId::PA3, alloy::device::SignalId::signal_twd0>>;
using BoardI2c = alloy::hal::i2c::port_handle<BoardI2cConnector>;

inline constexpr std::uint32_t kBoardI2cPeripheralClockHz =
    same70_xplained::ClockConfig::pclk_freq_hz;

[[nodiscard]] inline auto make_i2c(alloy::hal::i2c::Config config = {}) -> BoardI2c {
    if (config.peripheral_clock_hz == 0u) {
        config.peripheral_clock_hz = kBoardI2cPeripheralClockHz;
    }
    return BoardI2c{config};
}

}  // namespace board
