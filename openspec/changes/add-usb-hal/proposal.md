# Add USB HAL

## Why

USB is the most common interface for firmware update, serial consoles, HID devices,
and mass storage. Every competitor — Zephyr, modm, Embassy, mbed — has USB support.
Alloy v0.1.0 has no USB HAL concept, no backend, and no USB-related example.

Without USB, Alloy cannot target the large class of devices that use USB for:
- Virtual COM port (CDC-ACM) — the most common bring-up interface after UART
- DFU (Device Firmware Upgrade) — standardized firmware update without a programmer
- HID — keyboards, mice, gamepads, custom control panels
- Mass Storage / MSC — SD card readers, data loggers
- Vendor class — custom USB protocols for instrumentation

This change introduces a layered USB HAL:

1. **USB device controller concept** — hardware-agnostic MAC-like abstraction for
   the on-chip USB hardware (endpoint management, descriptor submission, data transfer).
2. **USB descriptor builder** — compile-time descriptor tree (device, config, interface,
   endpoint) using C++20 template metaprogramming. No heap, no `memcpy` at runtime.
3. **USB class drivers** — CDC-ACM (virtual COM), DFU, and HID as the first three.
   Each class driver is templated over a `UsbDeviceController` concept.
4. **Backend implementations** — STM32F4/L4/H7 FS/HS USB OTG, STM32G0 USB FS, nRF52840
   USBD, and SAME70 UOTGHS as the first concrete controllers.

## What Changes

### `src/hal/usb/` — USB device controller HAL

`usb_device_controller.hpp`:

```cpp
template <typename T>
concept UsbDeviceController = requires(T& c,
    EndpointAddress ep, std::span<const std::byte> tx, std::span<std::byte> rx) {
    { c.configure(UsbSpeed{}) } -> core::ResultLike;
    { c.connect() }             -> core::ResultLike;
    { c.disconnect() }          -> core::ResultLike;
    { c.enable_endpoint(ep, EndpointType{}, std::size_t{}) } -> core::ResultLike;
    { c.write(ep, tx) }         -> core::ResultLike;
    { c.read(ep, rx) }          -> core::Result<std::size_t>;
    { c.stall(ep) }             -> core::ResultLike;
    { c.set_address(std::uint8_t{}) } -> core::ResultLike;
};
```

`usb_descriptor.hpp`: compile-time descriptor tree. `UsbDeviceDescriptor<...>`,
`UsbConfigDescriptor<...>`, `UsbInterfaceDescriptor<...>`, `UsbEndpointDescriptor<...>`
compose via template parameters into a flat `std::array<std::byte, N>` at constexpr time.
Zero runtime allocation.

`usb_setup_packet.hpp`: `SetupPacket` struct with typed field accessors. `RequestType`,
`StandardRequest` enums.

### `src/hal/usb/backends/` — controller backends

`stm32_usb_fs.hpp`: `StmUsbFs<PeripheralId>` — covers STM32G0/L0/F0/WB FS. Packet
buffer memory (PMA) management, endpoint FIFO allocation.

`stm32_usb_otg_fs.hpp`: `StmUsbOtgFs<PeripheralId>` — covers STM32F4/L4/H7 full-speed
OTG core. FIFO allocation, IN/OUT endpoint control.

`nrf52_usbd.hpp`: `Nrf52Usbd` — nRF52840 USBD peripheral. POWER/USBD event model,
ERRATA-166 and ERRATA-187 workarounds.

`same70_uotghs.hpp`: `Same70Uotghs` — SAME70 UOTGHS in device mode. DMA endpoint
mode for bulk endpoints.

### `drivers/usb/` — USB class drivers

`cdc_acm/cdc_acm.hpp`: `CdcAcm<Controller>` implementing virtual COM port.
Exposes `write(span)`, `read(span)` returning `core::Result` — the same interface as
the UART HAL, enabling CDC-ACM as a drop-in `ByteStream` for Modbus USB variant.
`LineCoding` struct for baud rate handling (line coding changes are notified to the
application via callback; CDC-ACM over USB has no baud rate concept, but host software
sends it anyway).

`dfu/dfu.hpp`: `DfuRuntime<Controller, FlashBackend>` implementing DFU Runtime mode
(detach transition) and DFU Download mode (firmware upload to flash). `FlashBackend`
concept: `erase(addr, size)`, `write(addr, data)`, `read(addr, data)`. The W25Q flash
driver satisfies `FlashBackend` out of the box.

`hid/hid.hpp`: `HidDevice<Controller, ReportDescriptor>` with `send_report(span)` and
optional `recv_report_callback`. `ReportDescriptor` is a compile-time byte array
(constexpr).

### Examples

`examples/usb_cdc_acm/`: virtual COM port. Host sees `/dev/ttyACM0`. Echo server plus
loopback. Builds for `nucleo_g071rb` (STM32G0 USB FS).

`examples/usb_dfu/`: DFU Runtime + DFU Download. Host sees DFU device via `dfu-util`.
Flash the Alloy blink binary via USB without a JTAG probe. Target: `nucleo_g071rb`.

`examples/usb_hid_gamepad/`: HID gamepad with two axes and four buttons. ADC → axes,
GPIO → buttons. Host sees it as a USB HID gamepad in any OS without a driver.
Target: `nucleo_g071rb`.

### Documentation

`docs/USB.md`: USB HAL guide — controller concept, descriptor builder, class driver
usage, ISR wiring, DFU workflow, HID report descriptor construction.

## What Does NOT Change

- UART HAL — CDC-ACM provides the same `write`/`read` interface but is an additive
  driver, not a replacement.
- The `alloy-devices` contract — USB controller semantics are in `usb_device_controller.hpp`
  and do not require new entries in the generated artifact (USB controller config is
  static in the backend, not descriptor-driven).

## Alternatives Considered

**TinyUSB integration:** TinyUSB is the de-facto bare-metal USB stack. It provides
CDC, DFU, HID, MSC, MIDI, and vendor class support with proven hardware drivers.
The downside is a C API that does not compose with Alloy's `Result<T>` error model and
requires global callbacks. The chosen approach uses TinyUSB's hardware HAL layer
(device port) as a backend option while providing an Alloy-native API on top.

**USB host mode:** Out of scope for this change. USB device (peripheral mode) covers
95% of embedded use cases. USB host would require the OTG host stack and is a follow-up.
