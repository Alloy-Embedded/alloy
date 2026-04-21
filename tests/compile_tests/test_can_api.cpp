#include "hal/can.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE && \
    (defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED))
using PeripheralId = alloy::hal::can::PeripheralId;
using Can = alloy::hal::can::handle<PeripheralId::MCAN0>;
static_assert(Can::valid);
#endif

int main() {
#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE && \
    (defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED))
    auto can = alloy::hal::can::open<Can::peripheral_id>(
        alloy::hal::can::Config{
            .enter_init_mode = true,
            .enable_configuration = true,
            .enable_fd_operation = true,
            .enable_bit_rate_switch = true,
            .apply_nominal_timing = true});
    [[maybe_unused]] const auto configure_result = can.configure();
    [[maybe_unused]] const auto init_result = can.enter_init_mode();
    [[maybe_unused]] const auto cfg_result = can.enable_configuration();
    [[maybe_unused]] const auto fd_result = can.enable_fd_operation();
    [[maybe_unused]] const auto brs_result = can.enable_bit_rate_switch();
    [[maybe_unused]] const auto timing_result =
        can.set_nominal_timing(alloy::hal::can::nominal_timing_config{});
    [[maybe_unused]] const auto irq_result = can.enable_rx_fifo0_interrupt();
    [[maybe_unused]] const auto fill_level = can.rx_fifo0_fill_level();
    [[maybe_unused]] const auto tx_result = can.request_tx(0u);
    [[maybe_unused]] const auto leave_result = can.leave_init_mode();
#endif
    return 0;
}
