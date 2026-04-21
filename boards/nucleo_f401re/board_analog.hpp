#pragma once

#include "hal/adc.hpp"

namespace board {

inline constexpr bool kBoardHasAdc = true;
inline constexpr bool kBoardHasDac = false;

using BoardAdc = alloy::hal::adc::handle<alloy::hal::adc::PeripheralId::ADC1>;

[[nodiscard]] inline auto make_adc(alloy::hal::adc::Config config = {}) -> BoardAdc {
    return BoardAdc{config};
}

}  // namespace board
