#pragma once

// ESP32-DevKit board support — Xtensa LX6, ESP-IDF bootloader, 80 MHz APB clock.
//
// LED : GPIO2 (built-in blue LED, active HIGH)
// UART0: GPIO1 TX / GPIO3 RX, pre-configured by bootloader at 115200-8-N-1

#include <cstdint>

namespace board {

namespace led {
void init();
void on();
void off();
void toggle();
}  // namespace led

void init();

// Start the APP_CPU (core 1). `fn` is called on the second core with a
// dedicated 8 KB stack. Call once from core 0 after board::init().
void start_app_cpu(void (*fn)());

}  // namespace board
