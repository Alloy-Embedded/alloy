# public-hal-api Spec Delta: Ethernet Coverage Extension

## ADDED Requirements

### Requirement: Ethernet HAL SHALL Expose Speed / Duplex / PHY Interface / MDIO Setters

The `alloy::hal::ethernet::port_handle<P>` MUST expose:

- `enum class LinkSpeed { Mbit10, Mbit100, Gbit1 }`,
  `set_speed(LinkSpeed)`. `Gbit1` MUST return `NotSupported` on
  every device whose published surface caps at 100 Mbit/s.
- `set_full_duplex(bool)` — gated on `kFullDuplexField.valid`.
- `enum class PhyInterface { Mii, Rmii, Rgmii }`,
  `set_phy_interface(PhyInterface)` — gated on `kSupportsMii` /
  `kRmiiEnableField.valid`.
- `set_mdc_clock_divider(std::uint8_t)`,
  `enable_management_port(bool)` — gated on `kMdcClockDividerField` /
  `kManagementPortEnableField.valid`.

#### Scenario: SAM-E70 GMAC runs 100 Mbit/s full-duplex RMII

- **WHEN** an application calls `eth.set_speed(LinkSpeed::Mbit100)`,
  `eth.set_full_duplex(true)`, and
  `eth.set_phy_interface(PhyInterface::Rmii)` on GMAC of
  `same70_xplained`
- **THEN** all calls succeed and the GMAC NCFGR register is
  programmed accordingly

### Requirement: Ethernet HAL SHALL Expose Descriptor Ring Configuration

The HAL MUST expose `struct RxDescriptor` and `struct TxDescriptor`
plus `configure_rx_descriptors(std::span<RxDescriptor>)` /
`configure_tx_descriptors(std::span<TxDescriptor>)`,
`enable_rx(bool)`, `enable_tx(bool)`,
`set_rx_descriptor_base(std::uintptr_t)`, and
`set_tx_descriptor_base(std::uintptr_t)`. All gated on
`kHasDmaEngine`.

The descriptor structs are the cross-vendor superset; the
vendor-specific layout is hidden behind `if constexpr` gating on
`kSchemaId`.

#### Scenario: Application provides RX descriptor ring and starts reception

- **WHEN** an application allocates `std::array<RxDescriptor, 16>`
  with proper alignment, calls
  `eth.configure_rx_descriptors(ring)` and `eth.enable_rx(true)`
- **THEN** the GMAC begins writing received frames into the
  application-owned ring buffers

### Requirement: Ethernet HAL SHALL Expose Statistics Counters Per Capability

The HAL MUST expose
`enum class StatisticId { RxFrames, RxBytes, RxErrors,
RxCrcErrors, RxOverflow, TxFrames, TxBytes, TxErrors,
TxCollisions }` plus `read_statistic(StatisticId) ->
std::uint32_t` and `clear_statistics()` whenever
`kHasStatisticsCounters`. Backends without a particular counter
MUST return 0 (counters are observational; not present ≠ error).

#### Scenario: RxFrames counter survives until cleared

- **WHEN** an application reads `eth.read_statistic(StatisticId::RxFrames)`
  twice with packet activity in between
- **THEN** the second read returns a value greater than the first
- **AND** `eth.clear_statistics()` resets every counter to 0

### Requirement: Ethernet HAL SHALL Satisfy The EthernetMac Concept

`port_handle<P>` MUST satisfy
`alloy::hal::ethernet::EthernetMac` (defined by archived
`add-network-hal`) on every device with `kHasDmaEngine`. A
compile test MUST `static_assert` conformance.

#### Scenario: GMAC handle replaces hand-rolled Same70Gmac shim

- **WHEN** a board file constructs `port_handle<GMAC>{config}`
  and passes it to `EthernetInterface<Mac, Phy>`
- **THEN** the interface compiles and runs identically to the
  prior `Same70Gmac` shim
- **AND** lwIP integration via `drivers/net/lwip_adapter` works
  unchanged

### Requirement: Ethernet HAL SHALL Expose Typed Interrupt Setters And IRQ Number List

The HAL MUST expose
`enum class InterruptKind { RxComplete, TxComplete, RxError,
TxError, ManagementDone, LinkChange }` plus
`enable_interrupt(InterruptKind)` /
`disable_interrupt(InterruptKind)` (each kind gated), and
`irq_numbers() -> std::span<const std::uint32_t>`.

#### Scenario: LinkChange is gated on the link-change field

- **WHEN** an application calls
  `eth.enable_interrupt(InterruptKind::LinkChange)` on a peripheral
  whose link-change IE field is invalid
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Async Ethernet Adapter SHALL Add wait_for And receive_frame

The runtime `async::eth` namespace MUST expose
`wait_for<P>(handle, InterruptKind kind)` plus
`receive_frame<P>(handle) -> core::Result<operation<…>,
core::ErrorCode>` whose awaiter result yields the next received
frame.

#### Scenario: Coroutine receives the next frame

- **WHEN** a task awaits
  `async::eth::receive_frame<GMAC>(eth)` while frames are arriving
- **THEN** the task resumes when the next frame lands and the
  awaiter's result contains the frame data + length
