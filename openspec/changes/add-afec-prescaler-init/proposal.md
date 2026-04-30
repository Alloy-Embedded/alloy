# Add AFEC Prescaler Init to adc::configure()

## Why

After `add-peripheral-clock-gate-to-configure` wired the PMC clock enable into
`adc::handle::configure()`, SAME70 AFEC `read_sequence` still returned
`AdcConversionTimeout`. Root cause: `AFEC_MR.PRESCAL` defaults to 0 after reset,
which drives the AFEC internal clock at MCK/2. For the SAME70 Xplained running at
150 MHz MCK this gives AFEC_CLK = 75 MHz — nearly double the 40 MHz maximum.

The ADC semantic traits already publish `kAdcMaxClockHz = 40_000_000` for AFEC, and
the underlying field `field_afec0_mr_prescal` (bit 8, width 8 in AFEC_MR) exists in
the alloy-devices register-field database. The gap is:

1. `kPrescalerField` is not emitted into `AdcSemanticTraits<AFEC0/AFEC1>` by
   alloy-codegen (filed as `add-adc-prescaler-field` in alloy-codegen).
2. `hal::adc::Config` has no `peripheral_clock_hz` field, so the HAL has no way to
   know the MCK at call time.
3. `hal::adc::handle::configure()` never writes PRESCAL.

## What Changes

### `hal::adc::Config` — new field

```cpp
struct Config {
    bool enable_on_configure  = true;
    bool start_immediately    = false;
    std::uint32_t peripheral_clock_hz = 0u;  // NEW; 0 = skip prescaler init
};
```

When zero (the default) the prescaler write is skipped — backward compatible for
all devices that don't need it.

### `AdcSemanticTraits<AFEC0>` and `<AFEC1>` — new field

```cpp
static constexpr RuntimeFieldRef kPrescalerField =
    RuntimeFieldRef{FieldId::field_afec0_mr_prescal,
                    RuntimeRegisterRef{RegisterId::register_afec0_mr,
                                       0x4003C000u, 4u, true},
                    8u, 8u, true};
```

Also added to the base `AdcSemanticTraits` default struct as `kInvalidFieldRef` so
the `if constexpr (requires { … })` guard in the HAL compiles on every device.

The proper upstream fix (alloy-codegen) is tracked separately; the alloy-devices
patch here is a stopgap until alloy-codegen emits `kPrescalerField`.

### `hal::adc::handle::configure()` — prescaler init block

Inserted immediately after the clock-gate call, before `enable()`:

```cpp
if constexpr (requires { semantic_traits::kPrescalerField; } &&
              semantic_traits::kPrescalerField.valid &&
              semantic_traits::kAdcMaxClockHz > 0u) {
    if (config_.peripheral_clock_hz > 0u) {
        const auto divisor = 2u * semantic_traits::kAdcMaxClockHz;
        const auto prescal = (config_.peripheral_clock_hz + divisor - 1u) / divisor - 1u;
        if (const auto r = detail::runtime::modify_field(
                semantic_traits::kPrescalerField, prescal);
            r.is_err()) {
            return r;
        }
    }
}
```

Formula: `PRESCAL = ⌈MCK / (2 × kAdcMaxClockHz)⌉ − 1`, ensuring
`AFEC_CLK = MCK / (2 × (PRESCAL + 1)) ≤ kAdcMaxClockHz`.

For SAME70 @ 150 MHz MCK, 40 MHz max: PRESCAL = ⌈150/80⌉ − 1 = 2 − 1 = **1**
→ AFEC_CLK = 150 / 4 = **37.5 MHz** ✓

### SAME70 `boards/same70_xplained/board_analog.hpp`

```cpp
inline auto make_adc(alloy::hal::adc::Config config = {}) -> BoardAdc {
    config.peripheral_clock_hz = same70_xplained::ClockConfig::pclk_freq_hz;
    return BoardAdc{config};
}
```

## Impact

- Zero overhead on devices that don't publish `kPrescalerField` or pass
  `peripheral_clock_hz = 0` — the `if constexpr` blocks compile away entirely.
- AFEC_MR.PRESCAL is idempotent for a fixed MCK; calling `configure()` twice
  writes the same value.
- No ABI impact — all changes are in header-only inline paths.
- Other boards (`nucleo_g071rb`, `nucleo_f401re`): `peripheral_clock_hz` defaults to
  0; the prescaler block is skipped because STM32 ADC does not publish
  `kPrescalerField`.
