// Compile test: USB HAL API surface is instantiable on boards that publish
// USB semantics. Targeted at same70_xplained (PeripheralId::USBHS).
//
// Ref: openspec/changes/extend-usb-coverage

#include "device/runtime.hpp"
#include "hal/usb.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_USB_SEMANTICS_AVAILABLE && defined(ALLOY_BOARD_SAME70_XPLD)

namespace {

void exercise_usb_handle() {
    using namespace alloy;

    auto usb = hal::usb::open<device::PeripheralId::USBHS>();
    static_assert(decltype(usb)::valid);

    // ── Phase 1: mode + address + clock + crystalless + pins ─────────────────

    [[maybe_unused]] const auto ena = usb.enable(true);

    // set_mode: NotSupported on SAME70 (kForceDeviceModeField invalid)
    [[maybe_unused]] const auto mode_dev = usb.set_mode(hal::usb::UsbMode::Device);
    [[maybe_unused]] const auto mode_host = usb.set_mode(hal::usb::UsbMode::Host);

    [[maybe_unused]] const auto addr    = usb.set_address(0x02u);
    [[maybe_unused]] const auto addr_en = usb.enable_address(true);

    // freeze / unfreeze: kHasClockFreeze = true on SAME70 USBHS
    [[maybe_unused]] const auto freeze   = usb.freeze_clock();
    [[maybe_unused]] const auto unfreeze = usb.unfreeze_clock();
    [[maybe_unused]] const bool clk_ok   = usb.clock_usable();

    // crystalless: kCrystalless = false on SAME70 → NotSupported
    [[maybe_unused]] const auto xtalless = usb.enable_crystalless(true);

    [[maybe_unused]] constexpr auto dm = decltype(usb)::dm_pin();
    [[maybe_unused]] constexpr auto dp = decltype(usb)::dp_pin();

    // ── Phase 2: endpoint config + DPRAM ─────────────────────────────────────

    const hal::usb::EndpointConfig ep0{
        .number          = 0u,
        .direction       = hal::usb::Direction::In,
        .type            = hal::usb::EndpointType::Control,
        .max_packet_size = 64u,
    };
    [[maybe_unused]] const auto cfg_ep = usb.configure_endpoint(ep0);

    const hal::usb::EndpointConfig ep1{
        .number          = 1u,
        .direction       = hal::usb::Direction::In,
        .type            = hal::usb::EndpointType::Bulk,
        .max_packet_size = 512u,
    };
    [[maybe_unused]] const auto cfg_ep1 = usb.configure_endpoint(ep1);

    // DPRAM: kDpramBaseAddress = 0 on SAME70 → NotSupported
    [[maybe_unused]] const auto alloc = usb.allocate_endpoint_buffer(64u);
    [[maybe_unused]] const auto dpram = usb.reset_dpram();

    // ── Phase 3: interrupts + IRQ vector ─────────────────────────────────────

    [[maybe_unused]] const auto ie_rst  =
        usb.enable_interrupt(hal::usb::InterruptKind::Reset);
    [[maybe_unused]] const auto ie_susp =
        usb.enable_interrupt(hal::usb::InterruptKind::Suspend);
    [[maybe_unused]] const auto ie_rsm  =
        usb.enable_interrupt(hal::usb::InterruptKind::Resume);
    [[maybe_unused]] const auto ie_sof  =
        usb.enable_interrupt(hal::usb::InterruptKind::Sof);
    [[maybe_unused]] const auto ie_ep   =
        usb.enable_interrupt(hal::usb::InterruptKind::EndpointTransfer);
    [[maybe_unused]] const auto ie_lc   =
        usb.enable_interrupt(hal::usb::InterruptKind::ConnectionChange);

    [[maybe_unused]] const auto id_rst  =
        usb.disable_interrupt(hal::usb::InterruptKind::Reset);
    [[maybe_unused]] const auto id_sof  =
        usb.disable_interrupt(hal::usb::InterruptKind::Sof);

    [[maybe_unused]] const auto irqs = decltype(usb)::irq_numbers();

    [[maybe_unused]] constexpr auto ep_count = decltype(usb)::endpoint_count();
    [[maybe_unused]] constexpr bool hs = decltype(usb)::supports_high_speed();
}

}  // namespace

#endif
