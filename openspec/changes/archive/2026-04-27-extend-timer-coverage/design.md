# Design: Extend Timer Coverage

## Context

The runtime exposes the bare-minimum Timer HAL. The descriptor
publishes 25+ field refs plus 9 capability flags. This document
captures the design for closing the gap.

## Goals

1. Match modm-class Timer completeness on every device the
   descriptor declares Timer for: capture / compare / encoder /
   center-aligned / one-pulse / break input / complementary
   outputs / dead-time.
2. Stay schema-agnostic: capability-flag + typed-field-ref absorbs
   STM32 TIMx vs SAM TC vs NXP GPT differences.
3. Compose with `complete-async-hal`'s
   `async::timer::wait_period / delay`.
4. Keep the existing API byte-compatible — additive only.

## Non-Goals

- Trigger chaining (TIMx → TIMx+1 via ITRx) HAL-level abstraction.
- DMA burst recipe (CCR1..CCR4 in one DMA request). Tracked as
  `add-timer-dma-burst`.
- ESP32 / RP2040 / AVR-DA timer feature parity.

## Key Decisions

### Decision 1: Capability gates via `if constexpr`

Same pattern as ADC / UART / SPI / I2C. Each capability
(`kHasCapture`, `kHasCompare`, `kHasEncoder`, etc.) gates the
corresponding method group. Backends without the capability return
`NotSupported`.

### Decision 2: Channels addressed via `std::uint8_t` for now

The descriptor publishes channel-bit patterns; a typed
`TimerChannelOf<P>::type` enum is the natural follow-up (mirror of
`AdcChannelOf<P>`). Until that codegen change lands, the HAL
accepts a raw `std::uint8_t channel` parameter and clamps against
the descriptor's published channel count, returning
`InvalidArgument` for out-of-range channels. Tracked as
`add-timer-channel-typed-enum`.

### Decision 3: `set_frequency(u32 hz)` resolves PSC + ARR with ±2 % validation

Timer frequency depends on prescaler + auto-reload. The HAL picks
a (PSC, ARR) pair that produces the requested rate within ±2 %
using the current kernel clock; returns `InvalidArgument`
otherwise.

For users who want fine control, `set_prescaler` + `set_period`
(existing) still work — the HAL doesn't fight you. `frequency()`
returns the realised rate.

### Decision 4: `CaptureCompareMode` enum is the cross-vendor superset

The 8 modes (`Frozen / Active / Inactive / Toggle / ForceLow /
ForceHigh / Pwm1 / Pwm2`) are the union of what STM32 / SAM / NXP
publish. Backends without a particular mode return `NotSupported`
when set; the descriptor's `kModeField` width gates the validity.

### Decision 5: `EncoderMode` typed enum captures the encoder ladder

`EncoderMode { Disabled, Channel1, Channel2, BothChannels }` maps
1:1 to the SMS field encoding in STM32 advanced timers (mode 1,
2, 3 = encoder; mode 0 = disabled). Backends without
`kHasEncoder` return `NotSupported`.

### Decision 6: `InterruptKind` enum is the cross-vendor superset

Same shape as UART / SPI / I2C. `Update / ChannelCompare /
Trigger / Break / Update_DMA / Channel_DMA / Trigger_DMA`. Each
gated on the corresponding control-side IE field. The
`enable_interrupt(InterruptKind, channel)` overload covers the
per-channel variants (`ChannelCompare` requires a channel id;
`Update` doesn't).

## Risks

- **Channel count differs across timers.** TIM2/3/4 on STM32 have
  4 channels; TIM6/7 have none. The HAL clamps against
  `kChannelCount` in the descriptor; out-of-range returns
  `InvalidArgument`.
- **Center-aligned mode interaction with one-pulse mode.** Mutually
  exclusive on STM32. The HAL doesn't enforce — last write wins.
  Documented in `docs/TIMER.md`.

## Migration

No source-code changes for existing users. New methods are
additive. `docs/TIMER.md` includes a "before / after" migration
table for users currently writing raw register hex to control
capture / compare modes.
