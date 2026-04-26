// examples/driver_aht20_probe/main.cpp
//
// SAME70 Xplained Ultra — aht20 driver probe.
//
// TODO: document wiring (connector pins, I2C address / SPI CS, VCC/GND).
//
// Expected UART output:
//   [aht20] booting
//   [aht20] init ok
//   [aht20] value=<reading>
//   [aht20] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_aht20_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_I2C_HEADER
#    error "driver_aht20_probe requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "drivers/sensor/aht20/aht20.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::sensor::aht20;

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100); }
    uart::write_line(debug, "[aht20] booting");

    // TODO: configure bus (I2C address, SPI mode, baud rate, etc.)
    auto bus = board::make_i2c();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[aht20] bus configure failed");
        halt_blink(100);
    }

    drv::Device sensor{bus};
    if (sensor.init().is_err()) {
        uart::write_line(debug, "[aht20] init failed");
        halt_blink(100);
    }
    uart::write_line(debug, "[aht20] init ok");

    auto m = sensor.read();
    if (m.is_err()) {
        uart::write_line(debug, "[aht20] read failed");
        halt_blink(100);
    }

    // TODO: print measurement fields.
    uart::write_line(debug, "[aht20] read ok");
    uart::write_line(debug, "[aht20] PROBE PASS");
    halt_blink(500);
}
