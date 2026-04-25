#pragma once

// ESP32-C3-DevKitM-1 board support — RISC-V RV32IMC, direct boot, 40 MHz ROM clock.
//
// LED: GPIO8 (WS2812 data pin, used as simple digital output)
// UART0: GPIO21 TX / GPIO20 RX, pre-configured by ROM at 115200-8-N-1

#include <cstdint>

namespace board {

namespace led {
void init();
void on();
void off();
void toggle();
}  // namespace led

void init();

}  // namespace board
