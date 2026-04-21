#pragma once

#include "hal/adc.hpp"
#include "hal/dac.hpp"

namespace board {

inline constexpr bool kBoardHasAdc = true;
inline constexpr bool kBoardHasDac = true;

using BoardAdc = alloy::hal::adc::handle<alloy::hal::adc::PeripheralId::ADC1>;
using BoardDac = alloy::hal::dac::handle<alloy::hal::dac::PeripheralId::DAC, 0u>;

[[nodiscard]] inline auto make_adc(alloy::hal::adc::Config config = {}) -> BoardAdc {
    return BoardAdc{config};
}

[[nodiscard]] inline auto make_dac(alloy::hal::dac::Config config = {}) -> BoardDac {
    return BoardDac{config};
}

}  // namespace board
