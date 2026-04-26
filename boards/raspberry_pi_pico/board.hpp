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

// Launch core 1. `fn` runs on the second Cortex-M0+ core with a dedicated
// 8 KB stack (defined in rp2040.ld as `.core1_stack`). Uses the 5-word SIO
// FIFO handshake required by the RP2040 ROM (§2.8.2 of the datasheet).
// Call once from core 0 after board::init().
void launch_core1(void (*fn)());

}  // namespace board
