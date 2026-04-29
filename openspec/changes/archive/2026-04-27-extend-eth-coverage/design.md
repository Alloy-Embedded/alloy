# Design: Extend Ethernet Coverage

## Context

`add-network-hal` (archived) defines `EthernetMac` /
`MdioBus` / `NetworkInterface` concepts. The current GMAC handle
in `boards/same70_xplained/board_ethernet.hpp` is a hand-rolled
`Same70Gmac` shim. This change ships the descriptor-driven
peripheral handle that satisfies `EthernetMac` on every published
device.

## Goals

1. Replace hand-rolled `Same70Gmac` with a descriptor-driven
   `port_handle<P>` that satisfies `EthernetMac`.
2. Stay schema-agnostic via `if constexpr` capability gates.
3. Compose with `complete-async-hal`'s pattern.
4. lwIP via `drivers/net/lwip_adapter` keeps working.

## Key Decisions

### Decision 1: Descriptor rings owned by the application

Rather than allocating descriptor rings inside the handle, the
HAL takes spans:

```cpp
configure_rx_descriptors(std::span<RxDescriptor> ring);
configure_tx_descriptors(std::span<TxDescriptor> ring);
```

This matches lwIP's expectation that the application provides
the buffer pool. `RxDescriptor` and `TxDescriptor` types are
defined in the HAL with the cross-vendor superset of fields
(STM32 EMAC vs SAM GMAC layouts differ; the HAL converts).

### Decision 2: `LinkSpeed` and `PhyInterface` are typed enums

`LinkSpeed { Mbit10, Mbit100, Gbit1 }` — `Gbit1` returns
`NotSupported` on every published device today.
`PhyInterface { Mii, Rmii, Rgmii }` — gated on `kSupportsMii` /
`kRmiiEnableField.valid`.

### Decision 3: Statistics use a typed `StatisticId`

```cpp
enum class StatisticId : std::uint8_t {
    RxFrames, RxBytes, RxErrors, RxCrcErrors, RxOverflow,
    TxFrames, TxBytes, TxErrors, TxCollisions, ...
};
```

The descriptor publishes per-counter offsets; the HAL maps the
typed enum to the right offset. Backends without the counter
return 0 (not `NotSupported` — counters are observational).

## Risks

- **Descriptor ring layout differs between STM32 EMAC and SAM
  GMAC.** STM32 has 4×32-bit; SAM has 2×32-bit. The HAL's
  `RxDescriptor` / `TxDescriptor` carry the union of fields; the
  vendor-specific layout is hidden behind `if constexpr` gating
  on `kSchemaId`.
- **lwIP buffer alignment.** lwIP expects 4-byte alignment;
  GMAC needs 8-byte. The HAL documents the alignment requirement.

## Migration

The existing `Same70Gmac` shim is replaced by the descriptor-
driven handle. Boards that referenced `Same70Gmac` are migrated
to `port_handle<GMAC>`; the public surface (`EthernetMac`
concept) is identical so application code is unchanged.
