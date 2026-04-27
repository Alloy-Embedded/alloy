// watchdog_probe_complete — exercises every new extend-watchdog-coverage lever.
//
// Targets: nucleo_g071rb (IWDG only — most levers will return NotSupported),
//          nucleo_f401re (IWDG + WWDG with EWI), same70_xplained (WDT with
//          window + EWI + reset-enable).
//
// Strategy: open the board's watchdog handle, call every new method,
// print OK / NotSupported per feature. The watchdog is NOT armed — the
// example must run safely on every board without causing a reset loop.

#include BOARD_HEADER

#ifndef BOARD_WATCHDOG_HEADER
    #error "watchdog_probe_complete requires BOARD_WATCHDOG_HEADER for the selected board"
#endif

#include BOARD_WATCHDOG_HEADER

#ifdef BOARD_UART_HEADER
    #include BOARD_UART_HEADER
#endif

#include <cstdint>

#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

static_assert(board::BoardWatchdog::valid,
              "BoardWatchdog peripheral is not present in the published "
              "device descriptor — check board_watchdog.hpp");

namespace {

namespace console = alloy::examples::uart_console;
using K = alloy::hal::watchdog::InterruptKind;

template <typename Uart>
void report(Uart& uart, bool uart_ready, const char* label,
            const alloy::core::Result<void, alloy::core::ErrorCode>& result) {
    if (!uart_ready) {
        return;
    }
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
void report_bool(Uart& uart, bool uart_ready, const char* label, bool value) {
    if (!uart_ready) {
        return;
    }
    console::write_text(uart, label);
    console::write_line(uart, value ? " true" : " false");
}

template <typename Uart>
void report_u32(Uart& uart, bool uart_ready, const char* label, std::uint32_t value) {
    if (!uart_ready) {
        return;
    }
    console::write_text(uart, label);
    console::write_text(uart, " ");
    console::write_unsigned(uart, value);
    console::write_line(uart, "");
}

[[noreturn]] void blink_forever(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

template <typename Uart, typename Wdg>
void exercise(Uart& uart, bool uart_ready, Wdg& wdg) {
    if (uart_ready) {
        console::write_line(uart, "--- extend-watchdog-coverage ---");
    }

    // Phase 1: window mode — gated on kHasWindow.
    report(uart, uart_ready, "set_window(0x40)",
           wdg.set_window(std::uint16_t{0x40}));
    report(uart, uart_ready, "enable_window_mode(true)",
           wdg.enable_window_mode(true));
    report(uart, uart_ready, "enable_window_mode(false)",
           wdg.enable_window_mode(false));

    // Phase 1: early warning — gated on kEarlyWarningInterruptEnableField.
    report(uart, uart_ready, "enable_early_warning(50)",
           wdg.enable_early_warning(std::uint16_t{50}));
    report_bool(uart, uart_ready, "early_warning_pending",
                wdg.early_warning_pending());
    report(uart, uart_ready, "clear_early_warning",
           wdg.clear_early_warning());

    // Phase 1: status flags — gated per-field.
    report_bool(uart, uart_ready, "timeout_occurred",
                wdg.timeout_occurred());
    report_bool(uart, uart_ready, "prescaler_update_in_progress",
                wdg.prescaler_update_in_progress());
    report_bool(uart, uart_ready, "reload_update_in_progress",
                wdg.reload_update_in_progress());
    report_bool(uart, uart_ready, "window_update_in_progress",
                wdg.window_update_in_progress());
    report_bool(uart, uart_ready, "error",
                wdg.error());

    // Phase 1: reset-enable — gated on kResetEnableField.
    report(uart, uart_ready, "set_reset_on_timeout(false)",
           wdg.set_reset_on_timeout(false));
    report(uart, uart_ready, "set_reset_on_timeout(true)",
           wdg.set_reset_on_timeout(true));

    // Phase 2: typed interrupts.
    report(uart, uart_ready, "enable_interrupt(EarlyWarning)",
           wdg.enable_interrupt(K::EarlyWarning));
    report(uart, uart_ready, "disable_interrupt(EarlyWarning)",
           wdg.disable_interrupt(K::EarlyWarning));

    // Phase 2: irq numbers.
    const auto irqs = Wdg::irq_numbers();
    report_u32(uart, uart_ready, "irq_numbers.size =",
               static_cast<std::uint32_t>(irqs.size()));
    for (std::size_t i = 0u; i < irqs.size(); ++i) {
        if (uart_ready) {
            console::write_text(uart, "  irq[");
            console::write_unsigned(uart, static_cast<std::uint32_t>(i));
            console::write_text(uart, "] = ");
            console::write_unsigned(uart, irqs[i]);
            console::write_line(uart, "");
        }
    }

    // Refresh — must succeed on every backend.
    report(uart, uart_ready, "refresh()", wdg.refresh());
}

}  // namespace

int main() {
    board::init();

#ifdef BOARD_UART_HEADER
    auto uart = board::make_debug_uart();
    const auto uart_ready = uart.configure().is_ok();
    if (uart_ready) {
        console::write_line(uart, "watchdog_probe_complete ready");
    }
#else
    constexpr auto uart_ready = false;
#endif

    constexpr alloy::hal::watchdog::Config kOpenConfig{
        .disable_on_configure = board::kBoardWatchdogCanDisable,
        .refresh_on_configure = false,
    };
    auto wdg = board::make_watchdog(kOpenConfig);

#ifdef BOARD_UART_HEADER
    exercise(uart, uart_ready, wdg);
    if (uart_ready) {
        console::write_line(uart, "=== done ===");
    }
#else
    // No UART on this board — just prove the methods compile + run.
    static_cast<void>(wdg.refresh());
#endif

    blink_forever(500);
}
