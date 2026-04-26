#pragma once

// ESP-WROVER-KIT v4.1 board support — Espressif ESP32-WROVER-B module
// (Xtensa LX6, dual-core, 8 MB PSRAM on the WROVER-B variant).
//
// Default UART:        UART0 (GPIO1 TX / GPIO3 RX) routed through the
//                      FT2232HL channel B; visible on the host as the second
//                      USB serial port enumerated by the kit.
// Status LED:          green channel of the on-board RGB LED on GPIO2,
//                      active HIGH. The same `board::led` API as the simpler
//                      DevKitC; the red and blue channels (GPIO0, GPIO4) are
//                      reserved for future RGB helpers.
// JTAG:                FT2232HL channel A drives JTAG pins; OpenOCD config
//                      is `board/esp32-wrover-kit-3.3v.cfg` upstream.
//
// Out of scope for v1 (tracked in OpenSpec add-esp32-classic-family):
//   - PSRAM bring-up (the bootloader normally configures it; we run direct-
//     boot here so PSRAM stays disabled until a follow-up).
//   - Onboard ILI9341 LCD (SPI3) and microSD slot (SPI2).
//   - Full RGB LED helper (only the green channel is exposed today).
//   - Dual-core APP_CPU bring-up; v1 runs single-core like the DevKitC.

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
