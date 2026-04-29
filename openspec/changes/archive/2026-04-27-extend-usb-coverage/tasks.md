# Tasks: Extend USB Coverage

This change is paired with `add-usb-hal` (concept-level types).
Land them together; neither alone yields a working USB stack.

## 1. Mode + address + clock + crystalless + pin accessors

- [x] 1.1 `enum class UsbMode { Device, Host }`.
      `set_mode(UsbMode)` — gated on
      `kForceDeviceModeField.valid && kForceHostModeField.valid`.
- [x] 1.2 `set_address(std::uint8_t)`, `enable_address(bool)`.
- [x] 1.3 `freeze_clock()`, `unfreeze_clock()`,
      `clock_usable() -> bool` — gated on `kHasClockFreeze`.
- [x] 1.4 `enable_crystalless(bool)` — gated on `kCrystalless`.
- [x] 1.5 `dm_pin()` and `dp_pin()` static accessors returning
      descriptor's `kDmPin` / `kDpPin`.

## 2. Endpoint configuration + DPRAM allocator

- [x] 2.1 `struct EndpointConfig { number, direction, type,
      max_packet_size }`.
- [x] 2.2 `enum class EndpointType { Control, Bulk, Interrupt,
      Iso }`,
      `enum class Direction { In, Out }`.
- [x] 2.3 `configure_endpoint(EndpointConfig)` — gated on
      `kHasDedicatedEndpointConfig`.
- [x] 2.4 `allocate_endpoint_buffer(std::size_t bytes) ->
      Result<std::span<std::byte>, ErrorCode>`,
      `reset_dpram()` — gated on `kDpramBaseAddress` non-zero.

## 3. Status + interrupts + IRQ vector

- [x] 3.1 `enum class InterruptKind { Reset, Suspend, Resume,
      Setup, EndpointTransfer, Sof, Error, ConnectionChange }`.
      `enable_interrupt` / `disable_interrupt` — per-kind gated.
- [x] 3.2 `irq_numbers() -> std::span<const std::uint32_t>`.

## 4. Concept conformance + compile tests

- [ ] 4.1 `port_handle<P>` satisfies `UsbDeviceController` concept
      from `add-usb-hal`. `static_assert` in
      `tests/compile_tests/test_usb_descriptor_handle.cpp`.
- [x] 4.2 Compile test instantiates handle on every device with
      `kHardwarePresent = true` (foundational set: ST G0/F4,
      SAM-E70, NXP iMXRT).

## 5. Async + example + HW

- [ ] 5.1 `async::usb::wait_for(InterruptKind)` runtime sibling.
- [ ] 5.2 `examples/usb_probe_complete/`: targets `nucleo_g071rb`
      USB-FS device mode with crystalless. CDC-ACM virtual COM
      port using `drivers/usb/cdc_acm/` from `add-usb-hal`.
- [ ] 5.3 SAME70 / G0 / F4 hardware spot-check: enumerate as
      virtual COM port on host PC; loopback 1 MB without errors.
- [ ] 5.4 Update `docs/SUPPORT_MATRIX.md` `usb` row.

## 6. Documentation + follow-ups

- [ ] 6.1 `docs/USB.md` — comprehensive guide.
- [ ] 6.2 Cross-link from `docs/ASYNC.md` and `docs/COOKBOOK.md`.
- [ ] 6.3 File `add-usb-host-mode` follow-up.
- [ ] 6.4 File `add-usb-iso` follow-up.
- [ ] 6.5 File `add-usb-hs` once a high-speed family lands.
