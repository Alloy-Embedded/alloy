// examples/driver_ili9341_probe/main.cpp
//
// SAME70 Xplained Ultra — ILI9341 TFT display driver probe.
//
// Wiring (EXT1 connector on SAME70 Xplained Ultra):
//   ILI9341 SPI MOSI  → EXT1 pin 16  (SPI0 MOSI / PA13 / PERIPH A)
//   ILI9341 SPI MISO  → EXT1 pin 17  (SPI0 MISO / PA12 / PERIPH A)
//   ILI9341 SPI SCK   → EXT1 pin 18  (SPI0 SPCK / PA14 / PERIPH A)
//   ILI9341 SPI CS    → EXT1 pin 15  (PD25 — GPIO output, active low)
//   ILI9341 DC        → EXT1 pin 10  (PD26 — GPIO output, low=cmd, high=data)
//   ILI9341 VCC       → EXT1 pin 20  (VCC 3.3 V)
//   ILI9341 GND       → EXT1 pin 19  (GND)
//
// What the probe does, in order:
//   1. Initialises the board debug UART (115200-8-N-1).
//   2. Brings up SPI0 in mode 0 at 20 MHz.
//   3. Calls Device::init() to run the ILI9341 power-on sequence.
//   4. Fills the entire 240x320 screen with solid blue (RGB565 0x001F).
//   5. Draws a 10x10 red pixel block at (10, 10) as a spot-check.
//   6. Prints "[ili9341] PROBE PASS" and blinks the user LED at 500 ms.
//
// Expected UART output:
//   [ili9341] booting
//   [ili9341] init ok
//   [ili9341] fill blue ok
//   [ili9341] draw pixel ok
//   [ili9341] PROBE PASS
//
// On failure the line preceding FAIL tells you which step broke. The LED
// blinks rapidly (100 ms) on any failure.

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
    #error "driver_ili9341_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_SPI_HEADER
    #error "driver_ili9341_probe requires BOARD_SPI_HEADER for the selected board"
#endif
#ifndef BOARD_GPIO_HEADER
    #error "driver_ili9341_probe requires BOARD_GPIO_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER
#include BOARD_GPIO_HEADER

#include "drivers/display/ili9341/ili9341.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace {

using namespace alloy::examples::uart_console;

namespace drv = alloy::drivers::display::ili9341;

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

[[noreturn]] void blink_ok() {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500u);
    }
}

}  // namespace

int main() {
    board::init();

    // 1. Debug UART
    auto uart = board::make_debug_uart();
    if (uart.configure().is_err()) {
        blink_error(100u);
    }
    write_line(uart, "[ili9341] booting");

    // 2. SPI bus (SPI0, mode 0, ~20 MHz via board BSP)
    auto bus = board::make_spi();
    if (bus.configure().is_err()) {
        write_line(uart, "[ili9341] FAIL (spi configure)");
        blink_error(100u);
    }

    // CS pin — PD25 (EXT1 pin 15), GPIO output, active low.
    auto cs_pin = board::make_gpio_output(board::GpioId::PD25);
    if (cs_pin.configure().is_err()) {
        write_line(uart, "[ili9341] FAIL (cs pin configure)");
        blink_error(100u);
    }

    // DC pin — PD26 (EXT1 pin 10), GPIO output, low=command, high=data.
    auto dc_pin = board::make_gpio_output(board::GpioId::PD26);
    if (dc_pin.configure().is_err()) {
        write_line(uart, "[ili9341] FAIL (dc pin configure)");
        blink_error(100u);
    }

    drv::GpioDcPolicy<decltype(dc_pin)> dc{dc_pin};
    drv::GpioCsPolicy<decltype(cs_pin)> cs{cs_pin};

    drv::Device<decltype(bus),
                drv::GpioDcPolicy<decltype(dc_pin)>,
                drv::GpioCsPolicy<decltype(cs_pin)>>
        display{bus, dc, cs};

    // 3. Init
    if (display.init().is_err()) {
        write_line(uart, "[ili9341] FAIL (init)");
        blink_error(100u);
    }
    write_line(uart, "[ili9341] init ok");

    // 4. Fill screen blue (RGB565: R=0, G=0, B=31 → 0x001F)
    constexpr std::uint16_t kBlue = 0x001Fu;
    if (display.fill_rect(0u, 0u, drv::kWidth, drv::kHeight, kBlue).is_err()) {
        write_line(uart, "[ili9341] FAIL (fill blue)");
        blink_error(100u);
    }
    write_line(uart, "[ili9341] fill blue ok");

    // 5. Draw a 10x10 red block at (10, 10) as a visual spot-check.
    //    RGB565 red = 0xF800 (R=31, G=0, B=0).
    constexpr std::uint16_t kRed = 0xF800u;
    if (display.fill_rect(10u, 10u, 10u, 10u, kRed).is_err()) {
        write_line(uart, "[ili9341] FAIL (draw pixel block)");
        blink_error(100u);
    }
    write_line(uart, "[ili9341] draw pixel ok");

    write_line(uart, "[ili9341] PROBE PASS");
    blink_ok();
}
