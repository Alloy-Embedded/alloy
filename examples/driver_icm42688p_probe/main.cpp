// examples/driver_icm42688p_probe/main.cpp
//
// SAME70 Xplained Ultra — ICM-42688-P 6-axis IMU driver probe.
//
// Wiring (ICM-42688-P breakout → EXT1 header, SPI0, SW CS on PD25):
//   ICM VDD   → 3V3    (EXT1 pin 20)
//   ICM GND   → GND    (EXT1 pin 19)
//   ICM MOSI  → PD21   (EXT1 pin 16)   SPI0_MOSI
//   ICM MISO  → PD20   (EXT1 pin 17)   SPI0_MISO
//   ICM SCLK  → PD22   (EXT1 pin 18)   SPI0_SPCK
//   ICM CS    → PD25   (EXT1 pin 15)   GPIO output, active-low
//   ICM INT1  → (not connected for this probe)
//
// Expected UART output:
//   [icm42688p] booting
//   [icm42688p] init ok
//   [icm42688p] accel x=XX.XX y=XX.XX z=XX.XX g
//   [icm42688p] gyro  x=XX.XX y=XX.XX z=XX.XX dps
//   [icm42688p] temp=XX.XX C
//   [icm42688p] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_icm42688p_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_SPI_HEADER
#    error "driver_icm42688p_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include "drivers/sensor/icm42688p/icm42688p.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::sensor::icm42688p;

// ── Helpers ───────────────────────────────────────────────────────────────────

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

/// Format a float as "IIIII.FF" into buf (null-terminated, `decimals` places).
static void fmt_fixed(float value, unsigned decimals, char* buf, std::size_t sz) {
    if (sz == 0u) return;
    bool neg = value < 0.0f;
    if (neg) value = -value;

    std::uint32_t mult = 1u;
    for (unsigned i = 0u; i < decimals; ++i) mult *= 10u;

    const auto whole = static_cast<std::uint32_t>(value);
    const auto frac  = static_cast<std::uint32_t>(
        (value - static_cast<float>(whole)) * static_cast<float>(mult) + 0.5f);

    char tmp[32]{};
    std::size_t pos = 0u;
    if (neg) tmp[pos++] = '-';

    // Integer part.
    char w[12]{};
    std::size_t wi = 0u;
    std::uint32_t wv = whole;
    if (wv == 0u) { w[wi++] = '0'; }
    while (wv > 0u && wi < sizeof(w) - 1u) { w[wi++] = static_cast<char>('0' + wv % 10u); wv /= 10u; }
    for (std::size_t a = 0u, b = wi - 1u; a < b; ++a, --b) { char t = w[a]; w[a] = w[b]; w[b] = t; }
    for (std::size_t j = 0u; j < wi && pos < sizeof(tmp) - 1u; ++j) tmp[pos++] = w[j];

    if (decimals > 0u) {
        tmp[pos++] = '.';
        char f[12]{};
        std::size_t fi = 0u;
        std::uint32_t fv = frac;
        for (unsigned d = 0u; d < decimals; ++d) { f[fi++] = static_cast<char>('0' + fv % 10u); fv /= 10u; }
        for (std::size_t a = 0u, b = fi - 1u; a < b; ++a, --b) { char t = f[a]; f[a] = f[b]; f[b] = t; }
        for (std::size_t j = 0u; j < fi && pos < sizeof(tmp) - 1u; ++j) tmp[pos++] = f[j];
    }
    tmp[pos] = '\0';

    const std::size_t n = (pos < sz - 1u) ? pos : sz - 1u;
    for (std::size_t j = 0u; j < n; ++j) buf[j] = tmp[j];
    buf[n] = '\0';
}

/// Append a null-terminated string into buf starting at *pos, updating *pos.
static void append_str(char* buf, std::size_t sz, std::size_t* pos, const char* s) {
    while (*s != '\0' && *pos < sz - 1u) {
        buf[(*pos)++] = *s++;
    }
    buf[*pos] = '\0';
}

/// Append a float formatted to `decimals` places.
static void append_float(char* buf, std::size_t sz, std::size_t* pos,
                          float v, unsigned decimals) {
    char tmp[24]{};
    fmt_fixed(v, decimals, tmp, sizeof(tmp));
    append_str(buf, sz, pos, tmp);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100u); }
    uart::write_line(debug, "[icm42688p] booting");

    // Configure SPI bus (mode 0 or 3; 1 MHz safe default for bring-up).
    auto bus = board::make_spi();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[icm42688p] bus configure failed");
        halt_blink(100u);
    }

    // CS on PD25 — active-low GPIO managed by GpioCsPolicy.
    auto cs_pin = board::make_gpio_pd25();
    drv::GpioCsPolicy cs_policy{cs_pin};

    drv::Device sensor{bus, cs_policy};

    if (sensor.init().is_err()) {
        uart::write_line(debug, "[icm42688p] init failed");
        halt_blink(100u);
    }
    uart::write_line(debug, "[icm42688p] init ok");

    auto m = sensor.read();
    if (m.is_err()) {
        uart::write_line(debug, "[icm42688p] read failed");
        halt_blink(100u);
    }

    const auto& meas = m.unwrap();

    // Print: "[icm42688p] accel x=XX.XX y=XX.XX z=XX.XX g"
    {
        char line[80]{};
        std::size_t pos = 0u;
        append_str(line, sizeof(line), &pos, "[icm42688p] accel x=");
        append_float(line, sizeof(line), &pos, meas.accel_x_g, 2u);
        append_str(line, sizeof(line), &pos, " y=");
        append_float(line, sizeof(line), &pos, meas.accel_y_g, 2u);
        append_str(line, sizeof(line), &pos, " z=");
        append_float(line, sizeof(line), &pos, meas.accel_z_g, 2u);
        append_str(line, sizeof(line), &pos, " g");
        uart::write_line(debug, line);
    }

    // Print: "[icm42688p] gyro  x=XX.XX y=XX.XX z=XX.XX dps"
    {
        char line[80]{};
        std::size_t pos = 0u;
        append_str(line, sizeof(line), &pos, "[icm42688p] gyro  x=");
        append_float(line, sizeof(line), &pos, meas.gyro_x_dps, 2u);
        append_str(line, sizeof(line), &pos, " y=");
        append_float(line, sizeof(line), &pos, meas.gyro_y_dps, 2u);
        append_str(line, sizeof(line), &pos, " z=");
        append_float(line, sizeof(line), &pos, meas.gyro_z_dps, 2u);
        append_str(line, sizeof(line), &pos, " dps");
        uart::write_line(debug, line);
    }

    // Print: "[icm42688p] temp=XX.XX C"
    {
        char line[48]{};
        std::size_t pos = 0u;
        append_str(line, sizeof(line), &pos, "[icm42688p] temp=");
        append_float(line, sizeof(line), &pos, meas.temperature_c, 2u);
        append_str(line, sizeof(line), &pos, " C");
        uart::write_line(debug, line);
    }

    uart::write_line(debug, "[icm42688p] PROBE PASS");
    halt_blink(500u);
}
