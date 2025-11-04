#pragma once

/**
 * STM32F0 GPIO HAL
 *
 * STM32F0 uses modern GPIO registers (similar to F4)
 * Can reuse most of the STM32F4 implementation
 */

#include "../stm32f4/gpio_hal.hpp"

namespace alloy::hal::stm32f0 {

// Reuse STM32F4 GPIO implementation
template<typename Hardware, uint8_t Pin>
using GPIOPin = alloy::hal::stm32f4::GPIOPin<Hardware, Pin>;

}  // namespace alloy::hal::stm32f0
