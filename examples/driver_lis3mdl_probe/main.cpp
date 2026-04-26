// examples/driver_lis3mdl_probe/main.cpp
//
// SAME70 Xplained Ultra — LIS3MDL 3-axis magnetometer probe.
//
// Wiring (LIS3MDL breakout → EXT1 header, I2C address 0x1C, SA1 tied to GND):
//   LIS3MDL VDD  → 3V3   (EXT1 pin 20)
//   LIS3MDL GND  → GND   (EXT1 pin 19)
//   LIS3MDL SDA  → PA3   (EXT1 pin 11)   TWI0_SDA
//   LIS3MDL SCL  → PA4   (EXT1 pin 12)   TWI0_SCL
//   LIS3MDL SA1  → GND   (address 0x1C)
//
// Expected UART output:
//   [lis3mdl] booting
//   [lis3mdl] init ok
//   [lis3mdl] x=X.XX G  y=X.XX G  z=X.XX G
//   [lis3mdl] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_lis3mdl_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_I2C_HEADER
#    error "driver_lis3mdl_probe requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "drivers/sensor/lis3mdl/lis3mdl.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::sensor::lis3mdl;

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

// Format a fixed-point float value into buf (null-terminated).
// Negative values are prefixed with '-'. The fractional part is zero-padded
// to `decimals` digits.
static void fmt_fixed(float value, unsigned decimals, char* buf, std::size_t sz) {
    if (sz == 0) return;
    bool neg = value < 0.0f;
    if (neg) value = -value;

    std::uint32_t mult = 1u;
    for (unsigned i = 0; i < decimals; ++i) mult *= 10u;

    const auto whole = static_cast<std::uint32_t>(value);
    const auto frac  = static_cast<std::uint32_t>(
        (value - static_cast<float>(whole)) * static_cast<float>(mult) + 0.5f);

    char tmp[32]{};
    std::size_t i = 0;
    if (neg) tmp[i++] = '-';

    // Write whole part (reversed then flipped).
    char w[12]{};
    std::size_t wi = 0;
    std::uint32_t wv = whole;
    if (wv == 0u) { w[wi++] = '0'; }
    while (wv > 0u && wi < sizeof(w) - 1u) {
        w[wi++] = static_cast<char>('0' + wv % 10u);
        wv /= 10u;
    }
    for (std::size_t a = 0, b = wi - 1; a < b; ++a, --b) {
        char t = w[a]; w[a] = w[b]; w[b] = t;
    }
    for (std::size_t j = 0; j < wi && i < sizeof(tmp) - 1; ++j) tmp[i++] = w[j];

    if (decimals > 0u) {
        tmp[i++] = '.';
        // Write fractional part (reversed then flipped, zero-padded).
        char f[12]{};
        std::size_t fi = 0;
        std::uint32_t fv = frac;
        for (unsigned d = 0; d < decimals; ++d) {
            f[fi++] = static_cast<char>('0' + fv % 10u);
            fv /= 10u;
        }
        for (std::size_t a = 0, b = fi - 1; a < b; ++a, --b) {
            char t = f[a]; f[a] = f[b]; f[b] = t;
        }
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
    uart::write_line(debug, "[lis3mdl] booting");

    auto bus = board::make_i2c();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[lis3mdl] bus configure failed");
        halt_blink(100);
    }

    // SA1 tied to GND → default address 0x1C, ±4 gauss full-scale.
    drv::Device sensor{bus};
    if (sensor.init().is_err()) {
        uart::write_line(debug, "[lis3mdl] init failed");
        halt_blink(100);
    }
    uart::write_line(debug, "[lis3mdl] init ok");

    auto m = sensor.read();
    if (m.is_err()) {
        uart::write_line(debug, "[lis3mdl] read failed");
        halt_blink(100);
    }

    const auto& meas = m.unwrap();

    // Build output: "[lis3mdl] x=X.XX G  y=X.XX G  z=X.XX G"
    char line[80] = "[lis3mdl] x=";
    std::size_t pos = 12;

    char num[16]{};

    fmt_fixed(meas.x_gauss, 2, num, sizeof(num));
    for (std::size_t i = 0; num[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = num[i];
    const char* sep_y = " G  y=";
    for (std::size_t i = 0; sep_y[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = sep_y[i];

    fmt_fixed(meas.y_gauss, 2, num, sizeof(num));
    for (std::size_t i = 0; num[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = num[i];
    const char* sep_z = " G  z=";
    for (std::size_t i = 0; sep_z[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = sep_z[i];

    fmt_fixed(meas.z_gauss, 2, num, sizeof(num));
    for (std::size_t i = 0; num[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = num[i];
    const char* unit = " G";
    for (std::size_t i = 0; unit[i] != '\0' && pos < sizeof(line) - 1; ++i) line[pos++] = unit[i];
    line[pos] = '\0';

    uart::write_line(debug, line);
    uart::write_line(debug, "[lis3mdl] PROBE PASS");
    halt_blink(500);
}
