#pragma once

// ============================================================================
// Single-include GPIO header for STM32F030C6
// Include this file to get all GPIO functionality
//
// Usage:
//   #include "hal/vendors/st/stm32f0/stm32f030c6/gpio.hpp"
//
//   using namespace gpio;
//   using LED = GPIOPin<pins::PC13>;
//   LED::configureOutput();
//   LED::set();
// ============================================================================

#include "pins.hpp"
#include "pin_functions.hpp"
#include "traits.hpp"
#include "hardware.hpp"
#include "../gpio_hal.hpp"

namespace alloy::hal::stm32f0::stm32f030c6 {

// Alias GPIO HAL template with this MCU's hardware
template<uint8_t Pin>
using GPIOPin = alloy::hal::stm32f0::GPIOPin<Hardware, Pin>;

}  // namespace alloy::hal::stm32f0::stm32f030c6

// Convenience namespace alias
namespace gpio = alloy::hal::stm32f0::stm32f030c6;
