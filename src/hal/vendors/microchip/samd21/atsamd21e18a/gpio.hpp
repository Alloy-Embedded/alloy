#pragma once

#include "hardware.hpp"
#include "pins.hpp"
#include "../port_hal.hpp"

namespace alloy::hal::atmel::samd21::atsamd21e18a {

// Re-export from sub-namespaces for convenience
using namespace hardware;
using namespace pins;

// Use the SAMD21 PORT HAL
template<uint8_t PortIndex, uint8_t Pin>
using GPIOPin = samd21::PORTPin<hardware::PORT_Registers, PortIndex, Pin>;

}  // namespace alloy::hal::atmel::samd21::atsamd21e18a
