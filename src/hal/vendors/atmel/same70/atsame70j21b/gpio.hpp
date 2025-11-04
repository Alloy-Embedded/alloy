#pragma once

#include "hardware.hpp"
#include "pins.hpp"
#include "pin_functions.hpp"
#include "../pio_hal.hpp"

namespace alloy::hal::atmel::same70::atsame70j21b {

// Re-export from sub-namespaces for convenience
using namespace hardware;
using namespace pins;
using namespace pin_functions;

// Use the SAME70 PIO HAL
template<uint8_t Pin>
using GPIOPin = same70::PIOPin<hardware::PIO_Registers, Pin>;

}  // namespace alloy::hal::atmel::same70::atsame70j21b
