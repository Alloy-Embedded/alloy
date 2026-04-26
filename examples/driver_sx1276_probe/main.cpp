// examples/driver_sx1276_probe/main.cpp
//
// SAME70 Xplained Ultra — SX1276 LoRa radio driver probe.
//
// Wiring (EXT1 header, SPI0 peripheral):
//   SX1276 VCC    → EXT1 pin 20 (VCC 3.3 V)
//   SX1276 GND    → EXT1 pin 19 (GND)
//   SX1276 SCK    → EXT1 pin 8  (SPI0_SPCK  / PD22)
//   SX1276 MISO   → EXT1 pin 9  (SPI0_MISO  / PD20)
//   SX1276 MOSI   → EXT1 pin 10 (SPI0_MOSI  / PD21)
//   SX1276 NSS    → EXT1 pin 15 (SPI0_CS0   / PD25)  — board-managed CS
//   SX1276 DIO0   → EXT1 pin 3  (PA6)                 — NOT CONNECTED in this probe
//   SX1276 RESET  → not connected; module boots from power-on reset
//
// DIO0 is the SX1276 "TxDone / RxDone" interrupt output. This seed driver does
// NOT use it; it polls RegIrqFlags over SPI exclusively. Wire DIO0 to a GPIO
// and add an edge-detection ISR at the application layer when lower latency is
// needed.
//
// What the probe does, in order:
//   1. Initialise debug UART (115200-8-N-1 via board HAL).
//   2. Bring up SPI0 via board::make_spi().
//   3. Call Device::init() — reads RegVersion (must be 0x12) and configures
//      the radio for 915 MHz / 125 kHz BW / SF7 / CR4-5 / +14 dBm.
//   4. Transmit the 5-byte ASCII string "Hello" (0x48 0x65 0x6C 0x6C 0x6F).
//   5. Attempt a single-shot receive into a 64-byte buffer. Because this probe
//      runs without a peer transmitter the receive will time-out after
//      ~10 M polling iterations and is treated as non-fatal — the TX path
//      already validated end-to-end SPI access.
//   6. Read the last-packet RSSI (meaningful only if a packet was received).
//   7. Print PROBE PASS and blink the user LED at 500 ms.
//
// Expected UART output (no peer present):
//
//   sx1276 probe: ready
//   sx1276: init ok (version=0x12)
//   sx1276: TX "Hello" ok
//   sx1276: RX timed out (no peer) -- SPI path ok
//   sx1276: RSSI = -157 dBm (no packet)
//   sx1276: PROBE PASS
//
// Expected UART output (peer transmitting on same frequency/SF/BW):
//
//   sx1276 probe: ready
//   sx1276: init ok (version=0x12)
//   sx1276: TX "Hello" ok
//   sx1276: RX ok, 5 bytes
//   sx1276: RSSI = -82 dBm
//   sx1276: PROBE PASS

#include <array>
#include <cstddef>
#include <cstdint>

#include BOARD_HEADER

#ifndef BOARD_UART_HEADER
#    error "driver_sx1276_probe requires BOARD_UART_HEADER for the selected board"
#endif
#ifndef BOARD_SPI_HEADER
#    error "driver_sx1276_probe requires BOARD_SPI_HEADER for the selected board"
#endif

#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include "drivers/net/sx1276/sx1276.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace {

namespace uart = alloy::examples::uart_console;
namespace drv  = alloy::drivers::net::sx1276;

// ── Small formatting helpers ──────────────────────────────────────────────────

template <typename Uart>
void print_hex_byte(const Uart& u, std::uint8_t v) {
    constexpr auto kHex = "0123456789ABCDEF";
    const char buf[3] = {kHex[(v >> 4u) & 0x0Fu], kHex[v & 0x0Fu], '\0'};
    uart::write_text(u, std::string_view{buf, 2u});
}

template <typename Uart>
void print_int16(const Uart& u, std::int16_t v) {
    // Handles -157..+20 dBm range without printf or heap.
    char buf[8]{};
    std::size_t pos = 0u;
    std::int32_t n = v;
    if (n < 0) {
        buf[pos++] = '-';
        n = -n;
    }
    if (n >= 100) { buf[pos++] = static_cast<char>('0' + n / 100); n %= 100; }
    if (n >= 10)  { buf[pos++] = static_cast<char>('0' + n /  10); n %=  10; }
    buf[pos++] = static_cast<char>('0' + n);
    uart::write_text(u, std::string_view{buf, pos});
}

