// examples/driver_max17048_probe/main.cpp
//
// SAME70 Xplained Ultra — MAX17048 LiPo fuel gauge probe.
//
// Wiring (MAX17048 breakout → EXT1 header):
//   MAX17048 VCC  → 3V3   (EXT1 pin 20)
//   MAX17048 GND  → GND   (EXT1 pin 19)
//   MAX17048 SDA  → PA3   (EXT1 pin 11)   TWI0_SDA
//   MAX17048 SCL  → PA4   (EXT1 pin 12)   TWI0_SCL
//   LiPo cell connected to CELL+/CELL− on the breakout.
//
// Expected UART output:
//   [max17048] booting
//   [max17048] init ok
//   [max17048] voltage=X.XX V  soc=XX.XX %  rate=X.XX %/hr
//   [max17048] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_max17048_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_I2C_HEADER
#    error "driver_max17048_probe requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "drivers/power/max17048/max17048.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::power::max17048;

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

// Format a fixed-point float into buf (null-terminated).
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

// Append a null-terminated string into line[pos..], updating pos.
static void append(char* line, std::size_t sz, std::size_t& pos, const char* str) {
    for (std::size_t i = 0; str[i] != '\0' && pos < sz - 1; ++i) line[pos++] = str[i];
}

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100); }
    uart::write_line(debug, "[max17048] booting");

    auto bus = board::make_i2c();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[max17048] bus configure failed");
        halt_blink(100);
    }

    drv::Device sensor{bus};
    if (sensor.init().is_err()) {
        uart::write_line(debug, "[max17048] init failed");
        halt_blink(100);
    }
    uart::write_line(debug, "[max17048] init ok");

    auto m = sensor.read();
    if (m.is_err()) {
        uart::write_line(debug, "[max17048] read failed");
        halt_blink(100);
    }

    const auto& meas = m.unwrap();

    // Build: "[max17048] voltage=X.XX V  soc=XX.XX %  rate=X.XX %/hr"
    char line[80] = "[max17048] voltage=";
    std::size_t pos = 19;

    char num[16]{};
    fmt_fixed(meas.voltage_v, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " V  soc=");
    fmt_fixed(meas.soc_pct, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " %  rate=");
    fmt_fixed(meas.charge_rate, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " %/hr");
    line[pos] = '\0';

    uart::write_line(debug, line);
    uart::write_line(debug, "[max17048] PROBE PASS");
    halt_blink(500);
}
