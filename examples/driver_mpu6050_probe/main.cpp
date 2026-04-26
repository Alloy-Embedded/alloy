// examples/driver_mpu6050_probe/main.cpp
//
// SAME70 Xplained Ultra — MPU-6050 IMU driver probe.
//
// Wiring (MPU-6050 breakout → EXT1 header, I2C address 0x68, AD0 tied low):
//   MPU-6050 VCC  → 3V3   (EXT1 pin 20)
//   MPU-6050 GND  → GND   (EXT1 pin 19)
//   MPU-6050 SDA  → PA3   (EXT1 pin 11)   TWI0_SDA
//   MPU-6050 SCL  → PA4   (EXT1 pin 12)   TWI0_SCL
//   MPU-6050 AD0  → GND   (address 0x68)
//   MPU-6050 INT  → (leave unconnected for polling mode)
//
// Expected UART output:
//   [mpu6050] booting
//   [mpu6050] init ok
//   [mpu6050] accel x=+0.01 y=-0.00 z=+1.00 g  gyro x=+0.01 y=+0.00 z=-0.00 dps  temp=25.31 C
//   [mpu6050] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_mpu6050_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_I2C_HEADER
#    error "driver_mpu6050_probe requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "drivers/sensor/mpu6050/mpu6050.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::sensor::mpu6050;

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

// Format a float as a sign-prefixed fixed-point string (e.g. "+1.23" or "-0.07").
// |decimals| is the number of fractional digits. Result is null-terminated in buf.
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
    tmp[i++] = neg ? '-' : '+';

    // Whole part.
    char w[12]{};
    std::size_t wi = 0;
    std::uint32_t wv = whole;
    if (wv == 0) { w[wi++] = '0'; }
    while (wv > 0 && wi < sizeof(w) - 1) { w[wi++] = static_cast<char>('0' + wv % 10); wv /= 10; }
    for (std::size_t a = 0, b = wi - 1; a < b; ++a, --b) { char t = w[a]; w[a] = w[b]; w[b] = t; }
    for (std::size_t j = 0; j < wi && i < sizeof(tmp) - 1; ++j) tmp[i++] = w[j];

    if (decimals > 0) {
        tmp[i++] = '.';
        // Fractional part (zero-padded, reversed then corrected).
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

// Append a null-terminated string to buf starting at *pos, updating *pos.
static void append(char* buf, std::size_t sz, std::size_t* pos, const char* s) {
    for (std::size_t i = 0; s[i] != '\0' && *pos < sz - 1; ++i) {
        buf[(*pos)++] = s[i];
    }
    buf[*pos] = '\0';
}

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100); }
    uart::write_line(debug, "[mpu6050] booting");

    auto bus = board::make_i2c();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[mpu6050] bus configure failed");
        halt_blink(100);
    }

    drv::Config cfg{
        .address     = drv::kDefaultAddress,
        .accel_range = drv::AccelRange::G2,
        .gyro_range  = drv::GyroRange::Dps250,
    };
    drv::Device sensor{bus, cfg};

    if (sensor.init().is_err()) {
        uart::write_line(debug, "[mpu6050] init failed");
        halt_blink(100);
    }
    uart::write_line(debug, "[mpu6050] init ok");

    auto m = sensor.read();
    if (m.is_err()) {
        uart::write_line(debug, "[mpu6050] read failed");
        halt_blink(100);
    }

    const auto& meas = m.unwrap();

    // Build output line:
    // "[mpu6050] accel x=+X.XX y=+X.XX z=+X.XX g  gyro x=+X.XX y=+X.XX z=+X.XX dps  temp=XX.XX C"
    char line[128]{};
    std::size_t pos = 0;
    char num[16]{};

    append(line, sizeof(line), &pos, "[mpu6050] accel x=");
    fmt_fixed(meas.accel_x_g, 2, num, sizeof(num));
    append(line, sizeof(line), &pos, num);

    append(line, sizeof(line), &pos, " y=");
    fmt_fixed(meas.accel_y_g, 2, num, sizeof(num));
    append(line, sizeof(line), &pos, num);

    append(line, sizeof(line), &pos, " z=");
    fmt_fixed(meas.accel_z_g, 2, num, sizeof(num));
    append(line, sizeof(line), &pos, num);

    append(line, sizeof(line), &pos, " g  gyro x=");
    fmt_fixed(meas.gyro_x_dps, 2, num, sizeof(num));
    append(line, sizeof(line), &pos, num);

    append(line, sizeof(line), &pos, " y=");
    fmt_fixed(meas.gyro_y_dps, 2, num, sizeof(num));
    append(line, sizeof(line), &pos, num);

    append(line, sizeof(line), &pos, " z=");
    fmt_fixed(meas.gyro_z_dps, 2, num, sizeof(num));
    append(line, sizeof(line), &pos, num);

    append(line, sizeof(line), &pos, " dps  temp=");
    fmt_fixed(meas.temperature_c, 2, num, sizeof(num));
    append(line, sizeof(line), &pos, num);

    append(line, sizeof(line), &pos, " C");

    uart::write_line(debug, line);
    uart::write_line(debug, "[mpu6050] PROBE PASS");
    halt_blink(500);
}
