// Compile test: QSPI HAL API surface is instantiable on boards that publish
// QSPI semantics. Targeted at same70_xplained (PeripheralId::QSPI).
//
// Note: only SAME70 currently publishes kPresent = true for QSPI; the spec
// reference to nucleo_f401re is incorrect (F401 kPresent = false).
//
// Ref: openspec/changes/extend-qspi-coverage

#include "device/runtime.hpp"
#include "hal/qspi.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_QSPI_SEMANTICS_AVAILABLE && defined(ALLOY_BOARD_SAME70_XPLD)

namespace {

void exercise_qspi_handle() {
    using namespace alloy;

    auto q = hal::qspi::open<device::PeripheralId::QSPI>();
    static_assert(decltype(q)::valid);

    // ── Phase 1: frame config + transfer primitives ───────────────────────────

    [[maybe_unused]] const auto en  = q.enable();
    [[maybe_unused]] const auto dis = q.disable();
    [[maybe_unused]] const auto rst = q.software_reset();

    [[maybe_unused]] const auto iw =
        q.set_instruction_width(hal::qspi::FrameWidth::Single);
    [[maybe_unused]] const auto aw =
        q.set_address_width(hal::qspi::FrameWidth::Single);
    [[maybe_unused]] const auto dw =
        q.set_data_width(hal::qspi::FrameWidth::Quad);

    [[maybe_unused]] const auto instr = q.set_instruction(0xEBu);
    [[maybe_unused]] const auto addr  = q.set_address(0x001000u, 24u);
    [[maybe_unused]] const auto dum   = q.set_dummy_cycles(6u);
    [[maybe_unused]] const auto bpt   = q.set_bits_per_transfer(8u);

    [[maybe_unused]] const bool done = q.last_transfer_done();

    // read / write require a non-empty span; just verify they compile
    std::byte buf[4]{};
    [[maybe_unused]] const auto rd  = q.read(std::span{buf});
    [[maybe_unused]] const auto wr  = q.write(std::span<const std::byte>{buf});

    // ── Phase 2: mode + CS + continuous-read + scrambling ─────────────────────

    [[maybe_unused]] const auto mode =
        q.set_mode(hal::qspi::QspiMode::Indirect);
    [[maybe_unused]] const auto cs =
        q.set_cs_mode(hal::qspi::CsMode::PerInstruction);
    [[maybe_unused]] const auto crm = q.enable_continuous_read(false);
    [[maybe_unused]] const auto scr = q.enable_scrambling(false);
    [[maybe_unused]] const auto key = q.set_scrambling_key(0xDEADBEEFu);
    q.invalidate_cache_for_memory_map();

    // ── Phase 3: kernel clock + interrupts + IRQ ─────────────────────────────

    [[maybe_unused]] const auto clk =
        q.set_kernel_clock_source(hal::qspi::KernelClockSource::Default);
    [[maybe_unused]] const auto ie =
        q.enable_interrupt(hal::qspi::InterruptKind::TransferComplete);
    [[maybe_unused]] const auto id =
        q.disable_interrupt(hal::qspi::InterruptKind::FifoThreshold);

    [[maybe_unused]] const auto irqs = decltype(q)::irq_numbers();

    // DMA address helpers
    [[maybe_unused]] constexpr auto rx_addr = decltype(q)::rx_data_register_address();
    [[maybe_unused]] constexpr auto tx_addr = decltype(q)::tx_data_register_address();
}

}  // namespace

#endif
