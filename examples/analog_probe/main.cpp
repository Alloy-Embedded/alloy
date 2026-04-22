#include BOARD_HEADER

#ifndef BOARD_ANALOG_HEADER
    #error "analog_probe requires BOARD_ANALOG_HEADER for the selected board"
#endif

#include BOARD_ANALOG_HEADER

#ifdef BOARD_UART_HEADER
    #include BOARD_UART_HEADER
#endif

#include "examples/common/uart_console.hpp"
#include "hal/systick.hpp"

namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

}  // namespace

int main() {
    board::init();

#ifdef BOARD_UART_HEADER
    auto uart = board::make_debug_uart();
    const auto uart_ready = uart.configure().is_ok();
#else
    constexpr auto uart_ready = false;
#endif

    auto adc = board::make_adc({.enable_on_configure = true, .start_immediately = false});
    if (const auto result = adc.configure(); result.is_err()) {
        blink_error(150);
    }

    [[maybe_unused]] const auto adc_start = adc.start();
    [[maybe_unused]] const auto adc_ready = adc.ready();
    [[maybe_unused]] const auto adc_value = adc.read();

#ifdef BOARD_UART_HEADER
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "analog probe ready");
    }
#endif

    #if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_SAME70_XPLAINED) || \
        defined(ALLOY_BOARD_SAME70_XPLD)
        auto dac = board::make_dac({
            .enable_on_configure = true,
            .write_initial_value = true,
            .initial_value = 0x80u,
        });
        if (const auto result = dac.configure(); result.is_err()) {
            blink_error(200);
        }
        [[maybe_unused]] const auto dac_write = dac.write(0x123u);
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "dac active");
        }
#endif
    #endif

    std::uint32_t loop_count = 0u;
    while (true) {
        board::led::toggle();
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "analog loop=");
            alloy::examples::uart_console::write_unsigned(uart, loop_count);
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
#endif
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
        ++loop_count;
    }
}
