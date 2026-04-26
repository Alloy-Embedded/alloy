// examples/driver_uc8151_probe/main.cpp
//
// SAME70 Xplained Ultra — UC8151 e-paper display driver probe.
//
// Wiring (e-paper module → EXT1 header, SPI0):
//   Module VCC   → 3V3    (EXT1 pin 20)
//   Module GND   → GND    (EXT1 pin 19)
//   Module MOSI  → PD21   (EXT1 pin 16)  SPI0_MOSI
//   Module MISO  → PD20   (EXT1 pin 17)  SPI0_MISO  (NC on most e-paper modules)
//   Module SCLK  → PD22   (EXT1 pin 18)  SPI0_SPCK
//   Module CS    → PD25   (EXT1 pin 15)  GPIO output, active-low
//   Module DC    → PD26   (EXT1 pin 14)  GPIO output, command=low / data=high
//   Module BUSY  → PD27   (EXT1 pin 13)  GPIO input, high=busy / low=idle
//   Module RST   → 3V3 (tie high; or connect to a separate reset GPIO)
//
// Target panel: 2.13" 212×104 B/W (UC8151 default). Change Width/Height
// template args for other panel sizes (e.g. 296×128 for 2.9").
//
// Expected UART output:
//   [uc8151] booting
//   [uc8151] init ok
//   [uc8151] update ok
//   [uc8151] sleep ok
//   [uc8151] PROBE PASS
//
// On failure the line preceding FAIL identifies the failing step. The display
// will remain blank during the update (all-white framebuffer). LED blinks
// slowly (500 ms) on pass, rapidly (100 ms) on any failure.

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_uc8151_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_SPI_HEADER
#    error "driver_uc8151_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include "drivers/display/uc8151/uc8151.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::display::uc8151;

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
    uart::write_line(debug, "[uc8151] booting");

    // SPI0 bus (mode 0, ≤ 4 MHz recommended for e-paper).
    auto bus = board::make_spi();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[uc8151] FAIL (bus configure)");
        halt_blink(100u);
    }

    // CS on PD25 — active-low, software-managed.
    auto cs_pin = board::make_gpio_pd25();
    drv::GpioCsPolicy<decltype(cs_pin)> cs_policy{cs_pin};

    // DC on PD26 — command=low / data=high.
    auto dc_pin = board::make_gpio_pd26();
    drv::GpioDcPolicy<decltype(dc_pin)> dc_policy{dc_pin};

    // BUSY on PD27 — high while the controller is busy.
    auto busy_pin = board::make_gpio_pd27();
    drv::GpioBusyPolicy<decltype(busy_pin)> busy_policy{busy_pin};

    // Construct driver for 212×104 2.13" panel (UC8151 default dimensions).
    drv::Device<decltype(bus),
                drv::GpioDcPolicy<decltype(dc_pin)>,
                drv::GpioCsPolicy<decltype(cs_pin)>,
                drv::GpioBusyPolicy<decltype(busy_pin)>>
        epd{bus, dc_policy, cs_policy, busy_policy};

    if (epd.init().is_err()) {
        uart::write_line(debug, "[uc8151] FAIL (init)");
        halt_blink(100u);
    }
    uart::write_line(debug, "[uc8151] init ok");

    // All-white framebuffer: 0x00 = white for every pixel.
    // Size = (212 * 104 + 7) / 8 = 2756 bytes — stack-allocated.
    constexpr std::size_t kFbBytes = (212u * 104u + 7u) / 8u;
    std::array<std::uint8_t, kFbBytes> framebuffer{};
    framebuffer.fill(0x00u);

    if (epd.update(framebuffer).is_err()) {
        uart::write_line(debug, "[uc8151] FAIL (update)");
        halt_blink(100u);
    }
    uart::write_line(debug, "[uc8151] update ok");

    // Enter deep-sleep to minimise idle current.
    if (epd.sleep().is_err()) {
        uart::write_line(debug, "[uc8151] FAIL (sleep)");
        halt_blink(100u);
    }
    uart::write_line(debug, "[uc8151] sleep ok");

    uart::write_line(debug, "[uc8151] PROBE PASS");
    halt_blink(500u);
}
