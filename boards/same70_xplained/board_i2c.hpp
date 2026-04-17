#pragma once

#include <cstdint>

#include "hal/connect/runtime_connector.hpp"
#include "hal/connect/tags.hpp"
#include "hal/i2c.hpp"

#include "board.hpp"

namespace board {

using BoardI2cSclPin = alloy::hal::pin<"PA4">;
using BoardI2cSdaPin = alloy::hal::pin<"PA3">;

using BoardI2cConnector = alloy::hal::connection::runtime_connector<
    alloy::hal::peripheral<"TWIHS0">, alloy::device::runtime::PeripheralId::TWIHS0,
    alloy::hal::connection::runtime_binding<alloy::hal::scl<BoardI2cSclPin>,
                                            alloy::device::runtime::PinId::PA4,
                                            alloy::device::runtime::SignalId::signal_twck0>,
    alloy::hal::connection::runtime_binding<alloy::hal::sda<BoardI2cSdaPin>,
                                            alloy::device::runtime::PinId::PA3,
                                            alloy::device::runtime::SignalId::signal_twd0>>;
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
