#include <span>
#include <type_traits>

#include "hal/dac.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE
using PeripheralId = alloy::hal::dac::PeripheralId;

#if defined(ALLOY_BOARD_NUCLEO_G071RB) || defined(ALLOY_BOARD_NUCLEO_G0B1RE)
using Dac = alloy::hal::dac::handle<PeripheralId::DAC, 0u>;
#define ALLOY_TEST_HAS_RUNTIME_DAC 1
#elif defined(ALLOY_BOARD_SAME70_XPLD) || defined(ALLOY_BOARD_SAME70_XPLAINED)
using Dac = alloy::hal::dac::handle<PeripheralId::DACC, 0u>;
#define ALLOY_TEST_HAS_RUNTIME_DAC 1
#endif

#if defined(ALLOY_TEST_HAS_RUNTIME_DAC)
static_assert(Dac::valid);
#endif
#endif

int main() {
#if ALLOY_DEVICE_DAC_SEMANTICS_AVAILABLE && defined(ALLOY_TEST_HAS_RUNTIME_DAC)
    auto dac = alloy::hal::dac::open<Dac::peripheral_id, Dac::channel_index>(
        alloy::hal::dac::Config{
            .enable_on_configure = true, .write_initial_value = true, .initial_value = 0u});

    // ── pre-existing ─────────────────────────────────────────────────────────
    [[maybe_unused]] const auto configure_result = dac.configure();
    [[maybe_unused]] const auto enable_result    = dac.enable();
    [[maybe_unused]] const bool ready            = dac.ready();
    [[maybe_unused]] const auto write_result     = dac.write(0x123u);
    [[maybe_unused]] const auto disable_result   = dac.disable();

    // ── task 1.1: indexed channel ops ─────────────────────────────────────────
    [[maybe_unused]] const auto en_ch0   = dac.enable_channel(0u);
    [[maybe_unused]] const auto en_ch1   = dac.enable_channel(1u);
    [[maybe_unused]] const auto dis_ch0  = dac.disable_channel(0u);
    [[maybe_unused]] const bool rdy_ch0  = dac.channel_ready(0u);
    [[maybe_unused]] const auto wr_ch0   = dac.write_channel(0u, 0x800u);
    [[maybe_unused]] const auto wr_ch1   = dac.write_channel(1u, 0x400u);

    // ── task 1.2: hardware trigger ────────────────────────────────────────────
    using TE = alloy::hal::dac::TriggerEdge;
    [[maybe_unused]] const auto trig_off = dac.set_hardware_trigger(0u, TE::Disabled);
    [[maybe_unused]] const auto trig_r   = dac.set_hardware_trigger(2u, TE::Rising);
    [[maybe_unused]] const auto trig_f   = dac.set_hardware_trigger(3u, TE::Falling);
    [[maybe_unused]] const auto trig_b   = dac.set_hardware_trigger(1u, TE::Both);

    // ── task 1.3: prescaler ───────────────────────────────────────────────────
    [[maybe_unused]] const auto psc = dac.set_prescaler(4u);

    // ── task 1.4: software reset ──────────────────────────────────────────────
    [[maybe_unused]] const auto rst = dac.software_reset();

    // ── task 2.1: typed interrupts ────────────────────────────────────────────
    using IK = alloy::hal::dac::InterruptKind;
    [[maybe_unused]] const auto en_tc    = dac.enable_interrupt(IK::TransferComplete);
    [[maybe_unused]] const auto en_ur    = dac.enable_interrupt(IK::Underrun);
    [[maybe_unused]] const auto en_dma   = dac.enable_interrupt(IK::DmaComplete);
    [[maybe_unused]] const auto dis_tc   = dac.disable_interrupt(IK::TransferComplete);

    // ── task 2.2: underrun status ─────────────────────────────────────────────
    [[maybe_unused]] const bool ur_flag  = dac.underrun();
    [[maybe_unused]] const bool ur_ch0   = dac.underrun_channel(0u);
    [[maybe_unused]] const auto clr_ur   = dac.clear_underrun();

    // ── task 2.3: kernel clock source ────────────────────────────────────────
    [[maybe_unused]] const auto kcs = dac.set_kernel_clock_source(
        alloy::hal::dac::KernelClockSource::Default);

    // ── task 2.4: irq_numbers ────────────────────────────────────────────────
    [[maybe_unused]] const auto irqs = Dac::irq_numbers();
    static_assert(std::is_same_v<decltype(irqs), const std::span<const std::uint32_t>>);
#endif
    return 0;
}
