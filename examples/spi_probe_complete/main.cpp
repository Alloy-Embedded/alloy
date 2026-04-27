// spi_probe_complete — exercises every new extend-spi-coverage HAL lever.
//
// Targets: nucleo_g071rb (primary), nucleo_f401re, same70_xplained.
//
// What this example demonstrates on the board's SPI1 / SPI0 bus:
//   Phase 1: set_data_size(16), set_clock_speed(16M), realised_clock_speed(),
//            kernel_clock_hz()
//   Phase 2: set_frame_format(Motorola/TI), enable_crc + set_crc_polynomial,
//            crc_error / clear_crc_error, set_bidirectional / direction,
//            set_nss_management(HardwareOutput) + set_nss_pulse_per_transfer
//   Phase 3: SAM-style per-CS timing setters (NotSupported on STM32)
//   Phase 4: status flag reads, interrupt arm/disarm, irq_numbers()
//
// All "NotSupported" returns are expected for features absent on the
// selected peripheral (e.g. CRC absent on SAME70 SPI; per-CS timing absent
// on STM32 SPI). The example prints a one-line verdict per feature.

#include BOARD_HEADER
#include BOARD_UART_HEADER
#include BOARD_SPI_HEADER

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/error_code.hpp"
#include "examples/common/uart_console.hpp"
#include "hal/spi.hpp"
#include "hal/systick.hpp"
#include "hal/uart.hpp"

