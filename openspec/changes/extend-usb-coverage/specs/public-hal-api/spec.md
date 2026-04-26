# public-hal-api Spec Delta: USB Coverage Extension

## ADDED Requirements

### Requirement: USB HAL SHALL Expose Mode / Address / Clock / Crystalless / Pin Accessors Per Capability

The `alloy::hal::usb::port_handle<P>` MUST expose
`enum class UsbMode { Device, Host }` plus `set_mode(UsbMode)`
gated on `kForceDeviceModeField.valid && kForceHostModeField.valid`,
`set_address(std::uint8_t)` and `enable_address(bool)`,
`freeze_clock()` / `unfreeze_clock()` / `clock_usable() -> bool`
gated on `kHasClockFreeze`,
`enable_crystalless(bool)` gated on `kCrystalless`,
and static accessors `dm_pin() -> device::PinId` /
`dp_pin() -> device::PinId` returning descriptor's `kDmPin` /
`kDpPin`. Backends without a capability MUST return
`core::ErrorCode::NotSupported`.

#### Scenario: STM32G0 USB-FS runs crystalless from HSI48

- **WHEN** an application calls `usb.enable_crystalless(true)` on
  USB of `nucleo_g071rb`
- **THEN** the call succeeds and HSI48 drives the USB controller
  without an external crystal

### Requirement: USB HAL SHALL Expose Endpoint Configuration And DPRAM Allocator

The HAL MUST expose `struct EndpointConfig { number, direction,
type, max_packet_size }` with `enum class EndpointType { Control,
Bulk, Interrupt, Iso }` and `enum class Direction { In, Out }`,
plus `configure_endpoint(EndpointConfig)` gated on
`kHasDedicatedEndpointConfig`. The HAL MUST also expose
`allocate_endpoint_buffer(std::size_t bytes) ->
Result<std::span<std::byte>, ErrorCode>` and `reset_dpram()`
gated on `kDpramBaseAddress` non-zero. Out-of-DPRAM allocations
MUST return `core::ErrorCode::NoMemory`.

#### Scenario: Allocator hands out spans into DPRAM

- **WHEN** an application calls
  `auto r = usb.allocate_endpoint_buffer(64u)` on USB of
  `nucleo_g071rb`
- **THEN** the call returns `Ok` with a `std::span<std::byte>`
  whose `.data()` points inside the controller's DPRAM region
  and `.size() == 64`

### Requirement: USB HAL SHALL Satisfy The UsbDeviceController Concept On Every Published Device

`port_handle<P>` MUST satisfy the
`alloy::hal::usb::UsbDeviceController` concept (defined by
`add-usb-hal`) on every device whose
`UsbSemanticTraits<P>::kHardwarePresent` is true. A compile test
MUST `static_assert` conformance for the foundational set
(ST G0/F4, SAM-E70, NXP iMXRT).

#### Scenario: Concept conformance is enforced at compile time

- **WHEN** a contributor breaks the concept by removing a method
  the concept requires
- **THEN** the build fails because
  `static_assert(UsbDeviceController<port_handle<USB>>)` no longer
  holds

### Requirement: USB HAL SHALL Expose Typed Interrupt Setters And IRQ Number List

The HAL MUST expose
`enum class InterruptKind { Reset, Suspend, Resume, Setup,
EndpointTransfer, Sof, Error, ConnectionChange }` plus
`enable_interrupt(InterruptKind)` /
`disable_interrupt(InterruptKind)` (each kind gated on its IE
field), and `irq_numbers() -> std::span<const std::uint32_t>`.

#### Scenario: ConnectionChange is gated on host-mode availability

- **WHEN** an application calls
  `usb.enable_interrupt(InterruptKind::ConnectionChange)` on a
  device-mode-only peripheral
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Async USB Adapter SHALL Add wait_for(InterruptKind)

The runtime `async::usb` namespace MUST expose
`wait_for<P>(handle, InterruptKind kind)` so a coroutine can
`co_await` a Setup / EndpointTransfer / Reset event.

#### Scenario: Coroutine wakes on endpoint transfer complete

- **WHEN** a task awaits
  `async::usb::wait_for<USB>(usb, InterruptKind::EndpointTransfer)`
  while a CDC-ACM bulk transfer is in flight
- **THEN** the task resumes when the endpoint-transfer interrupt
  fires and the awaiter returns `Ok`
