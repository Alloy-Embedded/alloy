## ADDED Requirements

### Requirement: Runtime SHALL Provide An MdioBus Concept And A SAME70 Backend

The runtime MUST define a typed `alloy::hal::mdio::MdioBus<T>` concept whose
`read(phy, reg)` and `write(phy, reg, value)` operations are `noexcept` and
return typed `core::Result` values. Errors MUST be reported as
`core::ErrorCode::Timeout` when the bus busy-state never clears within a
bounded retry budget. The runtime MUST also ship a SAME70 backend
(`Same70Mdio`) that satisfies the concept and drives the SAM-E70 GMAC's
Clause-22 management interface. PHY ownership of the GMAC peripheral
registers MUST be coordinated with the Ethernet MAC backend so the two do
not race during initialisation.

#### Scenario: Driver authors can constrain on MdioBus

- **WHEN** a PHY driver template is parameterised over a bus type
- **THEN** it constrains the parameter via the `MdioBus` concept
- **AND** the constraint admits any class providing `noexcept` `read` /
  `write` returning `core::Result` with `core::ErrorCode::Timeout` reserved
  for busy-timeout

#### Scenario: SAME70 MDIO times out when the bus stays busy

- **WHEN** application code reads or writes a Clause-22 register through
  `Same70Mdio` while `GMAC_NSR.IDLE` never asserts within the documented
  retry cap
- **THEN** the call returns `core::ErrorCode::Timeout`
- **AND** the GMAC peripheral state is not corrupted (no partial frame
  in flight)

### Requirement: Runtime SHALL Provide An EthernetMac Concept And A SAME70 Backend

The runtime MUST define a typed `alloy::hal::ethernet::EthernetMac<T>`
concept whose `send_frame(span)`, `recv_frame(span)`, `get_mac_address()`,
and `link_up()` operations are `noexcept`. Errors MUST be reported through
a typed `MacError` enum covering at minimum `Busy` (TX ring full),
`FrameTooLarge`, `NoFrame` (RX ring empty), and `HardwareError`. The
runtime MUST ship a SAM-E70 backend (`Same70Gmac<TxN, RxN>`) that satisfies
the concept with parameterised TX and RX descriptor ring sizes. Descriptor
buffers MUST be aligned per the GMAC DMA constraint.

#### Scenario: Frame too large is rejected at the API boundary

- **WHEN** a caller invokes `send_frame` with a span larger than 1518 bytes
- **THEN** the call returns `MacError::FrameTooLarge`
- **AND** no DMA descriptor is consumed

#### Scenario: RX ring empty surfaces NoFrame

- **WHEN** `recv_frame` is called on an empty RX ring
- **THEN** the call returns `MacError::NoFrame` without blocking

#### Scenario: SAME70 GMAC descriptors meet the DMA alignment constraint

- **WHEN** the SAM-E70 backend is instantiated with any compile-time
  `TxN` / `RxN`
- **THEN** the descriptor buffers carry `alignas` sufficient for the GMAC
  DMA engine (8-byte minimum)

### Requirement: Runtime SHALL Provide A Vendor-Neutral NetworkInterface Concept

The runtime MUST define `alloy::net::NetworkInterface<T>` so Ethernet,
hardware-TCP/IP modules, and AT-firmware modules expose the same surface to
the lwIP integration and to application-level transports. The concept MUST
require `up()`, `down()`, `link_up()`, `send_frame(span)`, and a polling
hook for inbound traffic. Errors MUST flow through typed values, never
exceptions.

#### Scenario: Ethernet path satisfies the concept

- **WHEN** an `EthernetInterface<Mac, Phy>` adapter is instantiated over a
  conforming `EthernetMac` and `PhyDriver`
- **THEN** the adapter satisfies `NetworkInterface`
- **AND** the lwIP integration accepts it as the netif source

#### Scenario: WiFi-module paths satisfy the same concept directly

- **WHEN** the W5500 (hardware TCP/IP offload) or ESP-AT (UART AT
  firmware) interface is instantiated
- **THEN** the type satisfies `NetworkInterface` without being split into
  MAC + PHY layers
- **AND** application code targeting `NetworkInterface` is portable across
  Ethernet and WiFi backends

### Requirement: Runtime SHALL Provide A PhyDriver Concept

The runtime MUST define `alloy::net::PhyDriver<T>` so PHY drivers (e.g.
KSZ8081) plug into `EthernetInterface` polymorphically. The concept MUST
require `init(MdioBus&)`, `link_up()`, `negotiated_speed()`, and
`negotiated_duplex()`. PHY drivers MUST NOT touch GMAC registers directly;
all PHY communication MUST flow through the `MdioBus` parameter.

