# Extend Ethernet Coverage To Match Published Descriptor Surface

## Why

`EthSemanticTraits<P>` publishes config / control / DMA-config /
RX-descriptor-base / RX-status / interrupt-enable / mask / status /
disable registers, full-duplex + RMII-enable + speed +
MDC-clock-divider + MDIO-port-enable + management-done fields,
clear-statistics + RX-enable + RX-complete-interrupt fields, plus
capability flags (`kHasDmaEngine`, `kHasStatisticsCounters`,
`kSupportsMii`).

Alloy already has `add-network-hal` (archived) defining the
`MdioBus` / `EthernetMac` concepts. This change is the
descriptor-driven peripheral handle that satisfies the
`EthernetMac` concept on every device the descriptor publishes
(currently SAME70 GMAC and ST stm32f4 ETH).

## What Changes

### `src/hal/ethernet/ethernet_port.hpp` — descriptor-driven handle

- **Mode**: `enum class LinkSpeed { Mbit10, Mbit100, Gbit1 }`,
  `set_speed(LinkSpeed)`,
  `set_full_duplex(bool)`.
- **PHY interface**:
  `enum class PhyInterface { Mii, Rmii, Rgmii }`,
  `set_phy_interface(PhyInterface)` — gated on `kSupportsMii` /
  `kRmiiEnableField.valid`.
- **MDIO clock divider**:
  `set_mdc_clock_divider(std::uint8_t)`,
  `enable_management_port(bool)` — gated.
- **DMA engine** (gated on `kHasDmaEngine`):
  `configure_rx_descriptors(std::span<RxDescriptor>)`,
  `configure_tx_descriptors(std::span<TxDescriptor>)`,
  `enable_rx(bool)`, `enable_tx(bool)`,
  `set_rx_descriptor_base(std::uintptr_t)`,
  `set_tx_descriptor_base(std::uintptr_t)`.
- **Statistics** (gated on `kHasStatisticsCounters`):
  `read_statistic(StatisticId) -> std::uint32_t`,
  `clear_statistics()`.
- **Status / interrupts**:
  `enum class InterruptKind { RxComplete, TxComplete, RxError,
  TxError, ManagementDone, LinkChange }`.
  `enable_interrupt` / `disable_interrupt` — per-kind gated.
- **NVIC vector lookup**: `irq_numbers() ->
  std::span<const std::uint32_t>`.
- **Async sibling**: `async::eth::wait_for(InterruptKind)` and
  `async::eth::receive_frame()` returning the next received frame.

### Concept conformance

`port_handle<P>` MUST satisfy `alloy::hal::ethernet::EthernetMac`
(from the archived `add-network-hal`). Replaces the existing
hand-rolled `Same70Gmac` shim — now the descriptor's typed handle
IS the GMAC implementation.

### `examples/ethernet_probe_complete/`

Targets `same70_xplained` GMAC. Configures 100 Mbit/s full-duplex
RMII with KSZ8081 PHY, async receive task feeding into lwIP via
`drivers/net/lwip_adapter`, ICMP echo round-trip.

### Docs

`docs/ETHERNET.md` — model, MII vs RMII recipe, descriptor ring
allocation, statistics, link-change handling, async wiring,
`drivers/net/lwip_adapter` integration recipe.

## What Does NOT Change

- The archived `add-network-hal` concepts (`EthernetMac`, `MdioBus`,
  `NetworkInterface`) are unchanged. This change ships a
  descriptor-driven handle that satisfies them.
- Ethernet tier in `docs/SUPPORT_MATRIX.md` stays
  `representative` until hardware spot-check matrix lands.

## Out of Scope

- 1 Gbit/s Ethernet (no current device supports it).
- IEEE 1588 / PTP. Tracked as `add-eth-ptp`.
- Hardware spot-checks → `validate-eth-coverage-on-3-boards`.

## Alternatives Considered

Implementing the full TCP/IP stack — out of scope. The
`drivers/net/lwip_adapter` already integrates lwIP via the
`NetworkInterface` concept; this change makes the GMAC side feed
that adapter via the descriptor-driven handle.
