# Tasks: Extend Timer Coverage

Phases ordered. Phases 1-4 are host-testable. Phase 5 needs the
3-board hardware matrix.

## 1. Counter mode + frequency / prescaler / period

- [ ] 1.1 `enum class CountDirection { Up, Down }`.
      `set_direction(CountDirection)` — gated on
      `kDirectionField.valid`.
- [ ] 1.2 `enum class CenterAligned { Disabled, Mode1, Mode2, Mode3 }`.
      `set_center_aligned(CenterAligned)` — gated on `kHasCenterAligned`.
- [ ] 1.3 `set_one_pulse(bool)` — gated on `kHasOnePulse`.
- [ ] 1.4 `set_prescaler(std::uint16_t)` — direct PSC write.
- [ ] 1.5 `set_frequency(std::uint32_t hz)` — resolves PSC + ARR,
      ±2 % validation; `frequency() -> std::uint32_t` accessor.
- [ ] 1.6 `set_auto_reload_preload(bool)` —
      `kAutoReloadPreloadField.valid`.
- [ ] 1.7 `set_period_preload(bool)` — `kPreloadField.valid`.

## 2. Capture / compare channels

- [ ] 2.1 `enum class CaptureCompareMode { Frozen, Active,
      Inactive, Toggle, ForceLow, ForceHigh, Pwm1, Pwm2 }`.
      `set_channel_mode(std::uint8_t channel, CaptureCompareMode)`
      — gated on `kHasCompare || kHasCapture`. Out-of-range
      channel id returns `InvalidArgument`.
- [ ] 2.2 `set_compare_value(std::uint8_t, std::uint32_t)`,
      `read_capture_value(std::uint8_t) -> std::uint32_t`.
- [ ] 2.3 `enable_channel_output(std::uint8_t, bool)`,
      `set_channel_output_polarity(std::uint8_t, Polarity)`.
- [ ] 2.4 Complementary outputs (`kHasComplementaryOutputs`):
      `enable_complementary_output(std::uint8_t, bool)`,
      `set_complementary_polarity(std::uint8_t, Polarity)`.

## 3. Dead-time / break input / encoder

- [ ] 3.1 `set_dead_time(std::uint8_t cycles)` — gated on
      `kHasDeadtime`.
- [ ] 3.2 Break input (`kHasBreakInput`):
      `enable_break_input(bool)`,
      `set_break_polarity(Polarity)`,
      `break_active() -> bool`,
      `clear_break_flag()`.
- [ ] 3.3 Encoder (`kHasEncoder`):
      `enum class EncoderMode { Disabled, Channel1, Channel2,
      BothChannels }`,
      `set_encoder_mode(EncoderMode)`,
      `set_encoder_polarity(EncoderPolarity)`.

## 4. Status / interrupts / IRQ vector lookup

- [ ] 4.1 `update_event() -> bool`, `clear_update_event()`.
- [ ] 4.2 `channel_event(std::uint8_t channel) -> bool`,
      `clear_channel_event(std::uint8_t)`.
- [ ] 4.3 `enum class InterruptKind { Update, ChannelCompare,
      Trigger, Break, Update_DMA, Channel_DMA, Trigger_DMA }`.
      `enable_interrupt(InterruptKind, std::uint8_t channel = 0)` /
      `disable_interrupt(...)`.
- [ ] 4.4 `irq_numbers() -> std::span<const std::uint32_t>`.

## 5. Compile tests

- [ ] 5.1 Extend `tests/compile_tests/test_timer_api.cpp` to
      instantiate every new method against `nucleo_g071rb` TIM3.
- [ ] 5.2 Add `nucleo_f401re`-targeted test exercising
      complementary outputs + dead-time on advanced TIM1 / TIM8.
- [ ] 5.3 Add `same70_xplained`-targeted test for SAM TC encoder
      mode.

## 6. Async integration

- [ ] 6.1 `async::timer::wait_event(InterruptKind, std::uint8_t channel = 0)`
      — sibling of existing `async::timer::wait_period / delay`.
- [ ] 6.2 Compile test extends `test_async_peripherals.cpp`.

## 7. Example

- [ ] 7.1 `examples/timer_probe_complete/`: targets
      `nucleo_g071rb` TIM3, configures 1 kHz update, channel 1 in
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
