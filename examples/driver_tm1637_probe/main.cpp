// examples/driver_tm1637_probe/main.cpp
//
// SAME70 Xplained Ultra — TM1637 4-digit 7-segment display probe.
//
// Wiring (TM1637 module → EXT1 header):
//   TM1637 CLK → PD25  (EXT1 pin 15)   software GPIO bit-bang CLK
//   TM1637 DIO → PD26  (EXT1 pin 17)   software GPIO bit-bang DIO
//   TM1637 VCC → 5V    (EXT1 pin 18)
//   TM1637 GND → GND   (EXT1 pin 19)
//
// Expected UART output:
//   [tm1637] booting
//   [tm1637] init ok
//   [tm1637] display 1234 ok
//   [tm1637] PROBE PASS

#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_tm1637_probe requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER

#include "drivers/display/tm1637/tm1637.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/gpio.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::display::tm1637;

// ── GPIO pin types ────────────────────────────────────────────────────────────

// PD25 = EXT1 pin 15: CLK
using ClkPin = alloy::hal::gpio::pin_handle<
    alloy::hal::gpio::pin<alloy::device::PinId::PD25>>;

// PD26 = EXT1 pin 17: DIO
using DioPin = alloy::hal::gpio::pin_handle<
    alloy::hal::gpio::pin<alloy::device::PinId::PD26>>;

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
    uart::write_line(debug, "[tm1637] booting");

    // Configure CLK pin as push-pull output, initial state high (idle).
    ClkPin clk_pin{{.direction     = alloy::hal::PinDirection::Output,
                    .initial_state = alloy::hal::PinState::High}};
    if (clk_pin.configure().is_err()) {
        uart::write_line(debug, "[tm1637] CLK pin configure failed");
        halt_blink(100u);
    }

    // Configure DIO pin as push-pull output, initial state high (idle).
    DioPin dio_pin{{.direction     = alloy::hal::PinDirection::Output,
                    .initial_state = alloy::hal::PinState::High}};
    if (dio_pin.configure().is_err()) {
        uart::write_line(debug, "[tm1637] DIO pin configure failed");
        halt_blink(100u);
    }

    drv::Device<ClkPin, DioPin> display{clk_pin, dio_pin};

    if (display.init().is_err()) {
        uart::write_line(debug, "[tm1637] init failed");
        halt_blink(100u);
    }
    uart::write_line(debug, "[tm1637] init ok");

    if (display.display_number(1234u).is_err()) {
        uart::write_line(debug, "[tm1637] display_number failed");
        halt_blink(100u);
    }
    uart::write_line(debug, "[tm1637] display 1234 ok");

    uart::write_line(debug, "[tm1637] PROBE PASS");
    halt_blink(500u);
}