namespace {

namespace console = alloy::examples::uart_console;
using alloy::hal::spi::BiDir;
using alloy::hal::spi::FrameFormat;
using alloy::hal::spi::InterruptKind;
using alloy::hal::spi::NssManagement;

// ── helpers ──────────────────────────────────────────────────────────────────

template <typename Uart>
void report(Uart& uart, const char* label,
            const alloy::core::Result<void, alloy::core::ErrorCode>& result) {
    console::write_text(uart, label);
    if (result.is_ok()) {
        console::write_line(uart, " OK");
    } else {
        console::write_text(uart, " NotSupported(");
        console::write_unsigned(uart, static_cast<std::uint32_t>(result.err()));
        console::write_line(uart, ")");
    }
}

template <typename Uart>
void report_bool(Uart& uart, const char* label, bool value) {
    console::write_text(uart, label);
    console::write_line(uart, value ? " true" : " false");
}

template <typename Uart>
void report_u32(Uart& uart, const char* label, std::uint32_t value) {
    console::write_text(uart, label);
    console::write_text(uart, " ");
    console::write_unsigned(uart, value);
    console::write_line(uart, "");
}

// ── phase 1 ──────────────────────────────────────────────────────────────────

template <typename Uart, typename Spi>
void probe_data_size_and_clock(Uart& uart, Spi& spi) {
    console::write_line(uart, "--- Phase 1: data size / clock ---");

    report_u32(uart, "kernel_clock_hz =", spi.kernel_clock_hz());

    // 4-bit and 17-bit are static_assert at compile time via the templated
    // overload — exercised via the runtime variant only (returns InvalidParameter).
    report(uart, "set_data_size(8)",  spi.set_data_size(8u));
    report(uart, "set_data_size(16)", spi.set_data_size(16u));
    report(uart, "set_data_size(12)", spi.set_data_size(12u));  // STM32G0 supports
    report(uart, "set_data_size(3)",  spi.set_data_size(3u));   // always rejected

    report(uart, "set_clock_speed(16M)", spi.set_clock_speed(16'000'000u));
    report_u32(uart, "realised_clock_speed =", spi.realised_clock_speed());
    // 5 MHz on STM32 (powers-of-2 prescaler) cannot land within ±5 % of 5 MHz —
    // realisable rates are 32 / 16 / 8 / 4 / 2 / 1 MHz.
    report(uart, "set_clock_speed(5M)", spi.set_clock_speed(5'000'000u));
}

// ── phase 2 ──────────────────────────────────────────────────────────────────

template <typename Uart, typename Spi>
void probe_frame_format_and_crc(Uart& uart, Spi& spi) {
    console::write_line(uart, "--- Phase 2: frame / CRC / bidi / NSS ---");

    report(uart, "set_frame_format(Motorola)", spi.set_frame_format(FrameFormat::Motorola));
    report(uart, "set_frame_format(TI)",       spi.set_frame_format(FrameFormat::TI));

    // CCITT polynomial (0x1021) is the standard SD card / Bluetooth choice.
    report(uart, "enable_crc(true)",        spi.enable_crc(true));
    report(uart, "set_crc_polynomial(0x1021)", spi.set_crc_polynomial(0x1021u));
    report_u32(uart, "read_crc =",          spi.read_crc());
    report_bool(uart, "crc_error",          spi.crc_error());
    report(uart, "clear_crc_error",         spi.clear_crc_error());
    report(uart, "enable_crc(false)",       spi.enable_crc(false));

    report(uart, "set_bidirectional(true)",            spi.set_bidirectional(true));
    report(uart, "set_bidirectional_direction(Receive)",
           spi.set_bidirectional_direction(BiDir::Receive));
    report(uart, "set_bidirectional(false)",           spi.set_bidirectional(false));

    report(uart, "set_nss_management(HardwareOutput)",
           spi.set_nss_management(NssManagement::HardwareOutput));
    report(uart, "set_nss_pulse_per_transfer(true)",   spi.set_nss_pulse_per_transfer(true));
    // Restore software-managed NSS so the bus still works for normal transfers.
    report(uart, "set_nss_management(Software)",
           spi.set_nss_management(NssManagement::Software));
}

// ── phase 3: SAM-style per-CS timing (NotSupported on STM32) ────────────────

template <typename Uart, typename Spi>
void probe_per_cs_timing(Uart& uart, Spi& spi) {
    console::write_line(uart, "--- Phase 3: per-CS timing (SAM-only) ---");
    report(uart, "set_cs_decode_mode(false)",          spi.set_cs_decode_mode(false));
    report(uart, "set_cs_delay_between_consecutive(100)",
           spi.set_cs_delay_between_consecutive(std::uint16_t{100}));
    report(uart, "set_cs_delay_clock_to_active(50)",
           spi.set_cs_delay_clock_to_active(std::uint16_t{50}));
    report(uart, "set_cs_delay_active_to_clock(50)",
           spi.set_cs_delay_active_to_clock(std::uint16_t{50}));
}

// ── phase 4: status flags + interrupts + irq_numbers ────────────────────────

template <typename Uart, typename Spi>
void probe_status_and_interrupts(Uart& uart, Spi& spi) {
    console::write_line(uart, "--- Phase 4: status / interrupts ---");

    report_bool(uart, "tx_register_empty",     spi.tx_register_empty());
    report_bool(uart, "rx_register_not_empty", spi.rx_register_not_empty());
    report_bool(uart, "busy",                  spi.busy());
    report_bool(uart, "mode_fault",            spi.mode_fault());
    report(uart, "clear_mode_fault",           spi.clear_mode_fault());
    report_bool(uart, "frame_format_error",    spi.frame_format_error());

    report(uart, "enable_interrupt(Txe)",       spi.enable_interrupt(InterruptKind::Txe));
    report(uart, "enable_interrupt(Rxne)",      spi.enable_interrupt(InterruptKind::Rxne));
    report(uart, "enable_interrupt(Error)",     spi.enable_interrupt(InterruptKind::Error));
    report(uart, "enable_interrupt(ModeFault)", spi.enable_interrupt(InterruptKind::ModeFault));
    report(uart, "enable_interrupt(CrcError)",  spi.enable_interrupt(InterruptKind::CrcError));
    report(uart, "enable_interrupt(FrameError)",spi.enable_interrupt(InterruptKind::FrameError));

    static_cast<void>(spi.disable_interrupt(InterruptKind::Txe));
    static_cast<void>(spi.disable_interrupt(InterruptKind::Rxne));
    static_cast<void>(spi.disable_interrupt(InterruptKind::Error));

    const auto irqs = Spi::irq_numbers();
    report_u32(uart, "irq_numbers.size =", static_cast<std::uint32_t>(irqs.size()));
    for (std::size_t i = 0u; i < irqs.size(); ++i) {
        console::write_text(uart, "  irq[");
        console::write_unsigned(uart, static_cast<std::uint32_t>(i));
        console::write_text(uart, "] = ");
        console::write_unsigned(uart, irqs[i]);
        console::write_line(uart, "");
    }
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
        while (true) {
            board::led::toggle();
            alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(100);
        }
    }

    console::write_line(uart, "=== spi_probe_complete ===");

    auto spi = board::make_spi(alloy::hal::spi::Config{
        alloy::hal::SpiMode::Mode3,
        1'000'000u,
        alloy::hal::SpiBitOrder::MsbFirst,
        alloy::hal::SpiDataSize::Bits8,
        board::kBoardSpiPeripheralClockHz,
    });

    if (spi.configure().is_err()) {
        console::write_line(uart, "spi.configure() failed");
        while (true) {
            board::led::toggle();
            alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(200);
        }
    }

    probe_data_size_and_clock(uart, spi);
    probe_frame_format_and_crc(uart, spi);
    probe_per_cs_timing(uart, spi);
    probe_status_and_interrupts(uart, spi);

    console::write_line(uart, "=== done ===");

    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
