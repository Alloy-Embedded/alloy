// uart_probe_complete — exercises every new Phase 1-3 UART HAL lever.
//
// Target: nucleo_g071rb (primary), nucleo_f401re (secondary).
//
// What this example demonstrates on USART2 (connected to ST-Link):
//   Phase 1: set_baudrate(921600) with ±2% validation, kernel_clock_hz()
//   Phase 2: FIFO enable + RX/TX threshold, status flag reads, interrupt arm/disarm
//   Phase 3: LIN enable + break, RS-485 DE assertion times, half-duplex,
//             smartcard / IrDA (both disabled — just exercising the lever)
//   Async:   async::uart::wait_for<IdleLine> — idle-line detection pattern
//
// All "NotSupported" returns are expected for features absent on the
// selected peripheral (e.g. FIFO absent on F4 USART2, LIN present on both).
// The example prints a one-line verdict per feature over the debug UART.
//
// Hardware needed: USB-to-ST-Link cable only.  No loopback jumper required
// for the print path.  A loopback from PA2 to PA3 reveals the RX status flags.

#include BOARD_HEADER
#include BOARD_UART_HEADER

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/error_code.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"
#include "hal/uart.hpp"

namespace {

namespace console = alloy::examples::uart_console;
using alloy::hal::uart::FifoTrigger;
using alloy::hal::uart::InterruptKind;
using alloy::hal::uart::Oversampling;

// ── helpers ──────────────────────────────────────────────────────────────────

template <typename Uart>
void report(Uart& port, const char* label,
            const alloy::core::Result<void, alloy::core::ErrorCode>& result) {
    console::write_text(port, label);
    if (result.is_ok()) {
        console::write_line(port, " OK");
    } else {
        console::write_text(port, " NotSupported(");
        console::write_unsigned(port, static_cast<std::uint32_t>(result.err()));
        console::write_line(port, ")");
    }
}

template <typename Uart>
void report_bool(Uart& port, const char* label, bool value) {
    console::write_text(port, label);
    console::write_line(port, value ? " true" : " false");
}

// ── phase 1 ──────────────────────────────────────────────────────────────────

template <typename Uart>
void probe_baudrate(Uart& port) {
    console::write_line(port, "--- Phase 1: baudrate / oversampling ---");

    // kernel_clock_hz(): always available, read from config.
    console::write_text(port, "kernel_clock_hz = ");
    console::write_unsigned(port, port.kernel_clock_hz());
    console::write_line(port, " Hz");

    // set_oversampling X8 (may reduce jitter at high baud rates).
    report(port, "set_oversampling(X8)", port.set_oversampling(Oversampling::X8));

    // set_baudrate: 921600 — returns OutOfRange when ±2% cannot be met.
    report(port, "set_baudrate(921600)", port.set_baudrate(921'600u));

    // Restore to 115200 so subsequent prints still work.
    static_cast<void>(port.set_oversampling(Oversampling::X16));
    static_cast<void>(port.set_baudrate(115'200u));
}

// ── phase 2: FIFO ────────────────────────────────────────────────────────────

template <typename Uart>
void probe_fifo(Uart& port) {
    console::write_line(port, "--- Phase 2: FIFO ---");
    report(port, "enable_fifo(true)",                 port.enable_fifo(true));
    report(port, "set_tx_threshold(Half)",            port.set_tx_threshold(FifoTrigger::Half));
    report(port, "set_rx_threshold(Quarter)",         port.set_rx_threshold(FifoTrigger::Quarter));
    report_bool(port, "tx_fifo_full",                 port.tx_fifo_full());
    report_bool(port, "rx_fifo_empty",                port.rx_fifo_empty());
    // Disable FIFO before continuing (restore normal single-byte mode).
    static_cast<void>(port.enable_fifo(false));
}

// ── phase 2: status flags ────────────────────────────────────────────────────

template <typename Uart>
void probe_status_flags(Uart& port) {
    console::write_line(port, "--- Phase 2: status flags ---");
    report_bool(port, "tx_complete",         port.tx_complete());
    report_bool(port, "tx_register_empty",   port.tx_register_empty());
    report_bool(port, "rx_register_not_empty", port.rx_register_not_empty());
    report_bool(port, "parity_error",        port.parity_error());
    report_bool(port, "framing_error",       port.framing_error());
    report_bool(port, "noise_error",         port.noise_error());
    report_bool(port, "overrun_error",       port.overrun_error());
    report(port, "clear_parity_error",       port.clear_parity_error());
    report(port, "clear_framing_error",      port.clear_framing_error());
    report(port, "clear_noise_error",        port.clear_noise_error());
    report(port, "clear_overrun_error",      port.clear_overrun_error());
}

// ── phase 2: interrupts ──────────────────────────────────────────────────────

template <typename Uart>
void probe_interrupts(Uart& port) {
    console::write_line(port, "--- Phase 2: interrupts ---");
    report(port, "enable_interrupt(Tc)",       port.enable_interrupt(InterruptKind::Tc));
    report(port, "enable_interrupt(Rxne)",     port.enable_interrupt(InterruptKind::Rxne));
    report(port, "enable_interrupt(IdleLine)", port.enable_interrupt(InterruptKind::IdleLine));
    report(port, "disable_interrupt(IdleLine)",port.disable_interrupt(InterruptKind::IdleLine));
    report(port, "disable_interrupt(Rxne)",    port.disable_interrupt(InterruptKind::Rxne));
    report(port, "disable_interrupt(Tc)",      port.disable_interrupt(InterruptKind::Tc));
}

// ── phase 3 ──────────────────────────────────────────────────────────────────

template <typename Uart>
void probe_modes(Uart& port) {
    console::write_line(port, "--- Phase 3: mode setters ---");

    // LIN
    report(port, "enable_lin(true)",      port.enable_lin(true));
    report(port, "send_lin_break",        port.send_lin_break());
    report_bool(port, "lin_break_detected", port.lin_break_detected());
    report(port, "clear_lin_break_flag",  port.clear_lin_break_flag());
    report(port, "enable_lin(false)",     port.enable_lin(false));

    // RS-485 DE
    report(port, "set_de_assertion_time(1)",    port.set_de_assertion_time(1u));
    report(port, "set_de_deassertion_time(1)",  port.set_de_deassertion_time(1u));
    report(port, "enable_de(true)",             port.enable_de(true));
    report(port, "enable_de(false)",            port.enable_de(false));

    // Half-duplex
    report(port, "set_half_duplex(true)",  port.set_half_duplex(true));
    report(port, "set_half_duplex(false)", port.set_half_duplex(false));

    // Smartcard / IrDA (disabled — exercising the lever only)
    report(port, "set_smartcard_mode(false)", port.set_smartcard_mode(false));
    report(port, "set_irda_mode(false)",      port.set_irda_mode(false));
}

// ── IdleLine interrupt arm / disarm ──────────────────────────────────────────
// Demonstrates the interrupt-enable/disable API without pulling in the DMA
// subsystem headers (async::uart::wait_for is compile-tested separately in
// tests/compile_tests/test_async_peripherals.cpp).

template <typename Uart>
void probe_idle_interrupt(Uart& port) {
    console::write_line(port, "--- Interrupt arm/disarm: IdleLine ---");
    report(port, "enable_interrupt(IdleLine)",
           port.enable_interrupt(InterruptKind::IdleLine));
    // Without a real ISR registered there is nothing to wait on; just disarm.
    report(port, "disable_interrupt(IdleLine)",
           port.disable_interrupt(InterruptKind::IdleLine));
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart({
        .baudrate              = alloy::hal::Baudrate::e115200,
        .data_bits             = alloy::hal::DataBits::Eight,
        .parity                = alloy::hal::Parity::None,
        .stop_bits             = alloy::hal::StopBits::One,
        .flow_control          = alloy::hal::FlowControl::None,
        .peripheral_clock_hz   = board::kDebugUartPeripheralClockHz,
    });

    if (uart.configure().is_err()) {
        // Cannot print — blink fast.
        while (true) {
            board::led::toggle();
            alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(100);
        }
    }

    console::write_line(uart, "=== uart_probe_complete ===");

    probe_baudrate(uart);
    probe_fifo(uart);
    probe_status_flags(uart);
    probe_interrupts(uart);
    probe_modes(uart);
    probe_idle_interrupt(uart);

    console::write_line(uart, "=== done ===");

    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
