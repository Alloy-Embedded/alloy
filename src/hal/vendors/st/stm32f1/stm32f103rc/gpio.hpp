#pragma once

// ============================================================================
// Single-include GPIO header for STM32F103RC
// Include this file to get all GPIO functionality
//
// Usage:
//   #include "hal/vendors/st/stm32f1/stm32f103rc/gpio.hpp"
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

namespace alloy::hal::stm32f1::stm32f103rc {

// Alias GPIO HAL template with this MCU's hardware
template<uint8_t Pin>
using GPIOPin = alloy::hal::stm32f1::GPIOPin<Hardware, Pin>;

}  // namespace alloy::hal::stm32f1::stm32f103rc

// Convenience namespace alias
namespace gpio = alloy::hal::stm32f1::stm32f103rc;
