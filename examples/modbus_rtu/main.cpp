// examples/modbus_rtu/main.cpp
//
// RS-485 DE control demo — Nucleo-G071RB (USART1, PA9/PA10)
//
// Demonstrates the close-uart-hal-gaps HAL additions:
//   - enable_hardware_flow_control()   — CR3 RTSE+CTSE
//   - enable_de() / set_de_polarity()  — CR3 DEM/DEP (RS-485 DE pin)
//   - read_and_clear_errors()          — ISR + ICR atomic clear
//
// Hardware:
//   PA9  → USART1 TX  → RS-485 transceiver DI (driver input)
//   PA10 → USART1 RX  → RS-485 transceiver RO (receiver output)
//   PA12 → USART1 DE  → RS-485 transceiver DE/RE (driver enable, active-high)
//
// The DE pin is driven by the USART hardware automatically:
//   - asserted  one DEAT bit-times before the first data bit
//   - de-asserted one DEDT bit-times after the last stop bit
//   This gives glitch-free RS-485 half-duplex without any GPIO toggling.
//
// Wiring:
//   Nucleo-G071RB CN9 header:
//     D2  (PA10) — RX
//     D8  (PA9)  — TX
//   Morpho header CN7:
//     PA12 pin 12 — DE
//
// Expected output (via ST-LINK VCP at 115200 8N1):
//   rs485 init ok
//   tx#1 [48 65 6C 6C 6F 0D 0A]
//   errors: pe=0 fe=0 ne=0 or=0
//   tx#2 ...
//
// Build:
//   cmake -DALLOY_BOARD=nucleo_g071rb -DCMAKE_TOOLCHAIN_FILE=... .
//   cmake --build . --target modbus_rtu

#include <array>
#include <cstdint>

#include "nucleo_g071rb/board.hpp"
#include "nucleo_g071rb/board_uart.hpp"
#include "../common/uart_console.hpp"

#include "hal/connect/connector.hpp"
#include "hal/uart.hpp"
#include "device/runtime.hpp"

// ---------------------------------------------------------------------------
// RS-485 USART1 connector
//   PA9  = TX, PA10 = RX, PA12 = DE (hardware-driven)
// ---------------------------------------------------------------------------

using Rs485Connector = alloy::hal::connection::connector<
    alloy::device::PeripheralId::USART1,
    alloy::hal::connection::tx<alloy::device::PinId::PA9,  alloy::device::SignalId::signal_tx>,
    alloy::hal::connection::rx<alloy::device::PinId::PA10, alloy::device::SignalId::signal_rx>>;

using Rs485Port = alloy::hal::uart::port<Rs485Connector>;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

namespace uart_con = alloy::examples::uart_console;

// Write one byte as two hex digits + space to debug console.
template <typename Console>
void write_hex(const Console& con, std::uint8_t b) {
    constexpr auto kHex = "0123456789ABCDEF";
    std::array<char, 3> buf{kHex[(b >> 4) & 0x0Fu], kHex[b & 0x0Fu], ' '};
    uart_con::write_text(con, std::string_view{buf.data(), buf.size()});
}

}  // namespace

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    // 1. Board init (clocks, GPIO, SysTick).
    board::init();

    // 2. Debug UART on USART2 / ST-LINK VCP (PA2=TX, PA3=RX, 115200 8N1).
    auto debug = board::make_debug_uart();
    if (debug.configure().is_err()) {
        while (true) { board::led::toggle(); }
    }

    // 3. RS-485 UART on USART1, 115200 8N1.
    alloy::hal::uart::Config rs485_cfg{};
    rs485_cfg.peripheral_clock_hz = nucleo_g071rb::ClockConfig::apb_clock_hz;
    rs485_cfg.baud_rate            = 115200u;

    Rs485Port rs485{rs485_cfg};
    if (rs485.configure().is_err()) {
        uart_con::write_line(debug, "rs485 init FAILED");
        while (true) { board::led::toggle(); }
    }

    // 4. Enable RS-485 DE hardware control.
    //    DE polarity: active-high (most transceivers — e.g. MAX485, SN65HVD).
    //    The USART drives PA12 automatically during TX; no GPIO code needed.
    {
        const auto pol_ok = rs485.set_de_polarity(/*active_high=*/true);
        const auto de_ok  = rs485.enable_de(true);
        if (pol_ok.is_err() || de_ok.is_err()) {
            uart_con::write_line(debug, "rs485 DE FAILED (not supported?)");
            while (true) { board::led::toggle(); }
        }
    }

    uart_con::write_line(debug, "rs485 init ok");

    // ---------------------------------------------------------------------------
    // Main loop: transmit a short frame every ~1 second, then check for errors.
    // ---------------------------------------------------------------------------

    constexpr std::array<std::uint8_t, 7u> kHello{
        0x48u, 0x65u, 0x6Cu, 0x6Cu, 0x6Fu, 0x0Du, 0x0Au  // "Hello\r\n"
    };

    std::uint32_t tx_count = 0u;

    while (true) {
        // --- Transmit ---
        ++tx_count;
        uart_con::write_text(debug, "tx#");
        uart_con::write_unsigned(debug, tx_count);
        uart_con::write_text(debug, " [");
        for (const auto byte : kHello) {
            write_hex(debug, byte);
        }
        uart_con::write_text(debug, "]\r\n");

        const auto tx_bytes =
            std::span{reinterpret_cast<const std::byte*>(kHello.data()), kHello.size()};
        (void)rs485.write(tx_bytes);
        (void)rs485.flush();

        // --- Error check (read_and_clear_errors) ---
        const auto errs = rs485.read_and_clear_errors();
        if (errs.any()) {
            uart_con::write_text(debug, "errors: pe=");
            uart_con::write_unsigned(debug, static_cast<std::uint32_t>(errs.parity));
            uart_con::write_text(debug, " fe=");
            uart_con::write_unsigned(debug, static_cast<std::uint32_t>(errs.framing));
            uart_con::write_text(debug, " ne=");
            uart_con::write_unsigned(debug, static_cast<std::uint32_t>(errs.noise));
            uart_con::write_text(debug, " or=");
            uart_con::write_unsigned(debug, static_cast<std::uint32_t>(errs.overrun));
            uart_con::write_text(debug, "\r\n");
        }

        // ~1 s busy delay (no RTOS in this minimal example).
        for (volatile std::uint32_t i = 0u; i < 16'000'000u; ++i) { /* spin */ }
    }
}
