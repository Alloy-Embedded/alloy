# Tasks: Extend PWM Coverage

Phases ordered. Phases 1-4 are host-testable. Phase 5 needs the
3-board hardware matrix.

## 1. Mode + polarity + complementary

- [ ] 1.1 `enum class CenterAligned { Disabled, Mode1, Mode2,
      Mode3 }`. `set_center_aligned(CenterAligned)` — gated on
      `kSupportsCenterAligned || kHasCenterAligned`.
- [ ] 1.2 `enum class Polarity { Active, Inverted }`.
      `set_channel_polarity(std::uint8_t channel, Polarity)`.
- [ ] 1.3 Complementary outputs (gated on
      `kSupportsComplementary || kHasComplementaryOutputs`):
      `enable_complementary_output(std::uint8_t, bool)`,
      `set_complementary_polarity(std::uint8_t, Polarity)`.

## 2. Dead-time + fault + master output + sync

- [ ] 2.1 `set_dead_time(std::uint8_t rise_cycles,
      std::uint8_t fall_cycles)` — gated on `kSupportsDeadTime`.
      Combined-field backends average rise + fall; documented.
- [ ] 2.2 Fault input (`kSupportsFaultInput || kHasFaultInput`):
      `enable_fault_input(bool)`,
      `set_fault_polarity(Polarity)`,
      `fault_active() -> bool`,
      `clear_fault_flag() -> Result<void, ErrorCode>`.
- [ ] 2.3 `enable_master_output(bool)` — gated on the descriptor's
      `kMasterOutputEnableField.valid`.
- [ ] 2.4 `set_update_synchronized(bool)` — gated on
      `kSupportsSynchronizedUpdate || kHasSynchronizedUpdate`.

## 3. Carrier modulation + force-initialization

- [ ] 3.1 `set_carrier_modulation(bool)` — gated on
      `kSupportsCarrierModulation`.
- [ ] 3.2 `force_initialize() -> Result<void, ErrorCode>` — gated
      on `kSupportsForceInitialization`.

## 4. Status / interrupts / IRQ vector lookup / paired channels

- [ ] 4.1 `channel_event(std::uint8_t channel) -> bool`,
      `clear_channel_event(std::uint8_t)`.
- [ ] 4.2 `enum class InterruptKind { ChannelCompare, Update,
      Fault, Break, ChannelCompare_DMA, Update_DMA }`.
      `enable_interrupt(InterruptKind, std::uint8_t channel = 0)` /
      `disable_interrupt(...)`.
- [ ] 4.3 `irq_numbers() -> std::span<const std::uint32_t>`.
- [ ] 4.4 `static constexpr auto paired_channels(std::uint8_t channel)
      -> std::uint8_t` — returns the descriptor's
      `kPairedChannels` mapping.

## 5. Compile tests

- [ ] 5.1 Extend `tests/compile_tests/test_pwm_api.cpp` to
      instantiate every new method against `nucleo_g071rb` TIM3.
- [ ] 5.2 Add `nucleo_f401re`-targeted test exercising TIM1
      advanced features (complementary + dead-time + master-output
      + fault input).
- [ ] 5.3 Add `same70_xplained`-targeted test for SAM PWM with
      separate DTHI / DTLI fields.

## 6. Async integration

- [ ] 6.1 `async::pwm::wait_event(InterruptKind, std::uint8_t channel = 0)`
      — sibling of timer wrapper.
- [ ] 6.2 Compile test extends `test_async_peripherals.cpp`.

## 7. Example

- [ ] 7.1 `examples/pwm_probe_complete/`: targets `nucleo_f401re`
      TIM1 advanced timer. Configures 20 kHz, center-aligned mode 1,
      channels 1/2/3 with complementary CH1N/CH2N/CH3N, dead-time
      = 1 µs, fault input enabled (PA12), master output enabled.
      3-phase motor-drive demo.
- [ ] 7.2 Mirror configuration on `nucleo_g071rb` TIM3 (drop the
      complementary + advanced-timer-only paths).
- [ ] 7.3 Mirror on `same70_xplained` SAM PWM (SAM-specific
      DTHI / DTLI exercised).

## 8. Hardware spot-check (3-board matrix)

- [ ] 8.1 SAME70 Xplained: PWM 20 kHz, dead-time accuracy ±5 %
      over 60 s.
- [ ] 8.2 STM32G0 Nucleo: same.
- [ ] 8.3 STM32F4 Nucleo: 3-phase complementary + dead-time
      validated on oscilloscope.
- [ ] 8.4 Update `docs/SUPPORT_MATRIX.md` `pwm` row.

## 9. Documentation

- [ ] 9.1 `docs/PWM.md` — model, frequency / duty recipes,
      complementary + dead-time, fault input, synchronized update,
      carrier modulation, status flags, async wiring, modm
      migration table, motor-control reference example.
- [ ] 9.2 Cross-link from `docs/ASYNC.md` and `docs/COOKBOOK.md`.

## 10. Out-of-scope follow-ups (filed)

- [ ] 10.1 alloy-codegen `add-pwm-channel-typed-enum`.
- [ ] 10.2 alloy `add-motor-control-driver` —
      `MotorDriver<PWM, Encoder>` 3-phase abstraction.
- [ ] 10.3 alloy `add-pwm-fault-routing` — typed fault-source +
      filter setup once descriptor publishes the fault matrix.
