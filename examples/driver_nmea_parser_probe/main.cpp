// examples/driver_nmea_parser_probe/main.cpp
//
// SAME70 Xplained Ultra — NMEA-0183 GPS parser probe.
//
// Wiring note:
//   This probe uses BOARD_UART_HEADER for both debug output and GPS input.
//   In a real deployment you would wire the GPS receiver TX line to a *second*
//   UART peripheral (e.g. USART1 on EXT1 pin 13/14) and use that bus handle
//   with alloy::drivers::net::nmea_parser::Parser.  A separate GPS UART bus
//   has been omitted here to keep the probe board-agnostic; swap
//   `gps_uart` for the real peripheral handle when doing hardware validation.
//
// Expected UART output (example, values depend on GPS fix):
//   [nmea] booting
//   [nmea] waiting for fix...
//   [nmea] GGA: 14:32:10  lat=37.422  lon=-122.084  sats=9  alt=5.3m  q=1
//   [nmea] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_nmea_parser_probe requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER

#include "drivers/net/nmea_parser/nmea_parser.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::net::nmea_parser;

// ── Helpers ────────────────────────────────────────────────────────────────────

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

/// Append a null-terminated string to buf[] starting at *pos (in-place).
/// Never writes past sz bytes (including the final null terminator).
static void append_str(char* buf, std::size_t sz, std::size_t* pos, const char* s) {
    while (s != nullptr && *s != '\0' && *pos + 1u < sz) {
        buf[(*pos)++] = *s++;
    }
}

/// Append an unsigned integer (decimal) to buf[].
static void append_uint(char* buf, std::size_t sz, std::size_t* pos, std::uint32_t v) {
    char tmp[12]{};
    std::size_t ti = 0u;
    if (v == 0u) {
        tmp[ti++] = '0';
    } else {
        while (v > 0u && ti < sizeof(tmp) - 1u) {
            tmp[ti++] = static_cast<char>('0' + v % 10u);
            v /= 10u;
        }
        // reverse
        for (std::size_t a = 0u, b = ti - 1u; a < b; ++a, --b) {
            char t = tmp[a]; tmp[a] = tmp[b]; tmp[b] = t;
        }
    }
    for (std::size_t i = 0u; i < ti; ++i) {
        if (*pos + 1u < sz) buf[(*pos)++] = tmp[i];
    }
}

/// Append a float as "III.DDD" (3 decimal places) to buf[].
static void append_float(char* buf, std::size_t sz, std::size_t* pos, float v) {
    if (v < 0.0f) {
        if (*pos + 1u < sz) buf[(*pos)++] = '-';
        v = -v;
    }
    const auto whole = static_cast<std::uint32_t>(v);
    const auto frac  = static_cast<std::uint32_t>((v - static_cast<float>(whole)) * 1000.0f + 0.5f);
    append_uint(buf, sz, pos, whole);
    if (*pos + 1u < sz) buf[(*pos)++] = '.';
    // Zero-pad to 3 digits.
    char f[4]{};
    f[0] = static_cast<char>('0' + (frac / 100u) % 10u);
    f[1] = static_cast<char>('0' + (frac / 10u)  % 10u);
    f[2] = static_cast<char>('0' + frac           % 10u);
    f[3] = '\0';
    append_str(buf, sz, pos, f);
}

/// Append a two-digit zero-padded uint8 to buf[].
static void append_u8_2digit(char* buf, std::size_t sz, std::size_t* pos, std::uint8_t v) {
    if (*pos + 1u < sz) buf[(*pos)++] = static_cast<char>('0' + v / 10u);
    if (*pos + 1u < sz) buf[(*pos)++] = static_cast<char>('0' + v % 10u);
}

// ── main ───────────────────────────────────────────────────────────────────────

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100); }
    uart::write_line(debug, "[nmea] booting");

    // In hardware: replace `debug` below with a second UART handle wired to
    // the GPS receiver's TX pin (e.g. board::make_gps_uart()).
    // Here we reuse the debug UART so the probe compiles on any board.
    auto& gps_uart = debug;  // NOTE: swap for a real GPS UART in production

    drv::Parser parser{gps_uart};

    uart::write_line(debug, "[nmea] waiting for fix...");

    bool gga_pass = false;

    // Read lines indefinitely. On real hardware the GPS module streams NMEA
    // at 1 Hz; the loop will naturally block in read_line() between sentences.
    while (true) {
        if (parser.read_line().is_err()) {
            // UART glitch — skip and retry.
            continue;
        }

        // Try GGA.
        if (auto gga = parser.parse_gga(); gga.is_ok()) {
            const auto& f = gga.unwrap();

            char line[128]{};
            std::size_t pos = 0u;
            append_str(line, sizeof(line), &pos, "[nmea] GGA: ");
            append_u8_2digit(line, sizeof(line), &pos, f.hour);
            append_str(line, sizeof(line), &pos, ":");
            append_u8_2digit(line, sizeof(line), &pos, f.minute);
            append_str(line, sizeof(line), &pos, ":");
            append_u8_2digit(line, sizeof(line), &pos, f.second);
            append_str(line, sizeof(line), &pos, "  lat=");
            append_float(line, sizeof(line), &pos, f.latitude);
            append_str(line, sizeof(line), &pos, "  lon=");
            append_float(line, sizeof(line), &pos, f.longitude);
            append_str(line, sizeof(line), &pos, "  sats=");
            append_uint(line, sizeof(line), &pos, f.satellites);
            append_str(line, sizeof(line), &pos, "  alt=");
            append_float(line, sizeof(line), &pos, f.altitude_m);
            append_str(line, sizeof(line), &pos, "m  q=");
            append_uint(line, sizeof(line), &pos, f.fix_quality);
            line[pos] = '\0';
            uart::write_line(debug, std::string_view{line, pos});

            if (!gga_pass && f.fix_quality > 0u && f.satellites > 0u) {
                gga_pass = true;
                uart::write_line(debug, "[nmea] PROBE PASS");
                halt_blink(500);
            }
            continue;
        }

        // Try RMC.
        if (auto rmc = parser.parse_rmc(); rmc.is_ok()) {
            const auto& f = rmc.unwrap();

            char line[128]{};
            std::size_t pos = 0u;
            append_str(line, sizeof(line), &pos, "[nmea] RMC: ");
            append_u8_2digit(line, sizeof(line), &pos, f.hour);
            append_str(line, sizeof(line), &pos, ":");
            append_u8_2digit(line, sizeof(line), &pos, f.minute);
            append_str(line, sizeof(line), &pos, ":");
            append_u8_2digit(line, sizeof(line), &pos, f.second);
            append_str(line, sizeof(line), &pos, f.valid ? "  VALID" : "  VOID");
            append_str(line, sizeof(line), &pos, "  lat=");
            append_float(line, sizeof(line), &pos, f.latitude);
            append_str(line, sizeof(line), &pos, "  lon=");
            append_float(line, sizeof(line), &pos, f.longitude);
            append_str(line, sizeof(line), &pos, "  spd=");
            append_float(line, sizeof(line), &pos, f.speed_knots);
            append_str(line, sizeof(line), &pos, "kn  crs=");
            append_float(line, sizeof(line), &pos, f.course_deg);
            line[pos] = '\0';
            uart::write_line(debug, std::string_view{line, pos});
            continue;
        }

        // Unknown or malformed sentence — silently discard.
    }
}