// ── Lifecycle helpers ─────────────────────────────────────────────────────────

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

[[noreturn]] void blink_ok() {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500u);
    }
}

}  // namespace

int main() {
    board::init();

    // ── 1. Debug UART ─────────────────────────────────────────────────────────
    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) {
        blink_error(100u);
    }
    uart::write_line(debug, "sx1276 probe: ready");

    // ── 2. SPI bus ────────────────────────────────────────────────────────────
    auto bus = board::make_spi();
    if (bus.configure().is_err()) {
        uart::write_line(debug, "sx1276: FAIL (SPI configure)");
        blink_error(100u);
    }

    // ── 3. Driver init ────────────────────────────────────────────────────────
    // 915 MHz, 125 kHz BW, SF7, CR4/5, +14 dBm, CRC on.
    drv::Config cfg{};
    cfg.frequency_hz  = 915'000'000u;
    cfg.bandwidth     = drv::Bandwidth::kHz125;
    cfg.sf            = drv::SpreadingFactor::SF7;
    cfg.cr            = drv::CodingRate::CR4_5;
    cfg.tx_power_dbm  = 14u;
    cfg.crc_enable    = true;

    drv::Device<decltype(bus)> radio{bus, {}, cfg};

    if (auto r = radio.init(); r.is_err()) {
        uart::write_line(debug, "sx1276: FAIL (init — SPI fault or version mismatch)");
        blink_error(100u);
    }
    uart::write_text(debug, "sx1276: init ok (version=0x12)\r\n");

    // ── 4. Transmit "Hello" ───────────────────────────────────────────────────
    // 0x48='H' 0x65='e' 0x6C='l' 0x6C='l' 0x6F='o'
    constexpr std::array<std::uint8_t, 5u> kHello{
        0x48u, 0x65u, 0x6Cu, 0x6Cu, 0x6Fu
    };

    if (auto r = radio.transmit(std::span<const std::uint8_t>{kHello}); r.is_err()) {
        uart::write_line(debug, "sx1276: FAIL (transmit)");
        blink_error(100u);
    }
    uart::write_line(debug, "sx1276: TX \"Hello\" ok");

    // ── 5. Receive (non-fatal timeout) ────────────────────────────────────────
    std::array<std::uint8_t, 64u> rx_buf{};
    auto rx_result = radio.receive(std::span<std::uint8_t>{rx_buf});

    if (rx_result.is_err()) {
        const auto ec = rx_result.error();
        if (ec == alloy::core::ErrorCode::Timeout) {
            // No peer transmitting — this is expected in a solo seed probe.
            uart::write_line(debug, "sx1276: RX timed out (no peer) -- SPI path ok");
        } else {
            // CommunicationError (CRC) or SPI failure — report but continue.
            uart::write_line(debug, "sx1276: RX error (CRC or SPI fault)");
        }
    } else {
        const std::uint8_t n = rx_result.unwrap();
        uart::write_text(debug, "sx1276: RX ok, ");
        // Print byte count as single decimal digit (0..9; LoRa payload ≤ 255
        // so full formatting would need more chars, but keep it simple here).
        char cnt_buf[5]{};
        std::size_t pos = 0u;
        std::uint8_t cnt = n;
        if (cnt >= 100u) { cnt_buf[pos++] = static_cast<char>('0' + cnt / 100u); cnt %= 100u; }
        if (cnt >=  10u) { cnt_buf[pos++] = static_cast<char>('0' + cnt /  10u); cnt %=  10u; }
        cnt_buf[pos++] = static_cast<char>('0' + cnt);
        uart::write_text(debug, std::string_view{cnt_buf, pos});
        uart::write_line(debug, " bytes");
    }

    // ── 6. RSSI ───────────────────────────────────────────────────────────────
    auto rssi_result = radio.rssi();
    if (rssi_result.is_err()) {
        uart::write_line(debug, "sx1276: FAIL (rssi read)");
        blink_error(100u);
    }
    const std::int16_t rssi_dbm = rssi_result.unwrap();
    uart::write_text(debug, "sx1276: RSSI = ");
    print_int16(debug, rssi_dbm);
    uart::write_line(debug, " dBm");

    // ── 7. Done ───────────────────────────────────────────────────────────────
    uart::write_line(debug, "sx1276: PROBE PASS");
    blink_ok();
}
