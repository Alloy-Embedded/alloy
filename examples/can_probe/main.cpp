#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#else
    #error "can_probe currently supports SAME70 boards only."
#endif

#include BOARD_UART_HEADER

#include "examples/common/uart_console.hpp"
#include "device/runtime.hpp"
#include "hal/can.hpp"
#include "hal/systick.hpp"

using namespace alloy::hal;

namespace {

using PeripheralId = alloy::device::PeripheralId;
constexpr auto kCanPeripheral = PeripheralId::MCAN0;

[[noreturn]] void blink_error(std::uint32_t period_ms) {
    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(period_ms);
    }
}

}  // namespace

int main() {
    board::init();

    auto uart = board::make_debug_uart();
    const auto uart_ready = uart.configure().is_ok();
    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "can probe ready");
    }

    auto can = alloy::hal::can::open<kCanPeripheral>({
        .enter_init_mode = true,
        .enable_configuration = true,
        .enable_fd_operation = true,
        .enable_bit_rate_switch = true,
        .apply_nominal_timing = true,
        .nominal_timing =
            {
                .prescaler = 1u,
                .time_seg1 = 10u,
                .time_seg2 = 3u,
                .sync_jump_width = 2u,
            },
    });

    if (!can.configure().is_ok()) {
        if (uart_ready) {
            alloy::examples::uart_console::write_line(uart, "can configure failed");
        }
        blink_error(100);
    }
    [[maybe_unused]] const auto irq_result = can.enable_rx_fifo0_interrupt();
    [[maybe_unused]] const auto fill_level = can.rx_fifo0_fill_level();
    [[maybe_unused]] const auto tx_result = can.request_tx(0u);
    [[maybe_unused]] const auto leave_result = can.leave_init_mode();

    if (uart_ready) {
        alloy::examples::uart_console::write_line(uart, "can configured");
    }

    std::uint32_t loop_count = 0u;
    while (true) {
        board::led::toggle();
#ifdef BOARD_UART_HEADER
        if (uart_ready) {
            alloy::examples::uart_console::write_text(uart, "can loop=");
            alloy::examples::uart_console::write_unsigned(uart, loop_count);
            alloy::examples::uart_console::write_text(uart, "\r\n");
        }
#endif
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
        ++loop_count;
    }
}
