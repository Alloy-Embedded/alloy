#pragma once

#include "hardware.hpp"
#include "pins.hpp"
#include "../sio_hal.hpp"

namespace alloy::hal::raspberrypi::rp2040 {

// Re-export from sub-namespaces for convenience
using namespace hardware;
using namespace pins;

// Use the RP2040 SIO HAL
template<uint8_t Pin>
using GPIOPin = rp2040::SIOPin<hardware::SIO_Registers, Pin>;

}  // namespace alloy::hal::raspberrypi::rp2040
