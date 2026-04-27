# Tasks: Extend PWM Coverage

Phases ordered. Phases 1-4 are host-testable. Phase 5 needs the
3-board hardware matrix.

## 1. Mode + polarity + complementary

- [x] 1.1 `enum class CenterAligned { Disabled, Mode1, Mode2,
      Mode3 }`. `set_center_aligned(CenterAligned)` — gated on
      `kHasCenterAligned && kCenterAlignedField.valid`.
- [x] 1.2 `enum class Polarity { Active, Inverted }`.
      `set_channel_polarity(std::uint8_t channel, Polarity)`.
- [x] 1.3 Complementary outputs (gated on
      `kHasComplementaryOutputs || kSupportsComplementaryOutputs`):
      `enable_complementary_output(bool)`,
      `set_complementary_polarity(Polarity)`.

## 2. Dead-time + fault + master output + sync

- [x] 2.1 `set_dead_time(std::uint8_t rise_cycles,
      std::uint8_t fall_cycles)` — gated on `kHasDeadtime ||
      kSupportsDeadtime` + `kDeadtimeRiseField.valid`. Writes
      rise and fall separately when they map to different register
      locations (SAME70 DTHI/DTLI); collapses to single write
      when both fields alias the same bits (STM32 DTG).
- [x] 2.2 Fault input (`kHasFaultInput`):
      `enable_fault_input(bool)`,
      `set_fault_polarity(Polarity)`,
      `fault_active() -> bool`,
      `clear_fault_flag() -> Result<void, ErrorCode>`.
      kFaultEnableField / kFaultFlagField not yet published → all
      return NotSupported (follow-up tracked as 10.x).
- [x] 2.3 `enable_master_output(bool)` — gated on the descriptor's
      `kMasterOutputEnableField.valid`.
- [x] 2.4 `set_update_synchronized(bool)` — gated on
      `kHasSynchronizedUpdate && kLoadField.valid`.

## 3. Carrier modulation + force-initialization

- [x] 3.1 `set_carrier_modulation(bool)` — not yet published in
      device database → NotSupported.
- [x] 3.2 `force_initialize() -> Result<void, ErrorCode>` — not
      yet published → NotSupported.

## 4. Status / interrupts / IRQ vector lookup / paired channels

- [x] 4.1 `channel_event() -> bool`, `clear_channel_event()`.
- [x] 4.2 `enum class InterruptKind { ChannelCompare, Update,
      Fault, Break, ChannelCompare_DMA, Update_DMA }`.
      `enable_interrupt(InterruptKind)` /
      `disable_interrupt(InterruptKind)`.
- [x] 4.3 `irq_numbers() -> std::span<const std::uint32_t>`.
      kIrqNumbers not yet in PwmSemanticTraits → returns empty
      span.
- [x] 4.4 `paired_channel() -> std::uint8_t` — returns Channel
      itself; kPairedChannels index map not yet in
      PwmChannelSemanticTraits (follow-up tracked as 10.x).

## 5. Compile tests

- [x] 5.1 Extend `tests/compile_tests/test_pwm_api.cpp` against
      `nucleo_g071rb` TIM1 ch0.
- [x] 5.2 Add `nucleo_f401re`-targeted test (TIM1 advanced timer —
      complementary + dead-time + master-output).
- [x] 5.3 Add `same70_xplained`-targeted test (PWM0 ch0 —
      separate kDeadtimeRiseField / kDeadtimeFallField path).

## 6. Async integration

- [ ] 6.1 `async::pwm::wait_event(InterruptKind, std::uint8_t channel = 0)`
      — sibling of timer wrapper.
- [ ] 6.2 Compile test extends `test_async_peripherals.cpp`.

## 7. Example

- [ ] 7.1 `examples/pwm_probe_complete/`: targets `nucleo_f401re`
      TIM1 advanced timer.
- [ ] 7.2 Mirror on `nucleo_g071rb` TIM1.
- [ ] 7.3 Mirror on `same70_xplained` PWM0.

## 8. Hardware spot-check (3-board matrix)

- [ ] 8.1 SAME70 Xplained: PWM 20 kHz, dead-time accuracy ±5 %.
- [ ] 8.2 STM32G0 Nucleo: same.
- [ ] 8.3 STM32F4 Nucleo: complementary + dead-time.
- [ ] 8.4 Update `docs/SUPPORT_MATRIX.md` `pwm` row.

## 9. Documentation

- [ ] 9.1 `docs/PWM.md`.
- [ ] 9.2 Cross-links from `docs/ASYNC.md` and `docs/COOKBOOK.md`.

## 10. Out-of-scope follow-ups (filed)

- [ ] 10.1 alloy-codegen `add-pwm-channel-typed-enum`.
- [ ] 10.2 alloy `add-motor-control-driver`.
- [ ] 10.3 alloy `add-pwm-fault-routing`.
- [ ] 10.4 Publish kFaultEnableField + kFaultFlagField in device
      database so fault methods gain real register writes.
- [ ] 10.5 Publish kIrqNumbers in PwmSemanticTraits.
- [ ] 10.6 Publish kPairedChannels as index map in
      PwmChannelSemanticTraits (motor-drive paired-output routing).
