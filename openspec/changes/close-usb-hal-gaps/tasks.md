# Tasks: Close USB HAL Gaps

Host-testable: phases 1–3. Phase 4 requires hardware with USB cable.

## 1. IR additions

- [ ] 1.1 Add EPnR register fields for STM32G0 USB FS IR:
      `ep0r` through `ep7r` (only EP0 required; EP1–EP7 optional),
      `ep_type_field`, `ep_stat_tx_field`, `ep_stat_rx_field`, `ep_addr_field`.
- [ ] 1.2 Add USB PMA constants to STM32G0 IR:
      `usb_pma_base_address`, `usb_pma_size`, `btable_field`.
- [ ] 1.3 Add suspend/resume/rwup fields:
      `suspend_field` (SUSPM in CNTR), `resume_field` (RESUME in CNTR),
      `rwup_field` (RWUPE in CNTR).
- [ ] 1.4 Add STM32F4 OTG mode field: `usb_mode_field` in USB_OTG_GCCFG.
- [ ] 1.5 Extend `alloy-cpp-emit` USB template to include all new fields.
      Regen STM32G0 + STM32F4; verify fields appear in `UsbSemanticTraits`.
- [ ] 1.6 Update `cmake/hal-contracts/usb.json`: move endpoint fields from
      `optional` to `required` for any device claiming USB FS support.

## 2. DPRAM allocator

- [ ] 2.1 Create `src/hal/usb/detail/dpram_allocator.hpp`:
      `DpramAllocator<Size>` with `alloc(uint16_t) -> Result<uint16_t, ErrorCode>`
      and `reset()`.
- [ ] 2.2 Add unit test `tests/compile_tests/test_dpram_allocator.cpp`:
      alloc 64 bytes for EP0 TX + 64 bytes EP0 RX; verify no overlap;
      test overflow returns `ResourceExhausted`.
- [ ] 2.3 Integrate `DpramAllocator` into `usb_handle` constructor:
      initialize BDT at offset 0; EP0 TX at 64, EP0 RX at 128.

## 3. HAL method additions

- [ ] 3.1 Add `EndpointConfig` struct + `EpType` / `EpDir` enums to
      `src/hal/usb/usb_endpoint.hpp`.
- [ ] 3.2 Implement `configure_endpoint(const EndpointConfig&)` in `usb_handle`:
      set EPnR `EP_TYPE`, `EP_ADDR`, `STAT_TX`, `STAT_RX`; configure BDT entry.
- [ ] 3.3 Implement `stall_endpoint` / `unstall_endpoint`: write `STAT_TX/RX = STALL/VALID`.
- [ ] 3.4 Implement `write_endpoint`: copy data to DPRAM TX buffer; set STAT_TX = VALID.
- [ ] 3.5 Implement `read_endpoint`: read DPRAM RX buffer; set STAT_RX = VALID.
- [ ] 3.6 Implement `set_mode(UsbMode)` for OTG targets.
      Guard with `if constexpr (kUsbModeField.valid)`.
- [ ] 3.7 Add compile test `tests/compile_tests/test_usb_api.cpp`:
      call `configure_endpoint`, `stall_endpoint`, `write_endpoint`, `read_endpoint`
      against a mock USB handle.

## 4. Hardware validation

- [ ] 4.1 nucleo_f401re (USB FS): configure EP0 control, enumerate as CDC-ACM device.
      Verify device appears in host OS (`lsusb` / Device Manager).
- [ ] 4.2 nucleo_f401re: write_endpoint EP1 IN; verify host receives data via
      CDC serial port.
- [ ] 4.3 nucleo_f401re: read_endpoint EP1 OUT; host sends data; verify received
      in firmware rx buffer.
- [ ] 4.4 nucleo_g071rb (USB FS): same enumeration test with smaller PMA (512B).

## 5. Documentation

- [ ] 5.1 Update `docs/USB_HAL.md`: document endpoint config, DPRAM allocator,
      EP0 control flow, integration with TinyUSB.
- [ ] 5.2 Add `examples/usb_cdc_acm/` for nucleo_f401re using TinyUSB backend.
