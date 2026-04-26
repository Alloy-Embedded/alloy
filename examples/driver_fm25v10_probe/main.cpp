// examples/driver_fm25v10_probe/main.cpp
//
// SAME70 Xplained Ultra — FM25V10 Ferroelectric RAM driver probe.
//
// Wiring (FM25V10 → EXT1 header, SPI0, SW CS on PD25):
//   FM25V10 VDD  → 3V3    (EXT1 pin 20)
//   FM25V10 VSS  → GND    (EXT1 pin 19)
//   FM25V10 SI   → PD21   (EXT1 pin 16)   SPI0_MOSI
//   FM25V10 SO   → PD20   (EXT1 pin 17)   SPI0_MISO
//   FM25V10 SCK  → PD22   (EXT1 pin 18)   SPI0_SPCK
//   FM25V10 CS   → PD25   (EXT1 pin 15)   GPIO output, active-low
//   FM25V10 WP   → 3V3    (write-protect disabled)
//   FM25V10 HOLD → 3V3    (hold disabled)
//
// What the probe does, in order:
//   1. Init: RDID → check manufacturer byte == 0xC2.
//   2. Write pattern [0xAB, 0xCD] to address 0x00000.
//   3. Read 2 bytes from address 0x00000.
//   4. Compare: if bytes match → PROBE PASS; else → FAIL.
//
// Expected UART output:
//   [fm25v10] booting
//   [fm25v10] init ok
//   [fm25v10] write ok
//   [fm25v10] read: AB CD
//   [fm25v10] PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_fm25v10_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_SPI_HEADER
#    error "driver_fm25v10_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include "drivers/memory/fm25v10/fm25v10.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::memory::fm25v10;

// ── Helpers ───────────────────────────────────────────────────────────────────

[[noreturn]] static void halt_blink(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

/// Format a byte as two uppercase hex digits into buf[0..1]. No null terminator.
static void fmt_hex8(std::uint8_t v, char* buf) {
    constexpr auto kHex = "0123456789ABCDEF";
    buf[0] = kHex[(v >> 4u) & 0xFu];
    buf[1] = kHex[ v        & 0xFu];
}

/// Append a null-terminated string to buf starting at *pos, updating *pos.
static void append_str(char* buf, std::size_t sz, std::size_t* pos, const char* s) {
    while (*s != '\0' && *pos < sz - 1u) {
        buf[(*pos)++] = *s++;
    }
    buf[*pos] = '\0';
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    board::init();

    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) { halt_blink(100u); }
    uart::write_line(debug, "[fm25v10] booting");

    // Configure SPI bus (mode 0 or 3; 1 MHz safe default for bring-up).
    auto bus = board::make_spi();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[fm25v10] bus configure failed");
        halt_blink(100u);
    }

    // CS on PD25 — active-low GPIO managed by GpioCsPolicy.
    auto cs_pin = board::make_gpio_pd25();
    drv::GpioCsPolicy cs_policy{cs_pin};

    drv::Device dev{bus, cs_policy};

    if (dev.init().is_err()) {
        uart::write_line(debug, "[fm25v10] init failed (check wiring / RDID)");
        halt_blink(100u);
    }
    uart::write_line(debug, "[fm25v10] init ok");

    // Write two known bytes to address 0.
    constexpr std::array<std::uint8_t, 2> kPattern{0xABu, 0xCDu};
    if (dev.write(0x00000u, kPattern).is_err()) {
        uart::write_line(debug, "[fm25v10] write failed");
        halt_blink(100u);
    }
    uart::write_line(debug, "[fm25v10] write ok");

    // Read them back.
    std::array<std::uint8_t, 2> read_buf{};
    if (dev.read(0x00000u, read_buf).is_err()) {
        uart::write_line(debug, "[fm25v10] read failed");
        halt_blink(100u);
    }

    // Print: "[fm25v10] read: XX XX"
    {
        char line[32]{};
        std::size_t pos = 0u;
        append_str(line, sizeof(line), &pos, "[fm25v10] read: ");
        for (std::size_t i = 0u; i < read_buf.size(); ++i) {
            if (i > 0u) { append_str(line, sizeof(line), &pos, " "); }
            char hx[3]{ '0', '0', '\0' };
            fmt_hex8(read_buf[i], hx);
            hx[2] = '\0';
            append_str(line, sizeof(line), &pos, hx);
        }
        uart::write_line(debug, line);
    }

    // Verify the round-trip.
    if (read_buf[0] != kPattern[0] || read_buf[1] != kPattern[1]) {
        uart::write_line(debug, "[fm25v10] FAIL (data mismatch)");
        halt_blink(100u);
    }

    uart::write_line(debug, "[fm25v10] PROBE PASS");
    halt_blink(500u);
}
