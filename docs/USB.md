# Alloy USB HAL

`alloy::hal::usb` is the typed, descriptor-builder-driven USB device-side
abstraction. The HAL is layered:

1. **`UsbDeviceController` C++20 concept** — every backend (STM32 USB FS, OTG
   FS/HS, nRF52 USBD, SAME70 UOTGHS) satisfies this concept by exposing the
   canonical USB device-MAC operations: `configure`, `connect`, `enable_endpoint`,
   `write`, `read`, `stall`, `set_address`, `service`.
2. **Compile-time descriptor builder** — `device_descriptor`,
   `config_descriptor`, `interface_descriptor`, `endpoint_descriptor`, plus
   `concat_descriptors(...)` produce a flat `constexpr std::array<std::byte, N>`
   at compile time. Zero runtime allocation, zero `memcpy`.
3. **Class drivers** — `CdcAcm<Controller>`, `DfuRuntime<Controller>`,
   `DfuDownload<Controller, FlashBackend>`, `HidDevice<Controller, ReportSize>`.
   Each is templated over the controller; class-specific setup-handling and
   descriptor generation live in the driver.

This change ships the foundation: concepts, builders, class drivers, and a
host-side `MockUsbController` that satisfies the concept for unit tests.
**Real backend implementations are deferred to per-family follow-ups** —
`add-stm32-usb-fs-backend`, `add-stm32-otg-fs-backend`, `add-nrf52-usbd-backend`,
`add-same70-uotghs-backend`. Class-driver code can be written today against the
concept; binding to a real controller is a swap of the template parameter.

---

## Quick start: CDC-ACM virtual COM port

```cpp
#include "drivers/usb/cdc_acm/cdc_acm.hpp"
#include "hal/usb/backends/stm32_usb_fs.hpp"   // when the backend lands

using Controller = alloy::hal::usb::backends::StmUsbFs<PeripheralId::USB>;
using CdcAcm     = alloy::drivers::usb::cdc_acm::CdcAcm<Controller>;

Controller controller;
CdcAcm     cdc{controller};

// Build the configuration descriptor at compile time.
constexpr auto kConfig =
    alloy::drivers::usb::cdc_acm::build_configuration_descriptor();

void setup() {
    auto on_setup = [](const SetupPacket& s, void* user) -> auto {
        // Route GET_DESCRIPTOR(Configuration) to kConfig, class requests to cdc.
        if (s.is_class()) {
            return static_cast<CdcAcm*>(user)->handle_class_request(s, {});
        }
        // ...
    };
    controller.configure(UsbSpeed::Full, on_event, on_setup, &cdc);
    controller.connect();
}

// Application code: looks like UART.
auto write_log(std::span<const std::byte> data) {
    return cdc.write(data);
}
```

The host sees `/dev/ttyACM0` (Linux/macOS) or a COM port (Windows). The CDC-ACM
class driver handles `SET_LINE_CODING`, `GET_LINE_CODING`, and
`SET_CONTROL_LINE_STATE` automatically; the application only sees the bulk
write/read interface.

---

## Core concept

```cpp
template <typename T>
concept UsbDeviceController = requires(T& c, ...) {
    { c.configure(speed, event_handler, setup_handler, user) } -> Result<void>;
    { c.connect() }                              -> Result<void>;
    { c.disconnect() }                           -> Result<void>;
    { c.enable_endpoint(ep, type, mps) }         -> Result<void>;
    { c.disable_endpoint(ep) }                   -> Result<void>;
    { c.write(ep, span) }                        -> Result<void>;
    { c.read(ep, span) }                         -> Result<size_t>;
    { c.stall(ep) }                              -> Result<void>;
    { c.unstall(ep) }                            -> Result<void>;
    { c.set_address(addr) }                      -> Result<void>;
    { c.service() }                              -> Result<void>;
};
```

Backends call the application's `EventHandler` to surface bus events
(`Reset`, `Suspend`, `Resume`, `SetupReceived`, `TransferComplete`, `Error`)
and `SetupHandler` for EP0 setup packets. The application returns
`Ok(span)` to provide the IN payload or `Err(NotSupported)` to STALL.

---

## Compile-time descriptor builder

```cpp
namespace desc = alloy::hal::usb::descriptor;

constexpr auto kDevice = desc::device_descriptor({
    .vendor_id = 0xCAFE,
    .product_id = 0xBEEF,
    .num_configurations = 1,
});

constexpr auto kConfig = desc::config_descriptor({
    .num_interfaces = 1,
    .configuration_value = 1,
    .max_power_2ma_units = 50,   // 100 mA
});

constexpr auto kInterface = desc::interface_descriptor({
    .interface_number = 0,
    .num_endpoints = 1,
    .interface_class = 0xFF,     // vendor-specific
});

constexpr auto kEndpoint = desc::endpoint_descriptor({
    .address = EndpointAddress::make(1, EndpointDirection::In),
    .type = EndpointType::Bulk,
    .max_packet_size = 64,
});

// Compose into a flat array.
constexpr auto kConfigTree = desc::concat_descriptors(
    kConfig, kInterface, kEndpoint);
desc::patch_config_total_length(/*non-const ref*/);  // patches wTotalLength
```

