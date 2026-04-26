// examples/driver_lsm6dsox_probe/main.cpp
//
// SAME70 Xplained Ultra — LSM6DSOX 6-axis IMU (accel + gyro) probe.
//
// Wiring (LSM6DSOX breakout → EXT1 header, I2C address 0x6A, SA0 tied low):
//   LSM6DSOX VDD  → 3V3   (EXT1 pin 20)
//   LSM6DSOX GND  → GND   (EXT1 pin 19)
//   LSM6DSOX SDA  → PA3   (EXT1 pin 11)   TWI0_SDA
//   LSM6DSOX SCL  → PA4   (EXT1 pin 12)   TWI0_SCL
//   LSM6DSOX SA0  → GND   (address 0x6A)
//   LSM6DSOX CS   → VDD   (selects I2C mode)
//
// Expected UART output:
//   [lsm6dsox] booting
//   [lsm6dsox] init ok
//   [lsm6dsox] ax=X.XX g  ay=X.XX g  az=X.XX g
//   [lsm6dsox] gx=X.XX dps  gy=X.XX dps  gz=X.XX dps
//   [lsm6dsox] temp=XX.XX C
//   [lsm6dsox] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_lsm6dsox_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_I2C_HEADER
#    error "driver_lsm6dsox_probe requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "drivers/sensor/lsm6dsox/lsm6dsox.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::sensor::lsm6dsox;

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

/// Format a float as a fixed-point decimal string into buf (null-terminated).
/// Handles negative values; zero-pads the fractional part to `decimals` digits.
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

    // Whole part (reverse-digit trick).
    char w[12]{};
    std::size_t wi = 0;
    std::uint32_t wv = whole;
    if (wv == 0) { w[wi++] = '0'; }
    while (wv > 0 && wi < sizeof(w) - 1) { w[wi++] = static_cast<char>('0' + wv % 10); wv /= 10; }
    for (std::size_t a = 0, b = wi - 1; a < b; ++a, --b) { char t = w[a]; w[a] = w[b]; w[b] = t; }
    for (std::size_t j = 0; j < wi && i < sizeof(tmp) - 1; ++j) tmp[i++] = w[j];

    if (decimals > 0) {
        tmp[i++] = '.';
        char f[12]{};
        std::size_t fi = 0;
        std::uint32_t fv = frac;
        for (unsigned d = 0; d < decimals; ++d) { f[fi++] = static_cast<char>('0' + fv % 10); fv /= 10; }
        for (std::size_t a = 0, b = fi - 1; a < b; ++a, --b) { char t = f[a]; f[a] = f[b]; f[b] = t; }
        for (std::size_t j = 0; j < fi && i < sizeof(tmp) - 1; ++j) tmp[i++] = f[j];
    }
    tmp[i] = '\0';

    const std::size_t n = (i < sz - 1) ? i : sz - 1;
    for (std::size_t j = 0; j < n; ++j) buf[j] = tmp[j];
    buf[n] = '\0';
}

/// Append a C-string to buf[pos..sz-1]; advances pos past the appended chars.
static void append(char* buf, std::size_t sz, std::size_t& pos, const char* s) {
    for (std::size_t i = 0; s[i] != '\0' && pos < sz - 1; ++i) buf[pos++] = s[i];
}

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100); }
    uart::write_line(debug, "[lsm6dsox] booting");

    auto bus = board::make_i2c();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[lsm6dsox] bus configure failed");
        halt_blink(100);
    }

    drv::Device sensor{bus, {.address  = drv::kDefaultAddress,
                              .accel_fs = drv::AccelFullScale::G2,
                              .gyro_fs  = drv::GyroFullScale::Dps250}};

    if (sensor.init().is_err()) {
        uart::write_line(debug, "[lsm6dsox] init failed");
        halt_blink(100);
    }
    uart::write_line(debug, "[lsm6dsox] init ok");

    auto m = sensor.read();
    if (m.is_err()) {
        uart::write_line(debug, "[lsm6dsox] read failed");
        halt_blink(100);
    }

    const auto& meas = m.unwrap();
    char num[16]{};
    char line[80]{};
    std::size_t pos;

    // Print accelerometer values.
    pos = 0;
    append(line, sizeof(line), pos, "[lsm6dsox] ax=");
    fmt_fixed(meas.accel_x_g, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " g  ay=");
    fmt_fixed(meas.accel_y_g, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " g  az=");
    fmt_fixed(meas.accel_z_g, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " g");
    line[pos] = '\0';
    uart::write_line(debug, line);

    // Print gyroscope values.
    pos = 0;
    append(line, sizeof(line), pos, "[lsm6dsox] gx=");
    fmt_fixed(meas.gyro_x_dps, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " dps  gy=");
    fmt_fixed(meas.gyro_y_dps, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " dps  gz=");
    fmt_fixed(meas.gyro_z_dps, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " dps");
    line[pos] = '\0';
    uart::write_line(debug, line);

    // Print temperature.
    pos = 0;
    append(line, sizeof(line), pos, "[lsm6dsox] temp=");
    fmt_fixed(meas.temperature_c, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " C");
    line[pos] = '\0';
    uart::write_line(debug, line);

    uart::write_line(debug, "[lsm6dsox] PROBE PASS");
    halt_blink(500);
}
