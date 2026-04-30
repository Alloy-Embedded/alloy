# Tasks: Close Timer/PWM HAL Gaps

Host-testable: phases 1–3. Phase 4 requires hardware (scope/analyzer to verify timing).

## 1. IR additions

- [x] 1.1 Add BDTR semantic fields for STM32 TIM1 + TIM8 via bootstrap patches:
      `kDtgField`, `kBkeField`, `kBkpField`, `kAoeField`, `kMoeField`, `kBreakFlagField`.
      (alloy-devices: stm32g0-timer.yaml + stm32f4-timer.yaml patches created)
- [x] 1.2 Add CCER complementary output fields via per-channel semantics:
      `comp_enable_field`, `comp_polarity_field` per channel in `kChannels` list.
- [x] 1.3 SMCR slave mode + CR1 fields via `kEncoderModeField`, `kCenterAlignedField`,
      `kOnePulseField` semantics (already in existing G0 generated timer.hpp;
      added to new timer.hpp.j2 template and G0/F4 bootstrap patches).
- [ ] 1.4 DMA burst registers: `kDbaField`, `kDblField`, `kDmarReg` — deferred to next spec.
- [ ] 1.5 Regen STM32G0 (TIM1) + STM32F4 (TIM1, TIM8); verify new fields from
      BDTR + CCER patches are in FieldId enum and timer.hpp generated correctly.
      (Depends on full SVD regen pipeline run — alloy-devices task 1.2 / 2.2)
- [x] 1.6 Update `hal-contracts/timer.json`:
      `kDtgField`, `kBkeField`, `kBkpField`, `kAoeField`, `kMoeField`, `kBreakFlagField`
      moved to optional_fields. Channel fields added to channel_optional_fields.

## 2. Dead-time and complementary outputs

- [x] 2.1 Add `DeadTimeConfig` struct + `configure_dead_time(cfg)` to `timer.hpp`.
      Guarded by `if constexpr (!kDtgField.valid || !kMoeField.valid)`.
- [x] 2.2 Implement `ns_to_dtg(ns, clk_hz)` as `static constexpr`.
      All four DTG encoding ranges implemented (0b0xxxxxxx / 10xxxxxx / 110xxxxx / 111xxxxx).
- [x] 2.3 Implement `enable_main_output()` / `disable_main_output()`.
- [x] 2.4 Compile tests: `configure_dead_time`, `enable_main_output`,
      `ns_to_dtg` static_asserts at 100ns/64MHz, 1984ns/64MHz, 4000ns/64MHz.

## 3. Break input

- [x] 3.1 Add `BreakConfig` struct + `configure_break(cfg)` to `timer.hpp`.
      Guards `kBkeField`, `kBkpField`, `kAoeField` individually.
- [x] 3.2 Fix `enable_break_input`, `set_break_polarity`, `break_active`,
      `clear_break_flag` to use `kBkeField`, `kBkpField`, `kBreakFlagField`.
- [x] 3.3 Compile test: `configure_break({.enabled=true, .active_high=false})`.

## 4. Encoder mode

- [x] 4.1 `set_encoder_mode` already works via `kEncoderModeField` (existing code).
- [x] 4.2 `get_count()` already reads `kCounterRegister` (existing code).
- [x] 4.3 Compile tests already cover these via existing `exercise_timer_extended`.

## 5. Center-aligned and one-pulse mode

- [x] 5.1 `set_center_aligned` works via `kCenterAlignedField` (existing code).
- [x] 5.2 `set_one_pulse` works via `kOnePulseField` (existing code).
- [x] 5.3 Compile tests covered in existing `exercise_timer_extended`.

## 6. Hardware validation

- [ ] 6.1 nucleo_g071rb + oscilloscope: configure TIM1 with 100ns dead-time,
      complementary output on CH1+CH1N. Verify dead-band on scope.
- [ ] 6.2 nucleo_g071rb: pull BKIN low; verify MOE cleared (outputs disabled);
      call `enable_main_output()`; verify outputs resume.
- [ ] 6.3 nucleo_g071rb + rotary encoder: configure TIM2 encoder mode (both);
      rotate encoder; verify `read_encoder_count()` increments/decrements correctly.
- [ ] 6.4 nucleo_g071rb: center-aligned PWM mode 3; verify on scope
      that switching frequency doubles vs edge-aligned at same ARR.

## 7. Documentation

- [x] 7.1 Create `docs/TIMER_HAL.md`: dead-time, break, encoder, center-aligned,
      ns_to_dtg usage + 3-phase BLDC example, all four DTG encoding ranges,
      vendor extension points section.
- [ ] 7.2 Add `examples/bldc_pwm/` for nucleo_g071rb: 3-phase complementary PWM
      with dead-time using TIM1 (CH1/CH1N, CH2/CH2N, CH3/CH3N).
      (Pending hardware validation — task 6.1 / 6.2)
- [ ] 7.3 Add `examples/encoder_speed/` for nucleo_g071rb: quadrature encoder
      with speed calculation from timer overflow count.
      (Pending hardware validation — task 6.3)
