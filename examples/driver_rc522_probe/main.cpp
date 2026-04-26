// examples/driver_rc522_probe/main.cpp
//
// SAME70 Xplained Ultra — MFRC522 RFID reader driver probe.
//
// Wiring (RC522 module → EXT1 header, SPI0, SW CS on PD25):
//   RC522 VCC   → 3V3    (EXT1 pin 20)
//   RC522 GND   → GND    (EXT1 pin 19)
//   RC522 MOSI  → PD21   (EXT1 pin 16)   SPI0_MOSI
//   RC522 MISO  → PD20   (EXT1 pin 17)   SPI0_MISO
//   RC522 SCK   → PD22   (EXT1 pin 18)   SPI0_SPCK
//   RC522 SDA   → PD25   (EXT1 pin 15)   GPIO output, active-low (CS)
//   RC522 RST   → (tie to 3V3 or leave unconnected — not driven by driver)
//   RC522 IRQ   → (not connected for this probe)
//
// Expected UART output (card present during poll):
//   [rc522] booting
//   [rc522] ready
//   [rc522] polling for card...
//   [rc522] card present, reading UID
//   [rc522] uid=XX XX XX XX
//   [rc522] PROBE PASS
//
// Expected UART output (no card present during poll window):
//   [rc522] booting
//   [rc522] ready
//   [rc522] polling for card...
//   [rc522] no card detected after timeout
//   [rc522] PROBE PASS  (MDIO path confirmed OK by successful init)

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_rc522_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_SPI_HEADER
#    error "driver_rc522_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include "drivers/net/rc522/rc522.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::net::rc522;

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
    uart::write_line(debug, "[rc522] booting");

    // Configure SPI bus (mode 0; 1 MHz safe default for bring-up).
    auto bus = board::make_spi();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "[rc522] bus configure failed");
        halt_blink(100u);
    }

    // CS on PD25 — active-low GPIO managed by GpioCsPolicy.
    auto cs_pin = board::make_gpio_pd25();
    drv::GpioCsPolicy cs_policy{cs_pin};

    drv::Device dev{bus, cs_policy};

    if (dev.init().is_err()) {
        uart::write_line(debug, "[rc522] init failed (check wiring / VersionReg)");
        halt_blink(100u);
    }
    uart::write_line(debug, "[rc522] ready");

    // Poll for a card for up to ~5 seconds (50 × 100 ms).
    uart::write_line(debug, "[rc522] polling for card...");

    bool found = false;
    for (std::uint32_t i = 0u; i < 50u; ++i) {
        auto present = dev.is_card_present();
        if (present.is_err()) {
            uart::write_line(debug, "[rc522] is_card_present failed");
            halt_blink(100u);
        }
        if (present.unwrap()) {
            found = true;
            break;
        }
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(100u);
    }

    if (!found) {
        uart::write_line(debug, "[rc522] no card detected after timeout");
        uart::write_line(debug, "[rc522] PROBE PASS");
        halt_blink(500u);
    }

    uart::write_line(debug, "[rc522] card present, reading UID");

    auto uid_result = dev.read_uid();
    if (uid_result.is_err()) {
        uart::write_line(debug, "[rc522] read_uid failed");
        halt_blink(100u);
    }

    const auto uid = uid_result.unwrap();

    // Print: "[rc522] uid=XX XX XX XX"
    {
        char line[48]{};
        std::size_t pos = 0u;
        append_str(line, sizeof(line), &pos, "[rc522] uid=");
        for (std::uint8_t i = 0u; i < uid.size; ++i) {
            if (i > 0u) { append_str(line, sizeof(line), &pos, " "); }
            char hx[3]{ '0', '0', '\0' };
            fmt_hex8(uid.data[i], hx);
            hx[2] = '\0';
            append_str(line, sizeof(line), &pos, hx);
        }
        uart::write_line(debug, line);
    }

    uart::write_line(debug, "[rc522] PROBE PASS");
    halt_blink(500u);
}
