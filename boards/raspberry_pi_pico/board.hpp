#pragma once

// Raspberry Pi Pico board support — RP2040, Cortex-M0+, 125 MHz PLL.
//
// Pins used:
//   LED  : GP25 (active HIGH)
//   UART0: GP0 TX / GP1 RX (115200-8-N-1)
//
// board::init() configures the system clock via the published device contract,
// initialises SysTick to 1 ms, and sets GP25 as output-low.

#include <cstdint>

#include "hal/systick.hpp"

#include "board_config.hpp"
#include "device/system_clock.hpp"

namespace board {

using namespace raspberry_pi_pico;
using namespace alloy::hal;

using BoardSysTick = alloy::hal::cortex_m::SysTick<ClockConfig::cpu_freq_hz>;
using RTOSTick = BoardSysTick;

namespace led {
void init();
void on();
void off();
void toggle();
}  // namespace led

void init();

}  // namespace board
