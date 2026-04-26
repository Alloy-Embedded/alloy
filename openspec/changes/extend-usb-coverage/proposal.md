# Extend USB Coverage To Match Published Descriptor Surface

## Why

`UsbSemanticTraits<P>` publishes the full device-controller register
surface: control / device-control / device-interrupt-enable / mask /
status / disable, host-control / host-interrupt-enable / mask /
disable / status, address + address-enable, force-device-mode /
force-host-mode, freeze-clock + clock-usable, base addresses for
DPRAM (USB DPRAM), DM/DP pin ids (typed `PinId`), capability flags
(`kCrystalless`, `kHasClockFreeze`, `kHasDedicatedEndpointConfig`,
`kHardwarePresent`).

Alloy has the `add-usb-hal` openspec for the **concept-level**
USB types (UsbDeviceController concept, descriptor builder, setup
packet). This change is the **descriptor-driven** sibling: a
`port_handle<P>` HAL that consumes the published USB traits per
device, capability-gated via `if constexpr`. Together they form
the USB stack: `add-usb-hal` defines what a controller looks like;
`extend-usb-coverage` ships the typed peripheral handle that
satisfies the concept on every device the descriptor publishes.

## What Changes

### `src/hal/usb/usb_port.hpp` — descriptor-driven USB peripheral handle

- **Mode selection**:
  `enum class UsbMode { Device, Host }`.
  `set_mode(UsbMode)` — gated on
  `kForceDeviceModeField.valid && kForceHostModeField.valid`.
- **Address management**:
  `set_address(std::uint8_t addr)`,
  `enable_address(bool)` — typed write through
  `kAddressField` / `kAddressEnableField`.
- **Clock control** (gated on `kHasClockFreeze`):
  `freeze_clock()`, `unfreeze_clock()`, `clock_usable() -> bool`.
- **Crystalless mode** (gated on `kCrystalless`):
  `enable_crystalless(bool)` — for STM32G0 USB-FS where HSI48
  drives the controller without an external crystal.
- **Pin accessors**:
  `static constexpr auto dm_pin() -> device::PinId`,
  `static constexpr auto dp_pin() -> device::PinId` —
  return descriptor's `kDmPin` / `kDpPin`.
- **Endpoint configuration** (gated on
  `kHasDedicatedEndpointConfig`):
  `configure_endpoint(EndpointConfig)` —
  `EndpointConfig { number, direction, type (Control/Bulk/
  Interrupt/Iso), max_packet_size }`.
- **DPRAM allocator** (gated on `kDpramBaseAddress` non-zero):
  `allocate_endpoint_buffer(std::size_t bytes) ->
  Result<std::span<std::byte>, ErrorCode>` — returns a span into
  the controller's dual-port RAM.
- **Status / interrupts** (typed):
  `enum class InterruptKind { Reset, Suspend, Resume, Setup,
  EndpointTransfer, Sof, Error, ConnectionChange (host) }`.
  `enable_interrupt(InterruptKind)` /
  `disable_interrupt(InterruptKind)`.
- **NVIC vector lookup**: `irq_numbers() ->
  std::span<const std::uint32_t>`.
- **Async sibling**: `async::usb::wait_for(InterruptKind)`.

### Concept conformance

`port_handle<P>` MUST satisfy the
`alloy::hal::usb::UsbDeviceController` concept defined by
`add-usb-hal`. Compile test
`tests/compile_tests/test_usb_descriptor_handle.cpp`
instantiates `port_handle<USB>` against the concept on every
device with `kHardwarePresent = true`.

### `examples/usb_probe_complete/`

Targets `nucleo_g071rb` USB-FS in device mode with crystalless
clock. CDC-ACM virtual COM port using the
`drivers/usb/cdc_acm/` driver from `add-usb-hal`. Async receive
task `co_await async::usb::wait_for(EndpointTransfer)`.

### Docs

`docs/USB.md` — model, device-mode recipe, host-mode recipe,
endpoint configuration + DPRAM allocation, crystalless setup,
async wiring, modm migration table.

## What Does NOT Change

- The `add-usb-hal` openspec for concept-level types remains the
  contract this change satisfies. This change does NOT redefine
  the concept; it ships a conforming peripheral handle.
- USB tier in `docs/SUPPORT_MATRIX.md` stays
  `not-yet-implemented` until both this change AND `add-usb-hal`
  ship together.

## Out of Scope (Follow-Up Changes)

- Host-mode-specific extensions (HCD lifecycle, attached-device
  enumeration). Tracked as `add-usb-host-mode`.
- Isochronous endpoint scheduling. Tracked as `add-usb-iso`.
- High-speed USB (only full-speed in scope today). Tracked as
  `add-usb-hs` once a high-speed-capable family lands.
- ESP32 / RP2040 USB parity — gated on those families publishing
  USB traits.

## Alternatives Considered

Implementing the full USB stack here without `add-usb-hal` —
rejected. Concept-level types and class-driver scaffolds (CDC-ACM,
HID, MSC) belong in `add-usb-hal`; this change is strictly the
descriptor-driven peripheral handle that satisfies the concept.
