#pragma once

// ESP32-C3-DevKitM-1 board support — ESP32-C3 (RISC-V), 160 MHz PLL.
//
// LED: GPIO8 (RGB WS2812, used as simple GPIO for probe purposes, active HIGH)
// UART0: GPIO21 TX / GPIO20 RX, 115200-8-N-1

#include <cstdint>

#include "board_config.hpp"
#include "device/system_clock.hpp"

namespace board {

using namespace esp32c3_devkitm;
using namespace alloy::hal;

namespace led {
void init();
void on();
void off();
void toggle();
}  // namespace led

void init();

}  // namespace board
