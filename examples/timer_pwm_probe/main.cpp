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
#include "hal/pwm.hpp"
#include "hal/systick.hpp"
#include "hal/timer.hpp"

using namespace alloy::hal;

namespace {

using PeripheralId = alloy::device::runtime::PeripheralId;

#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
constexpr auto kTimerPeripheral = PeripheralId::TC0;
constexpr auto kPwmPeripheral = PeripheralId::PWM0;
#else
constexpr auto kTimerPeripheral = PeripheralId::TIM1;
constexpr auto kPwmPeripheral = PeripheralId::TIM1;
#endif

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
        alloy::examples::uart_console::write_line(uart, "timer/pwm probe ready");
    }
#else
    constexpr auto uart_ready = false;
#endif

    auto timer = alloy::hal::timer::open<kTimerPeripheral>();
    auto pwm = alloy::hal::pwm::open<kPwmPeripheral, 0u>();

    if (!timer.start().is_ok()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "timer start failed");
        }
#endif
        blink_error(100);
    }
    if (!timer.set_period(1000u).is_ok()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "timer period failed");
        }
#endif
        blink_error(150);
    }
    if (!pwm.set_period(1000u).is_ok()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "pwm period failed");
        }
#endif
        blink_error(200);
    }
    if (!pwm.set_duty_cycle(500u).is_ok()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "pwm duty failed");
        }
#endif
        blink_error(250);
    }
    if (!pwm.start().is_ok()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "pwm start failed");
        }
#endif
        blink_error(300);
    }
    if (!timer.stop().is_ok()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "timer stop failed");
        }
#endif
        blink_error(350);
    }
    if (!pwm.stop().is_ok()) {
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "pwm stop failed");
        }
#endif
        blink_error(400);
    }

#ifdef BOARD_UART_HEADER
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "timer/pwm configured");
    }
#endif

    std::uint32_t loop_count = 0u;
    while (true) {
        board::led::toggle();
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "timer/pwm loop=");
            alloy::examples::uart_console::write_unsigned(uart, loop_count);
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
#endif
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
        ++loop_count;
    }
}
