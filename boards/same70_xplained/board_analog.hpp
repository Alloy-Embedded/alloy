#pragma once

#include "hal/adc.hpp"
#include "hal/dac.hpp"
#include "board_config.hpp"

namespace board {

inline constexpr bool kBoardHasAdc = true;
inline constexpr bool kBoardHasDac = true;

using BoardAdc = alloy::hal::adc::handle<alloy::hal::adc::PeripheralId::AFEC0>;
using BoardDac = alloy::hal::dac::handle<alloy::hal::dac::PeripheralId::DACC, 0u>;

[[nodiscard]] inline auto make_adc(alloy::hal::adc::Config config = {}) -> BoardAdc {
    config.peripheral_clock_hz = same70_xplained::ClockConfig::pclk_freq_hz;
    return BoardAdc{config};
}

[[nodiscard]] inline auto make_dac(alloy::hal::dac::Config config = {}) -> BoardDac {
    return BoardDac{config};
}

}  // namespace board
