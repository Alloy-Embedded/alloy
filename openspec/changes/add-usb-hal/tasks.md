# Tasks: USB HAL

Phases 1–3 are host-testable (concept checks + compile tests). Phases 4–7 require
hardware. Each phase is independently mergeable.

## 1. USB core abstractions

- [ ] 1.1 Create `src/hal/usb/usb_types.hpp`: `UsbSpeed` enum, `EndpointAddress`,
      `EndpointType` enum, `UsbEvent` enum (Reset, Suspend, Resume, SetupReceived,
      TransferComplete, Error).
- [ ] 1.2 Create `src/hal/usb/usb_device_controller.hpp`: `UsbDeviceController<T>`
      C++20 concept. `static_assert` with a fake mock controller type.
- [ ] 1.3 Create `src/hal/usb/usb_descriptor.hpp`: compile-time descriptor builder.
      `UsbDeviceDescriptor<VID, PID, ...>`, `UsbConfigDescriptor<...>`,
      `UsbInterfaceDescriptor<...>`, `UsbEndpointDescriptor<...>`. All evaluate to
      `constexpr std::array<std::byte, N>` via fold expressions. Static-assert total
      size <= 255 bytes.
- [ ] 1.4 Create `src/hal/usb/usb_setup_packet.hpp`: `SetupPacket` struct with
      `bmRequestType`, `bRequest`, `wValue`, `wIndex`, `wLength` typed accessors.
      `is_standard()`, `is_class()`, `is_vendor()`, `recipient()` helpers.
- [ ] 1.5 `tests/compile_tests/test_hal_usb_controller_concept.cpp`: concept compile
      check with fake controller.
- [ ] 1.6 `tests/compile_tests/test_hal_usb_descriptor.cpp`: constexpr descriptor build
      size check — `static_assert(cdc_acm_descriptor.size() == expected_N)`.

## 2. CDC-ACM class driver

- [ ] 2.1 Create `drivers/usb/cdc_acm/cdc_acm.hpp`: `CdcAcm<Controller>` with
      `configure()`, `write(span)`, `read(span)`, `is_connected()`.
      Implements standard CDC-ACM `SET_LINE_CODING` / `GET_LINE_CODING` setup handling.
- [ ] 2.2 Internal: `class_request_handler()` for setup packets targeting the CDC
      control interface. Returns `Stall` on unsupported requests.
- [ ] 2.3 `static_assert(UartLike<CdcAcm<MockController>>)` — CDC-ACM satisfies the
      same write/read contract as UART so it can be used as a `ByteStream`.
- [ ] 2.4 `tests/compile_tests/test_usb_cdc_acm.cpp`: compile check with mock controller.
- [ ] 2.5 `tests/unit/test_usb_cdc_acm.cpp`: host-side unit test — feed setup packets
      into `class_request_handler`, verify correct responses and state transitions.

## 3. DFU and HID class drivers

- [ ] 3.1 Create `drivers/usb/dfu/dfu.hpp`: `DfuRuntime<Controller>` (reports DFU
      Detach on request), and `DfuDownload<Controller, FlashBackend>` (receives firmware
      blocks and writes them via `FlashBackend` concept).
- [ ] 3.2 `FlashBackend` concept in `drivers/usb/dfu/flash_backend.hpp`:
      `erase(addr, size)`, `write(addr, data)`, `read(addr, data)`. W25Q driver
      satisfies it — add `static_assert` to `drivers/memory/w25q/w25q.hpp`.
- [ ] 3.3 Create `drivers/usb/hid/hid.hpp`: `HidDevice<Controller, N>` where `N` is
      the report descriptor size. `send_report(span)` and
      `set_recv_callback(fn)` for output reports.
- [ ] 3.4 `tests/compile_tests/test_usb_dfu.cpp`, `test_usb_hid.cpp`: compile checks.

## 4. STM32G0 USB FS backend

- [ ] 4.1 Create `src/hal/usb/backends/stm32_usb_fs.hpp`: `StmUsbFs<PeripheralId>`.
      PMA (Packet Memory Area) allocator — static ring of 64-byte blocks.
      EP0 control transfer handler (setup, data, status stages).
      IN/OUT bulk endpoint support.
- [ ] 4.2 ISR: `USB_IRQHandler` signals endpoint events via `UsbEvent` callback.
      Application registers event handler in `configure()`.
- [ ] 4.3 `static_assert(UsbDeviceController<StmUsbFs<USB_0>>)`.
- [ ] 4.4 `examples/usb_cdc_acm/` targeting `nucleo_g071rb`. USB FS on PA11/PA12.
      Verify `/dev/ttyACM0` appears on Linux/macOS host after enumeration.
- [ ] 4.5 Hardware spot-check: `usb_cdc_acm` echo server on `nucleo_g071rb`.
      Send 1000 bytes, verify round-trip with no lost bytes.

## 5. STM32F4 / L4 / H7 USB OTG FS backend

- [ ] 5.1 Create `src/hal/usb/backends/stm32_usb_otg_fs.hpp`: `StmUsbOtgFs<PeripheralId>`.
      GRXFSIZ / GNPTXFSIZ / DIEPTXFx FIFO allocation. GINTSTS ISR dispatch.
      IN/OUT endpoint management. VBUS sensing configurable.
- [ ] 5.2 `static_assert(UsbDeviceController<StmUsbOtgFs<USB_OTG_FS_0>>)`.
- [ ] 5.3 Compile-review preset for `nucleo_f401re` (STM32F4) with USB OTG FS.
      Hardware validation deferred until STM32F4 USB bring-up board is available.

## 6. nRF52840 USBD backend

- [ ] 6.1 Create `src/hal/usb/backends/nrf52_usbd.hpp`: `Nrf52Usbd`.
      POWER/USBD event handling (USBDETECTED → connect, USBREMOVED → disconnect).
      Apply ERRATA-166 (USB ISO IN transfers) and ERRATA-187 workarounds.
      EasyDMA mode for IN endpoints.
- [ ] 6.2 `static_assert(UsbDeviceController<Nrf52Usbd>)`.
- [ ] 6.3 Compile-review preset for `nrf52840_dk`. Hardware validation deferred
      to the chip-coverage hardware validation phase.

## 7. DFU workflow and examples

- [ ] 7.1 `examples/usb_dfu/` targeting `nucleo_g071rb`.
      Boot in DFU Runtime mode. `dfu-util -l` lists the device.
      `dfu-util -D blink.bin` uploads a new firmware image to W25Q flash (SPI-connected).
      Verify flash contents after upload.
- [ ] 7.2 `examples/usb_hid_gamepad/` targeting `nucleo_g071rb`.
      Two ADC channels → axes, two GPIO buttons. Host OS enumerates as generic gamepad.
      Verify in `jstest` / `evtest` on Linux.
- [ ] 7.3 Hardware spot-check: DFU download of 64 KB image, verify CRC32 of written
      flash matches image CRC32.

## 8. Documentation and CI

- [ ] 8.1 Create `docs/USB.md`: controller concept, descriptor builder, class drivers,
      ISR wiring guide for each backend, DFU workflow, HID report descriptor guide.
- [ ] 8.2 `docs/SUPPORT_MATRIX.md`: add USB column. STM32G0 USB FS = `hardware`.
      STM32F4 OTG FS, nRF52840 = `compile-review`.
- [ ] 8.3 Add `compile-review-usb` CI job: builds all USB examples for all
      supported presets. Verifies no size regressions.
