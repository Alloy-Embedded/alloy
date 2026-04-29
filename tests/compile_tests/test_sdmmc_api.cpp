// Compile test: SDMMC HAL API surface is instantiable on boards that publish
// SDMMC semantics. Targeted at same70_xplained (PeripheralId::HSMCI).
//
// Ref: openspec/changes/extend-sdmmc-coverage

#include "device/runtime.hpp"
#include "hal/sdmmc.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_SDMMC_SEMANTICS_AVAILABLE && defined(ALLOY_BOARD_SAME70_XPLD)

namespace {

void exercise_sdmmc_handle() {
    using namespace alloy;

    auto sd = hal::sdmmc::open<device::PeripheralId::HSMCI>();
    static_assert(decltype(sd)::valid);

    // ── Phase 1: bus config + clock + command ─────────────────────────────────

    [[maybe_unused]] const auto en  = sd.enable();
    [[maybe_unused]] const auto dis = sd.disable();
    [[maybe_unused]] const auto rst = sd.software_reset();

    [[maybe_unused]] const auto bw1 = sd.set_bus_width(hal::sdmmc::BusWidth::Bits1);
    [[maybe_unused]] const auto bw4 = sd.set_bus_width(hal::sdmmc::BusWidth::Bits4);
    [[maybe_unused]] const auto bw8 = sd.set_bus_width(hal::sdmmc::BusWidth::Bits8);

    [[maybe_unused]] const auto clkdiv = sd.set_clock_divider(64u);
    [[maybe_unused]] const auto clksrc =
        sd.set_kernel_clock_source(hal::sdmmc::KernelClockSource::Default);

    // CMD0 (GO_IDLE_STATE) — no response
    const hal::sdmmc::CommandConfig cmd0{
        .index           = 0u,
        .argument        = 0u,
        .response_type   = hal::sdmmc::ResponseType::None,
        .wait_for_response = false,
    };
    [[maybe_unused]] const auto r0 = sd.send_command(cmd0);

    // CMD8 — short response
    const hal::sdmmc::CommandConfig cmd8{
        .index           = 8u,
        .argument        = 0x000001AAu,
        .response_type   = hal::sdmmc::ResponseType::Short,
        .wait_for_response = true,
    };
    [[maybe_unused]] const auto r8 = sd.send_command(cmd8);

    // ── Phase 2: block transfer + DMA + timeouts ──────────────────────────────

    [[maybe_unused]] const auto bsz = sd.set_block_size(512u);
    [[maybe_unused]] const auto bcnt = sd.set_block_count(1u);

    std::byte buf[512]{};
    [[maybe_unused]] const auto rd =
        sd.read_blocks(std::span{buf});
    [[maybe_unused]] const auto wr =
        sd.write_blocks(std::span<const std::byte>{buf});

    [[maybe_unused]] const auto dma_en  = sd.enable_dma(true);
    [[maybe_unused]] const auto dma_dis = sd.enable_dma(false);

    [[maybe_unused]] const auto dtout = sd.set_data_timeout(0xFFu);
    [[maybe_unused]] const auto ctout = sd.set_completion_timeout(0xFFu);

    // ── Phase 3: interrupts + IRQ vector ─────────────────────────────────────

    [[maybe_unused]] const auto ie_cc =
        sd.enable_interrupt(hal::sdmmc::InterruptKind::CommandComplete);
    [[maybe_unused]] const auto ie_dc =
        sd.enable_interrupt(hal::sdmmc::InterruptKind::DataComplete);
    [[maybe_unused]] const auto ie_cto =
        sd.enable_interrupt(hal::sdmmc::InterruptKind::CommandTimeout);
    [[maybe_unused]] const auto id_cc =
        sd.disable_interrupt(hal::sdmmc::InterruptKind::CommandComplete);
    [[maybe_unused]] const auto id_dc =
        sd.disable_interrupt(hal::sdmmc::InterruptKind::DataCrc);

    [[maybe_unused]] const auto irqs = decltype(sd)::irq_numbers();

    // DMA address helpers
    [[maybe_unused]] constexpr auto rx_addr = decltype(sd)::rx_data_register_address();
    [[maybe_unused]] constexpr auto tx_addr = decltype(sd)::tx_data_register_address();
}

}  // namespace

#endif
