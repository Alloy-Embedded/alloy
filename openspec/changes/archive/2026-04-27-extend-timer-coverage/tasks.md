# Tasks: Extend Timer Coverage

Phases ordered. Phases 1-4 are host-testable. Phase 5 needs the
3-board hardware matrix.

## 1. Counter mode + frequency / prescaler / period

- [x] 1.1 `enum class CountDirection { Up, Down }`.
      `set_direction(CountDirection)` — gated on
      `kDirectionField.valid`.
- [x] 1.2 `enum class CenterAligned { Disabled, Mode1, Mode2, Mode3 }`.
      `set_center_aligned(CenterAligned)` — gated on `kHasCenterAligned`.
- [x] 1.3 `set_one_pulse(bool)` — gated on `kHasOnePulse`.
- [x] 1.4 `set_prescaler(std::uint16_t)` — direct PSC write.
- [x] 1.5 `set_frequency(std::uint32_t hz)` — resolves PSC + ARR,
      ±2 % validation; `frequency() -> std::uint32_t` accessor.
- [x] 1.6 `set_auto_reload_preload(bool)` —
      `kAutoReloadPreloadField.valid`.
- [x] 1.7 `set_period_preload(bool)` — `kPreloadField.valid` (applies
      OCxPE to all channels simultaneously).

## 2. Capture / compare channels

- [x] 2.1 `enum class CaptureCompareMode { Frozen, Active,
      Inactive, Toggle, ForceLow, ForceHigh, Pwm1, Pwm2 }`.
      `set_channel_mode(std::uint8_t channel, CaptureCompareMode)`
      — gated on `kHasCompare || kHasCapture`. Out-of-range
      channel id returns `InvalidArgument`.
- [x] 2.2 `set_compare_value(std::uint8_t, std::uint32_t)`,
      `read_capture_value(std::uint8_t) -> std::uint32_t`.
- [x] 2.3 `enable_channel_output(std::uint8_t, bool)`,
      `set_channel_output_polarity(std::uint8_t, Polarity)`.
- [x] 2.4 Complementary outputs (`kHasComplementaryOutputs`):
      `enable_complementary_output(std::uint8_t, bool)`,
      `set_complementary_polarity(std::uint8_t, Polarity)`.

## 3. Dead-time / break input / encoder

- [x] 3.1 `set_dead_time(std::uint8_t cycles)` — gated on
      `kHasDeadtime`. kDeadtimeField not yet in database → always
      returns NotSupported (field-ref follow-up tracked as 10.x).
- [x] 3.2 Break input (`kHasBreakInput`):
      `enable_break_input(bool)`,
      `set_break_polarity(Polarity)`,
      `break_active() -> bool`,
      `clear_break_flag()`.
      kBreakEnableField not yet in database → all return NotSupported.
- [x] 3.3 Encoder (`kHasEncoder`):
      `enum class EncoderMode { Disabled, Channel1, Channel2,
      BothChannels }`,
      `set_encoder_mode(EncoderMode)`,
      `set_encoder_polarity(Polarity)`.
      STM32: kEncoderModeField (SMS). Microchip TC: kEncoderEnableField
      (QDEN) + kEncoderPhaseEdgeField (EDGPHA).

## 4. Status / interrupts / IRQ vector lookup

- [x] 4.1 `update_event() -> bool`, `clear_update_event()`.
- [x] 4.2 `channel_event(std::uint8_t channel) -> bool`,
      `clear_channel_event(std::uint8_t)`.
- [x] 4.3 `enum class InterruptKind { Update, ChannelCompare,
      Trigger, Break, Update_DMA, Channel_DMA, Trigger_DMA }`.
      `enable_interrupt(InterruptKind, std::uint8_t channel = 0)` /
      `disable_interrupt(...)`.
- [x] 4.4 `irq_numbers() -> std::span<const std::uint32_t>`.

## 5. Compile tests

- [x] 5.1 Extend `tests/compile_tests/test_timer_api.cpp` to
      instantiate every new method against `nucleo_g071rb` TIM1
      (TIM3 not yet in g071rb device database).
- [x] 5.2 Add `nucleo_f401re`-targeted test exercising
      complementary outputs + dead-time on advanced TIM1.
- [x] 5.3 Add `same70_xplained`-targeted test for SAM TC0 encoder
      mode (kEncoderEnableField path).

## 6. Async integration

- [ ] 6.1 `async::timer::wait_event(InterruptKind, std::uint8_t channel = 0)`
      — sibling of existing `async::timer::wait_period / delay`.
- [ ] 6.2 Compile test extends `test_async_peripherals.cpp`.

## 7. Example

- [ ] 7.1 `examples/timer_probe_complete/`: targets
      `nucleo_g071rb` TIM1, configures 1 kHz update, channel 1 in
      Pwm1 50 %, channel 2 in input-capture, async
      `wait_event(Update)` task.
- [ ] 7.2 Mirror on `nucleo_f401re` TIM1 (advanced timer +
      complementary).
- [ ] 7.3 Mirror on `same70_xplained` TC0 (SAM TC encoder).

## 8. Hardware spot-check (3-board matrix)

- [ ] 8.1 SAME70 Xplained: PWM 1 kHz, capture-edge accuracy
      < 0.1 % over 60 s.
- [ ] 8.2 STM32G0 Nucleo: same.
- [ ] 8.3 STM32F4 Nucleo: same + complementary + dead-time.
- [ ] 8.4 Update `docs/SUPPORT_MATRIX.md` `timer` row.

## 9. Documentation

- [ ] 9.1 `docs/TIMER.md` — model, prescaler / period math,
      capture / compare / encoder recipes, complementary +
      dead-time, break input, status flags, async wiring,
      modm migration table, per-vendor capability matrix.
- [ ] 9.2 Cross-link from `docs/ASYNC.md` and `docs/COOKBOOK.md`.

## 10. Out-of-scope follow-ups (filed)

- [ ] 10.1 alloy-codegen `add-timer-channel-typed-enum`.
- [ ] 10.2 alloy `add-timer-dma-burst` —
      CCR1..CCR4-streamed-in-one-DMA-request recipe.
- [ ] 10.3 alloy `add-timer-trigger-chain` — TIMx-feeds-TIMx+1 via
      ITRx HAL surface.
- [ ] 10.4 Publish kBreakEnableField + kDeadtimeField in device
      database (alloy-devices) so set_dead_time / enable_break_input
      gain real register writes instead of NotSupported.
