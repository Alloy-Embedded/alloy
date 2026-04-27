#include <span>
#include <type_traits>

#include "hal/can.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE && \
    (defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED))
using PeripheralId = alloy::hal::can::PeripheralId;
using Can = alloy::hal::can::handle<PeripheralId::MCAN0>;
static_assert(Can::valid);
#define ALLOY_TEST_HAS_RUNTIME_CAN 1
#endif

int main() {
#if ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE && defined(ALLOY_TEST_HAS_RUNTIME_CAN)
    auto can = alloy::hal::can::open<Can::peripheral_id>(
        alloy::hal::can::Config{
            .enter_init_mode       = true,
            .enable_configuration  = true,
            .enable_fd_operation   = true,
            .enable_bit_rate_switch = true,
            .apply_nominal_timing  = true});

    // ── pre-existing ─────────────────────────────────────────────────────────
    [[maybe_unused]] const auto configure_result = can.configure();
    [[maybe_unused]] const auto init_result      = can.enter_init_mode();
    [[maybe_unused]] const auto cfg_result       = can.enable_configuration();
    [[maybe_unused]] const auto fd_result        = can.enable_fd_operation();
    [[maybe_unused]] const auto brs_result       = can.enable_bit_rate_switch();
    [[maybe_unused]] const auto timing_result    =
        can.set_nominal_timing(alloy::hal::can::nominal_timing_config{});
    [[maybe_unused]] const auto irq_result       = can.enable_rx_fifo0_interrupt();
    [[maybe_unused]] const auto fill_level       = can.rx_fifo0_fill_level();
    [[maybe_unused]] const auto tx_result        = can.request_tx(0u);
    [[maybe_unused]] const auto leave_result     = can.leave_init_mode();

    // ── task 1.1: FD data timing ──────────────────────────────────────────────
    [[maybe_unused]] const auto dt_result =
        can.set_data_timing(alloy::hal::can::data_timing_config{
            .prescaler = 1u, .sync_jump_width = 1u, .time_seg1 = 5u, .time_seg2 = 2u});

    // ── task 1.2: test mode ───────────────────────────────────────────────────
    using TM = alloy::hal::can::TestMode;
    [[maybe_unused]] const auto tm_norm  = can.set_test_mode(TM::Normal);
    [[maybe_unused]] const auto tm_bm    = can.set_test_mode(TM::BusMonitor);
    [[maybe_unused]] const auto tm_lbi   = can.set_test_mode(TM::LoopbackInternal);
    [[maybe_unused]] const auto tm_lbe   = can.set_test_mode(TM::LoopbackExternal);
    [[maybe_unused]] const auto tm_rst   = can.set_test_mode(TM::Restricted);

    // ── task 1.3: transmit / receive ──────────────────────────────────────────
    alloy::hal::can::CanFrame tx_frame{};
    tx_frame.id = 0x123u;
    tx_frame.dlc = 8u;
    [[maybe_unused]] const auto tx_frame_r = can.transmit(tx_frame);

    alloy::hal::can::CanFrame rx_frame{};
    [[maybe_unused]] const auto rx_frame_r =
        can.receive(rx_frame, alloy::hal::can::FifoId::Fifo0);

    // ── task 2.1: filter banks ────────────────────────────────────────────────
    using FT = alloy::hal::can::FilterTarget;
    [[maybe_unused]] const auto filt_std =
        can.set_filter_standard(0u, 0x100u, 0x7FFu, FT::Rxfifo0);
    [[maybe_unused]] const auto filt_ext =
        can.set_filter_extended(1u, 0x1800'0000u, 0x1FFF'FFFFu, FT::Rxfifo1);

    // ── task 2.2: error counters + protocol status ────────────────────────────
    [[maybe_unused]] const std::uint8_t tx_ec = can.tx_error_count();
    [[maybe_unused]] const std::uint8_t rx_ec = can.rx_error_count();
    [[maybe_unused]] const bool bo_flag  = can.bus_off();
    [[maybe_unused]] const bool ep_flag  = can.error_passive();
    [[maybe_unused]] const bool ew_flag  = can.error_warning();

    // ── task 2.3: bus-off recovery ────────────────────────────────────────────
    [[maybe_unused]] const auto recovery = can.request_bus_off_recovery();

    // ── task 3.1: typed interrupts ────────────────────────────────────────────
    using IK = alloy::hal::can::InterruptKind;
    [[maybe_unused]] const auto en_tx   = can.enable_interrupt(IK::Tx);
    [[maybe_unused]] const auto en_rx0  = can.enable_interrupt(IK::RxFifo0);
    [[maybe_unused]] const auto en_rx1  = can.enable_interrupt(IK::RxFifo1);
    [[maybe_unused]] const auto en_bo   = can.enable_interrupt(IK::BusOff);
    [[maybe_unused]] const auto dis_tx  = can.disable_interrupt(IK::Tx);
    [[maybe_unused]] const auto dis_rx0 = can.disable_interrupt(IK::RxFifo0);

    // ── task 3.2: irq_numbers ────────────────────────────────────────────────
    [[maybe_unused]] const auto irqs = Can::irq_numbers();
    static_assert(std::is_same_v<decltype(irqs), const std::span<const std::uint32_t>>);
#endif
    return 0;
}
