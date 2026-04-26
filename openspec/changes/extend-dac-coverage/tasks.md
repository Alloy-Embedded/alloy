# Tasks: Extend DAC Coverage

Phases ordered. Phases 1-3 host-testable. Phase 4 needs the
3-board hardware matrix.

## 1. Per-channel + trigger + prescaler + software reset

- [ ] 1.1 `enable_channel(std::uint8_t)`,
      `disable_channel(std::uint8_t)`,
      `channel_ready(std::uint8_t) -> bool`,
      `write_channel(std::uint8_t, std::uint32_t)` — gated on
      `kChannelEnablePattern.valid`.
- [ ] 1.2 `enum class TriggerEdge { Disabled, Rising, Falling, Both }`.
      `set_hardware_trigger(std::uint8_t source, TriggerEdge edge)` —
      gated on `kHasHardwareTrigger`.
- [ ] 1.3 `set_prescaler(std::uint16_t)` — gated on
      `kPrescalerField.valid`.
- [ ] 1.4 `software_reset() -> Result<void, ErrorCode>` — gated on
      `kSoftwareResetField.valid`.

## 2. Status / interrupts / kernel clock / IRQ vector

- [ ] 2.1 `enum class InterruptKind { TransferComplete, Underrun,
      DmaComplete }`. `enable_interrupt` / `disable_interrupt` —
      per-kind gated.
- [ ] 2.2 `underrun() -> bool`, `underrun_channel(std::uint8_t) -> bool`,
      `clear_underrun()`.
- [ ] 2.3 `set_kernel_clock_source(KernelClockSource)` — gated on
      `kKernelClockSelectorField.valid`.
- [ ] 2.4 `irq_numbers() -> std::span<const std::uint32_t>`.

## 3. Compile tests + async

- [ ] 3.1 Extend `tests/compile_tests/test_dac_api.cpp` to instantiate
      every new method against `nucleo_g071rb` DAC1.
- [ ] 3.2 `async::dac::wait_for(InterruptKind)` runtime sibling.
- [ ] 3.3 Compile test extends `test_async_peripherals.cpp`.

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
