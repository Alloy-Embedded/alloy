# Design: Extend USB Coverage

## Context

`add-usb-hal` (existing 0% open openspec) defines the
concept-level USB types: `UsbDeviceController`, descriptor builder,
setup packet, class drivers. This change is the descriptor-driven
peripheral handle that satisfies that concept on every published
device.

The two changes are **paired**: ship them together. Neither alone
gives users a working USB stack.

## Goals

1. Ship a `port_handle<P>` USB peripheral handle that satisfies
   the `add-usb-hal` `UsbDeviceController` concept.
2. Lift every published `UsbSemanticTraits<P>` field into the
   handle, capability-gated via `if constexpr`.
3. Compose with `complete-async-hal`'s pattern.

## Non-Goals

- Host-mode HCD lifecycle (attached-device enumeration). Separate
  follow-up.
- Isochronous endpoints, high-speed USB.
- Class drivers (CDC-ACM, HID, MSC) — they live in `add-usb-hal`
  consuming this handle.

## Key Decisions

### Decision 1: Mode selection via `UsbMode` enum

The descriptor publishes `kForceDeviceModeField` +
`kForceHostModeField`. The HAL consolidates them into a single
`set_mode(UsbMode::Device | UsbMode::Host)` call that writes the
correct field combination.

### Decision 2: DPRAM allocator returns `std::span<std::byte>`

USB controllers have dual-port RAM that backs endpoint buffers.
The HAL's allocator hands out spans into that DPRAM. Allocations
are bumped from a single internal cursor (no free); applications
call `reset_dpram()` when reconfiguring endpoints. Mirrors what
modm does on STM32 / SAM.

### Decision 3: `EndpointConfig` is the cross-vendor superset

```cpp
struct EndpointConfig {
    std::uint8_t  number;       // 0..15
    Direction     direction;    // In | Out
    EndpointType  type;         // Control | Bulk | Interrupt | Iso
    std::uint16_t max_packet_size;
};
```

Backends that limit the matrix (e.g. only one OUT endpoint per
number) return `InvalidArgument` for unsupported configurations.

### Decision 4: Crystalless mode is its own gate

`kCrystalless` is the descriptor's flag for HSI48-driven USB
without external crystal. The HAL exposes `enable_crystalless(bool)`
gated on it; STM32 G0/F0/F4 with crystalless support light up,
others return `NotSupported`.

## Risks

- **Concept drift between `add-usb-hal` and this change.** If
  `add-usb-hal` evolves the concept, `port_handle<P>` must keep
  satisfying it. Mitigated by the compile test that `static_assert`s
  conformance on every device with `kHardwarePresent = true`.
- **DPRAM allocator fragmentation.** Bump-only is the simplest
  model; users reset and reconfigure on USB enumeration changes.
  Documented.

## Migration

This is the first version of the descriptor-driven USB handle. No
migration concern.
