// examples/driver_st7789_probe/main.cpp
//
// SAME70 Xplained Ultra — ST7789 240×320 TFT display driver probe.
//
// Wiring (ST7789 module → EXT1 header, SPI0, SW CS on PD25, DC on PD26):
//   ST7789 VCC   → 3V3    (EXT1 pin 20)
//   ST7789 GND   → GND    (EXT1 pin 19)
//   ST7789 MOSI  → PD21   (EXT1 pin 16)   SPI0_MOSI
//   ST7789 MISO  → PD20   (EXT1 pin 17)   SPI0_MISO  (may be NC on write-only modules)
//   ST7789 SCLK  → PD22   (EXT1 pin 18)   SPI0_SPCK
//   ST7789 CS    → PD25   (EXT1 pin 15)   GPIO output, active-low
//   ST7789 DC    → PD26   (EXT1 pin 14)   GPIO output, command=low / data=high
//   ST7789 RES   → 3V3 (tie high) or a dedicated GPIO reset pin
//   ST7789 BL    → 3V3    (backlight always on for probe)
//
// Expected UART output:
//   [st7789] booting
//   [st7789] init ok
//   [st7789] fill red ok
//   [st7789] draw pixel ok
//   [st7789] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_st7789_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_SPI_HEADER
#    error "driver_st7789_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include "drivers/display/st7789/st7789.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::display::st7789;

// ── Helpers ───────────────────────────────────────────────────────────────────

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100u); }
    uart::write_line(debug, "[st7789] booting");

    // Configure SPI bus (mode 0, ≤ 40 MHz; use ≤ 1 MHz for initial bring-up).
    auto bus = board::make_spi();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[st7789] bus configure failed");
        halt_blink(100u);
    }

    // CS on PD25 — active-low GPIO managed by GpioCsPolicy.
    auto cs_pin = board::make_gpio_pd25();
    drv::GpioCsPolicy<decltype(cs_pin)> cs_policy{cs_pin};

    // DC on PD26 — command=low / data=high, managed by GpioDcPolicy.
    auto dc_pin = board::make_gpio_pd26();
    drv::GpioDcPolicy<decltype(dc_pin)> dc_policy{dc_pin};

    // Construct display driver (default Config: 16-bit RGB565, no rotation).
    drv::Device<decltype(bus),
                drv::GpioDcPolicy<decltype(dc_pin)>,
                drv::GpioCsPolicy<decltype(cs_pin)>>
        display{bus, dc_policy, cs_policy};

    if (display.init().is_err()) {
        uart::write_line(debug, "[st7789] init failed");
        halt_blink(100u);
    }
    uart::write_line(debug, "[st7789] init ok");

    // Fill entire screen red (RGB565: 0xF800).
    constexpr std::uint16_t kRed = 0xF800u;
    if (display.fill_rect(0u, 0u, drv::kWidth, drv::kHeight, kRed).is_err()) {
        uart::write_line(debug, "[st7789] fill red failed");
        halt_blink(100u);
    }
    uart::write_line(debug, "[st7789] fill red ok");

    // Draw a single white pixel at the screen centre (120, 160).
    constexpr std::uint16_t kWhite = 0xFFFFu;
    if (display.draw_pixel(120u, 160u, kWhite).is_err()) {
        uart::write_line(debug, "[st7789] draw pixel failed");
        halt_blink(100u);
    }
    uart::write_line(debug, "[st7789] draw pixel ok");

    uart::write_line(debug, "[st7789] PROBE PASS");
    halt_blink(500u);
}
