# Tasks: Close Timer/PWM HAL Gaps

Host-testable: phases 1–3. Phase 4 requires hardware (scope/analyzer to verify timing).

## 1. IR additions

- [ ] 1.1 Add BDTR register fields for STM32 TIM1 + TIM8 IR:
      `kDtgField`, `kBkeField`, `kBkpField`, `kAoeField`, `kMoeField`.
- [ ] 1.2 Add CCER complementary output fields:
      `kCc1neField`, `kCc1npField`, `kCc2neField`, `kCc3neField`.
- [ ] 1.3 Add SMCR slave mode fields:
      `kSmsField` (3-bit, encoder modes), `kEceField`.
- [ ] 1.4 Add CR1 fields: `kCmsField` (2-bit), `kOpmField`.
- [ ] 1.5 Add DMA burst registers: `kDbaField`, `kDblField`, `kDmarReg`.
- [ ] 1.6 Regen STM32G0 (TIM1) + STM32F4 (TIM1, TIM8); verify new fields valid.
- [ ] 1.7 Update `cmake/hal-contracts/timer_pwm.json`:
      move dead-time + break fields to `optional` (advanced timers only);
      add encoder + center-aligned as `optional`.

## 2. Dead-time and complementary outputs

- [ ] 2.1 Add `DeadTimeConfig` struct + `configure_dead_time(cfg)` to `timer_handle.hpp`.
      Guard with `if constexpr (kDtgField.valid && kMoeField.valid)`.
- [ ] 2.2 Implement `ns_to_dtg(ns, clk_hz)` as `static constexpr`.
      Handles all four DTG encoding ranges (datasheet Table 83 for STM32).
- [ ] 2.3 Implement `enable_main_output()` / `disable_main_output()`.
- [ ] 2.4 Add compile test: call `configure_dead_time`, `enable_main_output`.
      Call `ns_to_dtg(100, 64_000_000)` and verify result at compile time.

## 3. Break input

- [ ] 3.1 Add `BreakConfig` struct + `configure_break(cfg)` to `timer_handle.hpp`.
      Guard with `if constexpr (kBkeField.valid)`.
- [ ] 3.2 Add compile test: call `configure_break({.enabled=true, .active_high=false})`.

## 4. Encoder mode

- [ ] 4.1 Add `EncoderMode` enum + `configure_encoder(mode)` to `timer_handle.hpp`.
      Guard with `if constexpr (kSmsField.valid)`.
- [ ] 4.2 Add `read_encoder_count()` that reads `kCntReg`.
- [ ] 4.3 Add compile test: call `configure_encoder(EncoderMode::both)`,
      `read_encoder_count()`.

## 5. Center-aligned and one-pulse mode

- [ ] 5.1 Add `CenterAlignedMode` enum + `set_center_aligned(mode)`.
      Guard with `if constexpr (kCmsField.valid)`.
- [ ] 5.2 Add `enable_one_pulse_mode()`.
      Guard with `if constexpr (kOpmField.valid)`.
- [ ] 5.3 Add compile tests.

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

- [ ] 7.1 Update `docs/TIMER_HAL.md`: document dead-time, break, encoder, center-aligned.
      Include `ns_to_dtg` usage example and all four DTG encoding ranges.
- [ ] 7.2 Add `examples/bldc_pwm/` for nucleo_g071rb: 3-phase complementary PWM
      with dead-time using TIM1 (CH1/CH1N, CH2/CH2N, CH3/CH3N).
- [ ] 7.3 Add `examples/encoder_speed/` for nucleo_g071rb: quadrature encoder
      with speed calculation from timer overflow count.