The `concat_descriptors` template uses fold expressions to compute the total
size at compile time; the result is a `constexpr std::array<std::byte, N>`
that can be stored as a `static constexpr` member of a class driver.

---

## Class drivers

### CDC-ACM (virtual COM port)

`CdcAcm<Controller>` exposes `write(span)` / `read(span)` returning
`core::Result` — the same shape as the UART HAL. Drop-in for any
`ByteStream`-shaped consumer (Modbus RTU over USB, log sink, JSON-RPC).

Class requests handled automatically:
- `SET_LINE_CODING` (0x20) — host sends baud / parity / data-bits. Stored on
  the driver; the application can read via `cdc.line_coding()` to mirror to a
  real UART if needed.
- `GET_LINE_CODING` (0x21) — driver returns the cached state.
- `SET_CONTROL_LINE_STATE` (0x22) — DTR / RTS bits. `cdc.is_connected()`
  returns the DTR state.
- `SEND_BREAK` (0x23) — recorded in `cdc.last_break_duration_ms()`.

The descriptor builder `cdc_acm::build_configuration_descriptor()` produces
the full 67-byte CDC-ACM configuration tree:
config(9) + iface(9) + cdc-header(5) + call-mgmt(5) + acm(4) + union(5) +
notify-ep(7) + iface(9) + in-ep(7) + out-ep(7).

### DFU (Device Firmware Upgrade)

Two modes per USB DFU 1.1:

`DfuRuntime<Controller>` — exposed alongside the device's normal interface
(CDC, HID, vendor). Handles `DFU_DETACH` (the application registers a
callback that typically jumps to a bootloader).

`DfuDownload<Controller, FlashBackend>` — bootloader-side downloader. Receives
`DFU_DNLOAD` blocks and commits each one to a `FlashBackend`. The W25Q SPI
driver and an STM32 internal-flash adapter both satisfy `FlashBackend` —
the downloader doesn't care which one.

```cpp
struct StmInternalFlash { /* erase, write, read */ };
static_assert(alloy::drivers::usb::dfu::FlashBackend<StmInternalFlash>);

DfuDownload dl{controller, flash, /*base*/ 0x08010000, /*size*/ 0x00010000};
```

State machine transitions: `DfuIdle → DfuDnloadIdle → DfuManifestSync →
DfuManifestWaitReset`. The host uses `DFU_GETSTATUS` to poll progress.

### HID (Human Interface Device)

`HidDevice<Controller, InputReportSize, OutputReportSize=0>` sends fixed-size
input reports on an interrupt-IN endpoint. The application provides a
compile-time report descriptor (e.g. `kBootKeyboardReportDescriptor`).

Output reports (host → device, e.g. keyboard LEDs) are optional; register a
callback via `set_output_callback` and call `deliver_output_report` from the
controller's transfer-complete event for the OUT endpoint.

```cpp
HidDevice<StmUsbFs<USB>, 8u> kbd{
    controller,
    std::span<const std::byte>{kBootKeyboardReportDescriptor},
    EndpointAddress::make(1, EndpointDirection::In),
};

std::array<std::byte, 8> report{};
report[2] = std::byte{0x04};  // 'a' keycode
kbd.send_report(report);
```

---

## ISR wiring

Real backends drive the application via two callbacks installed at
`configure()`:

- **Event handler**: surfaces bus state changes. Keep it short — most
  backends call from interrupt context. Suspend/resume are the typical
  application observation points.
- **Setup handler**: returns the IN payload for an EP0 control transfer. The
  application routes by request type:
  - `setup.is_standard()` → typically the controller backend handles
    GET_DESCRIPTOR / SET_ADDRESS / SET_CONFIGURATION; the application only
    sees what the backend doesn't recognise.
  - `setup.is_class()` → forward to the class driver's
    `handle_class_request(setup, data)`.
  - `setup.is_vendor()` → application-defined.

---

## Backend status

| Backend | Status | Tracking |
|---------|--------|----------|
| `MockUsbController` | host-side, satisfies concept | shipped |
| `StmUsbFs<USB>` (G0/L0/F0/WB) | scaffolding deferred | `add-stm32-usb-fs-backend` |
| `StmUsbOtgFs<USB_OTG_FS>` (F4/L4/H7) | scaffolding deferred | `add-stm32-otg-fs-backend` |
| `Nrf52Usbd` | scaffolding deferred | `add-nrf52-usbd-backend` |
| `Same70Uotghs` | scaffolding deferred | `add-same70-uotghs-backend` |

Class-driver code can be written today against the `UsbDeviceController`
concept; the `static_assert(UsbDeviceController<MockUsbController>)` in the
mock backend pins the concept's signature.

---

## See also

- [`tests/compile_tests/test_hal_usb_controller_concept.cpp`](../tests/compile_tests/test_hal_usb_controller_concept.cpp) — concept compile check
- [`tests/compile_tests/test_hal_usb_descriptor.cpp`](../tests/compile_tests/test_hal_usb_descriptor.cpp) — descriptor builder size pinning
- [`tests/compile_tests/test_usb_class_drivers.cpp`](../tests/compile_tests/test_usb_class_drivers.cpp) — CDC-ACM / DFU / HID instantiation
- USB 2.0 specification §9 (device framework) — wire-layout reference
- USB CDC 1.2, USB DFU 1.1, USB HID 1.11 — class specifications
