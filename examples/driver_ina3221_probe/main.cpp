// examples/driver_ina3221_probe/main.cpp
//
// SAME70 Xplained Ultra — INA3221 triple-channel power monitor probe.
//
// Wiring (INA3221 breakout → EXT1 header):
//   INA3221 VCC  → 3V3   (EXT1 pin 20)
//   INA3221 GND  → GND   (EXT1 pin 19)
//   INA3221 SDA  → PA3   (EXT1 pin 11)   TWI0_SDA
//   INA3221 SCL  → PA4   (EXT1 pin 12)   TWI0_SCL
//   INA3221 A0   → GND   (I2C address 0x40)
//   0.1Ω shunts on IN1+/IN1−, IN2+/IN2−, IN3+/IN3−.
//
// Expected UART output:
//   [ina3221] booting
//   [ina3221] init ok
//   [ina3221] ch1: bus=X.XX V  current=X.XX A
//   [ina3221] ch2: bus=X.XX V  current=X.XX A
//   [ina3221] ch3: bus=X.XX V  current=X.XX A
//   [ina3221] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_ina3221_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_I2C_HEADER
#    error "driver_ina3221_probe requires BOARD_I2C_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_I2C_HEADER

#include "drivers/power/ina3221/ina3221.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::power::ina3221;

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

// Print one channel measurement line: "[ina3221] chN: bus=X.XX V  current=X.XX A"
static void print_channel(auto& debug, unsigned ch_num,
                           const drv::ChannelMeasurement& ch) {
    char line[64]{};
    std::size_t pos = 0;
    append(line, sizeof(line), pos, "[ina3221] ch");
    line[pos++] = static_cast<char>('0' + ch_num);
    append(line, sizeof(line), pos, ": bus=");

    char num[16]{};
    fmt_fixed(ch.bus_voltage_v, 2, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " V  current=");
    fmt_fixed(ch.current_a, 3, num, sizeof(num));
    append(line, sizeof(line), pos, num);
    append(line, sizeof(line), pos, " A");
    line[pos] = '\0';

    uart::write_line(debug, line);
}

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100); }
    uart::write_line(debug, "[ina3221] booting");

    auto bus = board::make_i2c();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[ina3221] bus configure failed");
        halt_blink(100);
    }

    // Default config: address=0x40, shunt=0.1Ω.
    drv::Device sensor{bus};
    if (sensor.init().is_err()) {
        uart::write_line(debug, "[ina3221] init failed");
        halt_blink(100);
    }
    uart::write_line(debug, "[ina3221] init ok");

    auto m = sensor.read();
    if (m.is_err()) {
        uart::write_line(debug, "[ina3221] read failed");
        halt_blink(100);
    }

    const auto& meas = m.unwrap();
    print_channel(debug, 1, meas.ch[0]);
    print_channel(debug, 2, meas.ch[1]);
    print_channel(debug, 3, meas.ch[2]);

    uart::write_line(debug, "[ina3221] PROBE PASS");
    halt_blink(500);
}
