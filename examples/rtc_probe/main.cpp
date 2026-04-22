#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G0B1RE)
    #include "nucleo_g0b1re/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_G071RB)
    #include "nucleo_g071rb/board.hpp"
#elif defined(ALLOY_BOARD_NUCLEO_F401RE)
    #include "nucleo_f401re/board.hpp"
#else
    #error "Unsupported board! Define ALLOY_BOARD_* in your build system."
#endif

#ifdef BOARD_UART_HEADER
    #include BOARD_UART_HEADER
#endif

#include "examples/common/uart_console.hpp"
#include "device/runtime.hpp"
#include "hal/rtc.hpp"
#include "hal/systick.hpp"

using namespace alloy::hal;

namespace {

using PeripheralId = alloy::device::PeripheralId;
constexpr auto kRtcPeripheral = PeripheralId::RTC;

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

}  // namespace

int main() {
    board::init();

#ifdef BOARD_UART_HEADER
    auto uart = board::make_debug_uart();
    const auto uart_ready = uart.configure().is_ok();
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "rtc probe ready");
    }
#else
    constexpr auto uart_ready = false;
#endif

    auto rtc = alloy::hal::rtc::open<kRtcPeripheral>({
        .enable_write_access = true,
        .enter_init_mode = true,
        .enable_alarm_interrupt = false,
    });

    if (!rtc.configure().is_ok()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "rtc configure failed");
        }
#endif
        blink_error(100);
    }
    [[maybe_unused]] const auto init_ready = rtc.init_ready();
    [[maybe_unused]] const auto time_result = rtc.read_time();
    [[maybe_unused]] const auto date_result = rtc.read_date();
    [[maybe_unused]] const auto leave_result = rtc.leave_init_mode();
    [[maybe_unused]] const auto disable_result = rtc.disable_write_access();

#ifdef BOARD_UART_HEADER
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "rtc configured");
    }
#endif

    std::uint32_t loop_count = 0u;
    while (true) {
        board::led::toggle();
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "rtc loop=");
            alloy::examples::uart_console::write_unsigned(uart, loop_count);
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
#endif
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
        ++loop_count;
    }
}
