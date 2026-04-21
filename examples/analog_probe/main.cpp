#include BOARD_HEADER

#ifndef BOARD_ANALOG_HEADER
    #error "analog_probe requires BOARD_ANALOG_HEADER for the selected board"
#endif

#ifndef BOARD_UART_HEADER
    #error "analog_probe requires BOARD_UART_HEADER for the selected board"
#endif

#include BOARD_ANALOG_HEADER
#include BOARD_UART_HEADER

#include "hal/systick.hpp"

#include <cstddef>

namespace {

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

template <typename Uart>
void write_uart_text(Uart& uart, const char* text) {
    while (*text != '\0') {
        static_cast<void>(uart.write_byte(static_cast<std::byte>(*text++)));
    }
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    if (const auto result = uart.configure(); result.is_err()) {
        blink_error(100);
    }

    auto adc = board::make_adc({.enable_on_configure = true, .start_immediately = false});
    if (const auto result = adc.configure(); result.is_err()) {
        blink_error(150);
    }

    [[maybe_unused]] const auto adc_start = adc.start();
    [[maybe_unused]] const auto adc_ready = adc.ready();
    [[maybe_unused]] const auto adc_value = adc.read();

    write_uart_text(uart, "analog probe ready\r\n");

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
        write_uart_text(uart, "dac active\r\n");
    #endif

    while (true) {
        board::led::toggle();
        alloy::hal::SysTickTimer::delay_ms<board::BoardSysTick>(1000);
    }
}
