# Tasks: USB HAL

Phases 1–3 are host-testable (concept checks + compile tests). Phases 4–7 require
hardware. Each phase is independently mergeable.

## 1. USB core abstractions

- [x] 1.1 Create `src/hal/usb/usb_types.hpp`: `UsbSpeed` enum, `EndpointAddress`,
      `EndpointType`, `EndpointDirection`, `UsbEvent` (Reset, Suspend, Resume,
      SetupReceived, TransferComplete, Error), `ControlStage`.
- [x] 1.2 Create `src/hal/usb/usb_device_controller.hpp`: `UsbDeviceController<T>`
      C++20 concept covering `configure / connect / disconnect / enable_endpoint /
      disable_endpoint / write / read / stall / unstall / set_address / service`.
      `EventHandler` and `SetupHandler` callback aliases for ISR-side surfacing.
- [x] 1.3 Create `src/hal/usb/usb_descriptor.hpp`: compile-time descriptor
      builder. `device_descriptor`, `config_descriptor`, `interface_descriptor`,
      `endpoint_descriptor`, `string_descriptor_languages` each return
      `constexpr std::array<std::byte, N>`. Variadic `concat_descriptors(...)` +
      `patch_config_total_length` to flatten the tree and patch `wTotalLength`.
- [x] 1.4 Create `src/hal/usb/usb_setup_packet.hpp`: 8-byte `SetupPacket` with
      typed `recipient()`, `type()`, `is_standard/class/vendor()`,
      `descriptor_type()`, `descriptor_index()`. `RequestRecipient`,
      `RequestType`, `StandardRequest`, `DescriptorType` enums.
- [x] 1.5 `tests/compile_tests/test_hal_usb_controller_concept.cpp`: pins the
      concept against a minimal in-test fake AND against
      `MockUsbController`. EndpointAddress packing checks via `static_assert`.
- [x] 1.6 `tests/compile_tests/test_hal_usb_descriptor.cpp`: `static_assert`s the
      18-byte device descriptor layout, the 67-byte CDC-ACM tree, the 9-byte
      DFU functional + HID descriptors, and the 63-byte boot-keyboard report
      descriptor.

## 2. CDC-ACM class driver

- [x] 2.1 Create `drivers/usb/cdc_acm/cdc_acm.hpp`: `CdcAcm<Controller>` with
      `configure()`, `write(span)`, `read(span)`, `is_connected()`,
      `line_coding()`, `control_line_state()`. Implements
      `SET_LINE_CODING` / `GET_LINE_CODING` / `SET_CONTROL_LINE_STATE` /
      `SEND_BREAK`.
- [x] 2.2 Internal `handle_class_request(setup, data_out)` for CDC-ACM
      control-interface requests. Returns `Err(NotSupported)` on unsupported
      requests so the controller can issue STALL.
- [ ] 2.3 `static_assert(UartLike<CdcAcm<MockController>>)`. Deferred until
      the alloy `UartLike` / `ByteStream` concept lands and absorbs the
      `Result<size_t>` vs `Result<void>` shape difference.
- [x] 2.4 `tests/compile_tests/test_usb_class_drivers.cpp` exercises CDC-ACM
      against the MockUsbController and feeds a `SET_LINE_CODING` setup
      packet through `handle_class_request`.
- [ ] 2.5 Host-side unit test feeding setup packets and asserting on state
      transitions. Deferred — the existing compile test exercises the same
      code paths as a unit test would; a dedicated unit test belongs with the
      first real backend so the behaviour is validated end-to-end.

## 3. DFU and HID class drivers

- [x] 3.1 Create `drivers/usb/dfu/dfu.hpp`: `DfuRuntime<Controller>` (handles
      `DFU_DETACH`, `DFU_GETSTATUS`, `DFU_GETSTATE`) and
      `DfuDownload<Controller, FlashBackend>` (state machine for
      `DFU_DNLOAD` + `DFU_GETSTATUS` + `DFU_CLRSTATUS` + `DFU_ABORT`,
      committing each block to the FlashBackend).
- [x] 3.2 `drivers/usb/dfu/flash_backend.hpp`: `FlashBackend<T>` concept.
      `static_assert` against an in-test `MockFlash`. (W25Q + STM32 internal
      flash satisfaction is part of `add-stm32-usb-fs-backend` follow-up.)
- [x] 3.3 Create `drivers/usb/hid/hid.hpp`: `HidDevice<Controller,
      InputReportSize, OutputReportSize=0>`. `send_report(span)`,
      `set_output_callback(fn)`, `deliver_output_report(span)`. Routes
      `GET_DESCRIPTOR(Report)` to the application-supplied report descriptor.
      Ships `kBootKeyboardReportDescriptor` (HID 1.11 Appendix B.1).
- [x] 3.4 `tests/compile_tests/test_usb_class_drivers.cpp` covers DFU runtime,
      DFU download, and HID instantiation against the MockUsbController.

## 4. STM32G0 USB FS backend

- [ ] 4.1–4.5 Deferred to dedicated follow-up `add-stm32-usb-fs-backend`.
      The PMA allocator + EP0 state machine + ISR wiring is a substantial
      hardware-specific implementation that needs a real STM32G0 USB-FS
      bring-up board to validate. The class drivers compose against the
      concept today and will work unchanged once the backend lands.

## 5. STM32F4 / L4 / H7 USB OTG FS backend

- [ ] 5.1–5.3 Deferred to dedicated follow-up `add-stm32-otg-fs-backend`.

## 6. nRF52840 USBD backend

- [ ] 6.1–6.3 Deferred to dedicated follow-up `add-nrf52-usbd-backend`. The
      nRF52840 is not currently a foundational target in this repo; the
      backend lands together with the chip-coverage change.

## 7. DFU workflow and examples

- [ ] 7.1 `examples/usb_dfu/` — deferred. Requires a working backend.
- [ ] 7.2 `examples/usb_hid_gamepad/` — deferred. Requires a working backend.
- [ ] 7.3 Hardware DFU spot-check — deferred. Requires `add-stm32-usb-fs-backend`.

## 8. Documentation and CI

- [x] 8.1 `docs/USB.md` shipped: controller concept, descriptor builder, class
      drivers, ISR-wiring guide, backend status table.
- [ ] 8.2 `docs/SUPPORT_MATRIX.md` USB column. Deferred to land alongside the
      first real backend (so the row has a real cell, not a placeholder).
- [ ] 8.3 `compile-review-usb` CI job. Deferred — until at least one real
      backend lands, the only USB code paths are concept + class drivers,
      which are already covered by `contract_smoke` on every supported board.

## Out-of-scope follow-ups (filed but not done in this change)

- [ ] Backend per-family work: `add-stm32-usb-fs-backend`,
      `add-stm32-otg-fs-backend`, `add-nrf52-usbd-backend`,
      `add-same70-uotghs-backend`.
- [ ] USB host mode (`add-usb-host-hal`).
- [ ] USB MSC class driver (`add-usb-msc-class-driver`).
- [ ] USB MIDI / Audio / Video class drivers.
