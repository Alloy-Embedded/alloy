# Tasks: Add AFEC Prescaler Init

## 1. alloy-devices patch (stopgap until alloy-codegen emits kPrescalerField)

- [x] 1.1 `alloy-devices/microchip/same70/.../driver_semantics/adc.hpp` —
      add `kPrescalerField`, `kResetField`, `kModeRegisterOneField`,
      `kAnalogControlField`, `kAnalogControlValue` defaults to base struct.
- [x] 1.2 Same file — add all five fields to `AdcSemanticTraits<AFEC0>`:
      `kPrescalerField` (AFEC_MR.PRESCAL bit 8:8), `kResetField`
      (AFEC_CR.SWRST bit 0), `kModeRegisterOneField` (AFEC_MR.ONE bit 23),
      `kAnalogControlField` (AFEC_ACR.IBCTL bits 8:2), `kAnalogControlValue=3`.
- [x] 1.3 Same — same five fields for `AdcSemanticTraits<AFEC1>`.
- [x] 1.4 Applied to both atsame70q21b and atsame70n21b device variants.

## 2. HAL

- [x] 2.1 `src/hal/adc.hpp` — add `peripheral_clock_hz` field to
      `hal::adc::Config` (default 0 = skip).
- [x] 2.2 `src/hal/adc.hpp` — add prescaler init block to `configure()`.
- [x] 2.3 `src/hal/adc.hpp` — add SWRST block (`kResetField`) before
      prescaler — required by AFEC before reconfiguring AFEC_MR.
- [x] 2.4 `src/hal/adc.hpp` — add `kModeRegisterOneField` block: sets
      AFEC_MR.ONE (bit 23), which the SAME70 datasheet requires to always be 1.
- [x] 2.5 `src/hal/adc.hpp` — add `kAnalogControlField`/`kAnalogControlValue`
      block: sets AFEC_ACR.IBCTL=3, required for the comparator bias current
      (reset value 0 disables the comparator; conversions never complete).

## 3. Board wiring

- [x] 3.1 `boards/same70_xplained/board_analog.hpp` — `make_adc()` sets
      `config.peripheral_clock_hz = same70_xplained::ClockConfig::pclk_freq_hz`
      before returning the handle.

## 4. Hardware validation

- [ ] 4.1 SAME70 Xplained: rebuild and run `analog_probe_complete`.
      `read_sequence(4 samples)` must return `Ok` and print four
      non-zero sample values.
- [ ] 4.2 STM32G071RB: rebuild and run `analog_probe_complete`. No
      regression — `read_sequence` outcome unchanged (compile-time gate
      skips prescaler block entirely).
- [ ] 4.3 STM32F401RE: same as 4.2.

## 5. alloy-codegen follow-up (out of scope for this change)

- [ ] 5.1 File `add-adc-prescaler-field` in alloy-codegen: emit
      `kPrescalerField` as a `RuntimeFieldRef` in `AdcSemanticTraits`
      for every ADC peripheral that has a clock-prescaler register field
      (AFEC, and any future ADC variant that needs it).
      Once alloy-codegen emits it, the alloy-devices manual patch in
      tasks 1.x can be removed.