#### Scenario: KSZ8081 satisfies PhyDriver

- **WHEN** `alloy::net::KSZ8081{mdio, phy_addr}` is instantiated
- **THEN** the type satisfies `PhyDriver`
- **AND** all register touches go through the `MdioBus` reference; no
  direct GMAC MMIO is performed by the PHY driver

### Requirement: Runtime SHALL Integrate lwIP Through A Typed Adapter

The runtime MUST integrate lwIP via `alloy::net::LwipAdapter<Interface>`
templated over a `NetworkInterface`-conforming type. The adapter MUST own
the lwIP `netif` instance and MUST drive the lwIP timer and input pumps
from a cooperative polling loop (no RTOS). lwIP MUST be configured without
libc heap (`MEM_LIBC_MALLOC = 0`); its memory pools MUST resolve to fixed
`.bss` buffers. The adapter MUST expose a `TcpStream` type whose byte-stream
operations satisfy `alloy::modbus::ByteStream` so Modbus-TCP framing layers
consume it without a per-transport specialisation.

#### Scenario: TcpStream is interoperable with Modbus-TCP

- **WHEN** a Modbus-TCP slave or master is parameterised over a byte
  stream
- **THEN** it accepts an instance of the runtime's `TcpStream` directly
- **AND** no `ByteStream` trait or wrapper is required

#### Scenario: lwIP runs without libc heap

- **WHEN** the build is configured for any board that pulls in
  `LwipAdapter`
- **THEN** lwIP is compiled with `MEM_LIBC_MALLOC = 0`
- **AND** lwIP's pool allocator is backed by fixed-size buffers in `.bss`
- **AND** no `malloc` / `new` symbols are required by the network HAL

### Requirement: WiFi-Module Backends SHALL Be Scaffolded To Lock The Concept Boundary

The runtime MUST scaffold `W5500Interface` (SPI, hardware TCP/IP offload)
and `EspAtInterface` (UART AT firmware) at concept-conforming stub level
so future full implementations cannot drift from the `NetworkInterface`
contract. Scope gate: this change does NOT deliver full radio bring-up,
authentication, or DHCP integration for either backend; those are
follow-up changes.

#### Scenario: Concept conformance is enforced at compile time

- **WHEN** the W5500 or EspAt scaffold is built
- **THEN** a `static_assert` (or equivalent compile-time check) verifies
  the type satisfies `NetworkInterface`
- **AND** any future change that breaks the concept fails to build

### Requirement: Network HAL SHALL Honour The Existing No-Heap / No-RTOS Constraints

Every network HAL layer MUST be heap-free, RTOS-free, and entirely
`noexcept`. This applies to the MDIO bus, Ethernet MAC, PHY driver,
NetworkInterface, lwIP adapter, and WiFi-module scaffolds alike. Global
mutable state MUST be confined to `LwipAdapter` (which owns the singleton
`netif`); every other layer MUST be free of global mutable state.

#### Scenario: Zero-heap gate covers the network HAL

- **WHEN** the zero-overhead release gate compiles a board that pulls in
  the network HAL
- **THEN** no `malloc` / `new` references appear in the linked image from
  network-HAL code
- **AND** lwIP's `MEM_LIBC_MALLOC = 0` configuration is honoured

#### Scenario: Cooperative polling is the only execution model

- **WHEN** any network-HAL component services traffic
- **THEN** the work is performed from a cooperative `poll()` invocation in
  the application main loop
- **AND** no RTOS task / thread is required by the HAL itself

### Requirement: Boards Wiring Ethernet SHALL Expose Typed Factories

A board that wires its physical Ethernet stack (MAC + PHY + MDIO) SHALL
expose typed factory functions (e.g. `board::make_mdio()`,
`board::make_gmac(mac_addr)`, `board::make_ksz8081(mdio)`) so applications
construct the stack without naming vendor register addresses or
peripheral indices. Hand-rolled per-example MDIO drivers MUST be removed.

#### Scenario: SAME70 Xplained exposes board::make_* factories

- **WHEN** an application targets `boards/same70_xplained/`
- **THEN** the board provides `board::make_mdio`, `board::make_gmac`, and
  `board::make_ksz8081` as `noexcept` factories
- **AND** the existing KSZ8081 probe example uses the board factories,
  not a hand-rolled `Same70GmacMdio`
