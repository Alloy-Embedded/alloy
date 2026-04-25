#pragma once

// AVR128DA32 Curiosity Nano board support.
//
// MCU: AVR128DA32 (AVR® DA, 32-pin TQFP)
// Clock: 20 MHz OSC / 4× PLL → 24 MHz (CLKCTRL default profile)
// LED: PC6 active-LOW (placeholder: PA5 until pin bootstrap is extended)
// UART: USART0 — PA0 TX / PA1 RX, 115200-8-N-1

#include <cstdint>

#include "board_config.hpp"
#include "device/system_clock.hpp"

namespace board {

using namespace avr128da32_curiosity_nano;
using namespace alloy::hal;

namespace led {
void init();
void on();
void off();
void toggle();
}  // namespace led

void init();

}  // namespace board
