# Tasks: Extend Ethernet Coverage

## 1. Mode + PHY interface + MDIO

- [x] 1.1 `enum class LinkSpeed { Mbit10, Mbit100, Gbit1 }`,
      `set_speed(LinkSpeed)` (Gbit1 returns NotSupported on
      published devices).
- [x] 1.2 `set_full_duplex(bool)`.
- [x] 1.3 `enum class PhyInterface { Mii, Rmii, Rgmii }`,
      `set_phy_interface(PhyInterface)` — gated on `kSupportsMii`
      / `kRmiiEnableField.valid`.
- [x] 1.4 `set_mdc_clock_divider(std::uint8_t)`,
      `enable_management_port(bool)` — gated.

## 2. DMA descriptor rings + statistics

- [x] 2.1 `struct RxDescriptor`, `struct TxDescriptor` (cross-
      vendor superset).
      `configure_rx_descriptors(std::span<RxDescriptor>)`,
      `configure_tx_descriptors(std::span<TxDescriptor>)`.
- [x] 2.2 `enable_rx(bool)`, `enable_tx(bool)`,
      `set_rx_descriptor_base(std::uintptr_t)`,
      `set_tx_descriptor_base(std::uintptr_t)`.
- [x] 2.3 `enum class StatisticId { RxFrames, RxBytes, RxErrors,
      RxCrcErrors, RxOverflow, TxFrames, TxBytes, TxErrors,
      TxCollisions }`.
      `read_statistic(StatisticId) -> std::uint32_t`,
      `clear_statistics()` — gated on `kHasStatisticsCounters`.

## 3. Interrupts + IRQ vector + concept conformance

- [x] 3.1 `enum class InterruptKind { RxComplete, TxComplete,
      RxError, TxError, ManagementDone, LinkChange }`.
      `enable_interrupt` / `disable_interrupt` — per-kind gated.
- [x] 3.2 `irq_numbers() -> std::span<const std::uint32_t>`.
- [x] 3.3 `port_handle<P>` satisfies `EthernetMac` concept from
      archived `add-network-hal`. `static_assert` in
      `tests/compile_tests/test_eth_descriptor_handle.cpp`.

## 4. Compile tests + async + example + HW

- [x] 4.1 Replace hand-rolled `Same70Gmac` callers with
      `port_handle<GMAC>`. No public-API change.
- [ ] 4.2 `async::eth::wait_for(InterruptKind)` and
      `async::eth::receive_frame()` runtime siblings.
- [ ] 4.3 `examples/ethernet_probe_complete/`: targets
      `same70_xplained` GMAC + KSZ8081, lwIP ICMP echo.
- [ ] 4.4 SAME70 hardware spot-check: 1 minute of ICMP + ARP
      with no errors.
- [ ] 4.5 Update `docs/SUPPORT_MATRIX.md` `ethernet` row.

## 5. Documentation + follow-ups

- [ ] 5.1 `docs/ETHERNET.md` — model, MII/RMII recipe, descriptor
      ring sizing, lwIP adapter integration, link-change handling.
- [ ] 5.2 Cross-link from `docs/NETWORK.md` and `docs/ASYNC.md`.
- [ ] 5.3 File `add-eth-ptp` follow-up.
