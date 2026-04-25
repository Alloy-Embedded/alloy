#pragma once

// ESP32-S3-DevKitC-1 board support — ESP32-S3 (Xtensa LX7 dual-core), 240 MHz PLL.
//
// LED: GPIO48 RGB WS2812 (not in bootstrap descriptor; GPIO8 used as placeholder)
// UART0: GPIO43 TX / GPIO44 RX, 115200-8-N-1

#include <cstdint>

#include "board_config.hpp"
#include "device/system_clock.hpp"

namespace board {

using namespace esp32s3_devkitc;
using namespace alloy::hal;

namespace led {
void init();
void on();
void off();
void toggle();
}  // namespace led

void init();

}  // namespace board
