# Tasks: Extend DAC Coverage

Phases ordered. Phases 1-3 host-testable. Phase 4 needs the
3-board hardware matrix.

## 1. Per-channel + trigger + prescaler + software reset

- [x] 1.1 `enable_channel(std::uint8_t)`,
      `disable_channel(std::uint8_t)`,
      `channel_ready(std::uint8_t) -> bool`,
      `write_channel(std::uint8_t, std::uint32_t)` — uses
      `modify_indexed_channel_field` (new helper in runtime_ops.hpp
      that accounts for bit_stride_bits; pre-existing
      modify_indexed_field only handled stride_bytes).
- [x] 1.2 `enum class TriggerEdge { Disabled, Rising, Falling, Both }`.
      `set_hardware_trigger(std::uint8_t source, TriggerEdge edge)` —
      gated on `kHasHardwareTrigger`; uses `channel_traits::kTriggerEnableField`
      + `kTriggerSelectField`. No edge-select field in DB; Rising/Falling/Both
      all map to "trigger enabled".
- [x] 1.3 `set_prescaler(std::uint16_t)` — gated on `kPrescalerField.valid`
      (valid on SAME70 DACC; invalid on G071 DAC).
- [x] 1.4 `software_reset() -> Result<void, ErrorCode>` — gated on
      `kSoftwareResetField.valid` (valid on SAME70 DACC; invalid on G071).

## 2. Status / interrupts / kernel clock / IRQ vector

- [x] 2.1 `enum class InterruptKind { TransferComplete, Underrun,
      DmaComplete }`. `enable_interrupt` / `disable_interrupt` —
      NotSupported stubs (no individual IE field refs in DB).
- [x] 2.2 `underrun() -> bool`, `underrun_channel(std::uint8_t) -> bool`,
      `clear_underrun()` — NotSupported stubs (no underrun fields in DB).
- [x] 2.3 `set_kernel_clock_source(KernelClockSource)` — NotSupported stub
      (no kKernelClockSelectorField published in DB).
- [x] 2.4 `irq_numbers() -> std::span<const std::uint32_t>`.

## 3. Compile tests + async

- [x] 3.1 Extended `tests/compile_tests/test_dac_api.cpp`:
      G071 (DAC ch0) + SAME70 (DACC ch0) exercise all new methods.
      F401 has no DAC — no board-specific section.
- [x] 3.2 `async::dac::wait_for<Kind>(handle)` — new files
      `src/runtime/dac_event.hpp` + `src/runtime/async_dac.hpp`.
      Added to `src/async.hpp`.
- [x] 3.3 `tests/compile_tests/test_async_peripherals.cpp` extended
      with MockDacHandle + TransferComplete + Underrun wait_for calls.

## 4. Hardware spot-check + example

- [ ] 4.1 `examples/dac_probe_complete/`: targets `nucleo_g071rb`
      DAC1 channel 1, hardware trigger from TIM3 update at 1 kHz,
      DMA-driven sine-wave output.
- [ ] 4.2 Mirror on `same70_xplained` (DACC) and `nucleo_f401re`.
- [ ] 4.3 SAME70 / STM32G0 / STM32F4: scope-verify sine output
      < 0.5 % THD over 1 minute.
- [ ] 4.4 Update `docs/SUPPORT_MATRIX.md` `dac` row.

## 5. Documentation + follow-ups

- [ ] 5.1 `docs/DAC.md` — comprehensive guide.
- [ ] 5.2 Cross-link from `docs/ASYNC.md` and `docs/COOKBOOK.md`.
- [ ] 5.3 File alloy-codegen `add-dac-channel-typed-enum`.
- [ ] 5.4 File alloy `add-dac-trigger-source-typed-enum` (consumer
      side once codegen above lands).

## 10. Device-database follow-ups (deferred)

- [ ] 10.1 Publish individual IE field refs for DAC interrupts.
- [ ] 10.2 Publish underrun status/clear field refs.
- [ ] 10.3 Publish kKernelClockSelectorField for DAC.
- [ ] 10.4 Fix G071 kChannelDisablePattern — currently identical to
      kChannelEnablePattern (same CR register); disable should write
      0 to enable bit, not 1. Needs separate logic or DB fix.
