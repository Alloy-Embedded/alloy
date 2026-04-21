#if defined(ALLOY_BOARD_SAME70_XPLAINED) || defined(ALLOY_BOARD_SAME70_XPLD)
    #include "same70_xplained/board.hpp"
#else
    #error "can_probe currently supports SAME70 boards only."
#endif

#include "device/runtime.hpp"
#include "hal/can.hpp"
#include "hal/systick.hpp"

using namespace alloy::hal;

namespace {

using PeripheralId = alloy::device::runtime::PeripheralId;
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
        blink_error(100);
    }
    [[maybe_unused]] const auto irq_result = can.enable_rx_fifo0_interrupt();
    [[maybe_unused]] const auto fill_level = can.rx_fifo0_fill_level();
    [[maybe_unused]] const auto tx_result = can.request_tx(0u);
    [[maybe_unused]] const auto leave_result = can.leave_init_mode();

    while (true) {
        board::led::toggle();
        SysTickTimer::delay_ms<board::BoardSysTick>(500);
    }
}
