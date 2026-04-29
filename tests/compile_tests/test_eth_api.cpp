// Compile test: Ethernet HAL API surface is instantiable on boards that
// publish ETH semantics. Targeted at same70_xplained (PeripheralId::GMAC).
//
// Ref: openspec/changes/extend-eth-coverage

#include "device/runtime.hpp"
#include "hal/eth.hpp"

static_assert(alloy::device::SelectedRuntimeDescriptors::available);

#if ALLOY_DEVICE_ETH_SEMANTICS_AVAILABLE && defined(ALLOY_BOARD_SAME70_XPLD)

namespace {

void exercise_eth_handle() {
    using namespace alloy;

    auto mac = hal::eth::open<device::PeripheralId::GMAC>();
    static_assert(decltype(mac)::valid);

    // ── Phase 1: mode + PHY interface + MDIO ─────────────────────────────────

    [[maybe_unused]] const auto spd10  = mac.set_speed(hal::eth::LinkSpeed::Mbit10);
    [[maybe_unused]] const auto spd100 = mac.set_speed(hal::eth::LinkSpeed::Mbit100);
    [[maybe_unused]] const auto spd1g  = mac.set_speed(hal::eth::LinkSpeed::Gbit1);

    [[maybe_unused]] const auto fd  = mac.set_full_duplex(true);
    [[maybe_unused]] const auto hd  = mac.set_full_duplex(false);

    [[maybe_unused]] const auto mii  = mac.set_phy_interface(hal::eth::PhyInterface::Mii);
    [[maybe_unused]] const auto rmii = mac.set_phy_interface(hal::eth::PhyInterface::Rmii);
    [[maybe_unused]] const auto rgmii = mac.set_phy_interface(hal::eth::PhyInterface::Rgmii);

    [[maybe_unused]] const auto mdc = mac.set_mdc_clock_divider(4u);
    [[maybe_unused]] const auto mpe = mac.enable_management_port(true);

    // ── Phase 2: descriptors + statistics ────────────────────────────────────

    [[maybe_unused]] const auto rxen  = mac.enable_rx(true);
    [[maybe_unused]] const auto txen  = mac.enable_tx(true);
    [[maybe_unused]] const auto rxdis = mac.enable_rx(false);
    [[maybe_unused]] const auto txdis = mac.enable_tx(false);

    hal::eth::RxDescriptor rx_ring[4]{};
    hal::eth::TxDescriptor tx_ring[4]{};

    [[maybe_unused]] const auto rx_desc =
        mac.configure_rx_descriptors(std::span{rx_ring});
    [[maybe_unused]] const auto tx_desc =
        mac.configure_tx_descriptors(std::span{tx_ring});

    [[maybe_unused]] const auto rx_base = mac.set_rx_descriptor_base(0x20000000u);
    [[maybe_unused]] const auto tx_base = mac.set_tx_descriptor_base(0x20001000u);

    [[maybe_unused]] const auto tstart = mac.tx_start();

    [[maybe_unused]] const auto stat_rx = mac.read_statistic(hal::eth::StatisticId::RxFrames);
    [[maybe_unused]] const auto stat_tx = mac.read_statistic(hal::eth::StatisticId::TxBytes);
    [[maybe_unused]] const auto clrstat = mac.clear_statistics();

    // ── Phase 3: interrupts + IRQ vector ─────────────────────────────────────

    [[maybe_unused]] const auto ie_rx  =
        mac.enable_interrupt(hal::eth::InterruptKind::RxComplete);
    [[maybe_unused]] const auto ie_tx  =
        mac.enable_interrupt(hal::eth::InterruptKind::TxComplete);
    [[maybe_unused]] const auto ie_mgmt =
        mac.enable_interrupt(hal::eth::InterruptKind::ManagementDone);
    [[maybe_unused]] const auto ie_lc  =
        mac.enable_interrupt(hal::eth::InterruptKind::LinkChange);

    [[maybe_unused]] const auto id_rx  =
        mac.disable_interrupt(hal::eth::InterruptKind::RxComplete);
    [[maybe_unused]] const auto id_err =
        mac.disable_interrupt(hal::eth::InterruptKind::RxError);

    [[maybe_unused]] const auto irqs = decltype(mac)::irq_numbers();
}

}  // namespace

#endif
