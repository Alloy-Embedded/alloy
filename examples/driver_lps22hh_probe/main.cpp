// examples/driver_lps22hh_probe/main.cpp
//
// SAME70 Xplained Ultra — LPS22HH barometric pressure + temperature probe.
//
// Wiring (LPS22HH breakout → EXT1 header, I2C address 0x5C, SDO tied low):
//   LPS22HH VDD  → 3V3   (EXT1 pin 20)
//   LPS22HH GND  → GND   (EXT1 pin 19)
//   LPS22HH SDA  → PA3   (EXT1 pin 11)   TWI0_SDA
//   LPS22HH SCL  → PA4   (EXT1 pin 12)   TWI0_SCL
//   LPS22HH SDO  → GND   (address 0x5C)
//   LPS22HH CS   → VDD   (selects I2C mode)
//
// Expected UART output:
//   [lps22hh] booting
//   [lps22hh] init ok
//   [lps22hh] pressure=XXXXX.XX hPa  temp=XX.XX C
//   [lps22hh] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_lps22hh_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_I2C_HEADER
#    error "driver_lps22hh_probe requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "drivers/sensor/lps22hh/lps22hh.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::sensor::lps22hh;

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

// Format a fixed-point value as "IIIII.FF" into buf (null-terminated).
// |value| is the physical quantity, |scale| is the number of decimal places.
static void fmt_fixed(float value, unsigned decimals, char* buf, std::size_t sz) {
    if (sz == 0) return;
    bool neg = value < 0.0f;
    if (neg) value = -value;

    // Multiply by 10^decimals to get integer representation.
    std::uint32_t mult = 1u;
    for (unsigned i = 0; i < decimals; ++i) mult *= 10u;

    const auto whole = static_cast<std::uint32_t>(value);
    const auto frac  = static_cast<std::uint32_t>((value - static_cast<float>(whole)) *
                                                    static_cast<float>(mult) + 0.5f);

    char tmp[32]{};
    std::size_t i = 0;
    if (neg) tmp[i++] = '-';

    // Write whole part.
    char w[12]{};
    std::size_t wi = 0;
    std::uint32_t wv = whole;
    if (wv == 0) { w[wi++] = '0'; }
    while (wv > 0 && wi < sizeof(w) - 1) { w[wi++] = static_cast<char>('0' + wv % 10); wv /= 10; }
    for (std::size_t a = 0, b = wi - 1; a < b; ++a, --b) { char t = w[a]; w[a] = w[b]; w[b] = t; }
    for (std::size_t j = 0; j < wi && i < sizeof(tmp) - 1; ++j) tmp[i++] = w[j];

    if (decimals > 0) {
        tmp[i++] = '.';
        // Write fractional part (zero-padded to `decimals` digits).
        char f[12]{};
        std::size_t fi = 0;
        std::uint32_t fv = frac;
        for (unsigned d = 0; d < decimals; ++d) { f[fi++] = static_cast<char>('0' + fv % 10); fv /= 10; }
        // f is in reverse order.
        for (std::size_t a = 0, b = fi - 1; a < b; ++a, --b) { char t = f[a]; f[a] = f[b]; f[b] = t; }
        for (std::size_t j = 0; j < fi && i < sizeof(tmp) - 1; ++j) tmp[i++] = f[j];
    }
    tmp[i] = '\0';

    const std::size_t n = (i < sz - 1) ? i : sz - 1;
    for (std::size_t j = 0; j < n; ++j) buf[j] = tmp[j];
    buf[n] = '\0';
}

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100); }
    uart::write_line(debug, "[lps22hh] booting");

    auto bus = board::make_i2c();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[lps22hh] bus configure failed");
        halt_blink(100);
    }

    drv::Device sensor{bus};
    if (sensor.init().is_err()) {
        uart::write_line(debug, "[lps22hh] init failed");
        halt_blink(100);
    }
    uart::write_line(debug, "[lps22hh] init ok");

    auto m = sensor.read();
    if (m.is_err()) {
        uart::write_line(debug, "[lps22hh] read failed");
        halt_blink(100);
    }

    const auto& meas = m.unwrap();

    // Build output: "[lps22hh] pressure=XXXXX.XX hPa  temp=XX.XX C"
    char line[64] = "[lps22hh] pressure=";
    std::size_t pos = 19;

    char pnum[16]{};
    fmt_fixed(meas.pressure_hpa, 2, pnum, sizeof(pnum));
    for (std::size_t i = 0; pnum[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = pnum[i];
    const char* unit_p = " hPa  temp=";
    for (std::size_t i = 0; unit_p[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = unit_p[i];
    char tnum[16]{};
    fmt_fixed(meas.temperature_c, 2, tnum, sizeof(tnum));
    for (std::size_t i = 0; tnum[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = tnum[i];
    const char* unit_t = " C";
    for (std::size_t i = 0; unit_t[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = unit_t[i];
    line[pos] = '\0';

    uart::write_line(debug, line);
    uart::write_line(debug, "[lps22hh] PROBE PASS");
    halt_blink(500);
}
