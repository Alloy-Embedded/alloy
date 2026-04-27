# Extend Timer Coverage To Match Published Descriptor Surface

## Why

The latest alloy-devices publication ships
`TimerSemanticTraits<P>` covering every CR1 / CR2 / SMCR / DIER /
SR / EGR / CCMR / CCER / CNT / PSC / ARR / CCR field across ST /
Microchip / NXP, plus capability flags (`kHasCapture`,
`kHasCompare`, `kHasCenterAligned`, `kHasEncoder`,
`kHasComplementaryOutputs`, `kHasDeadtime`, `kHasOnePulse`,
`kHasPwm`, `kHasBreakInput`) and Tier 2/3/4 metadata
(`kIrqNumbers`, `kSupportsCapture`, `kSupportsCompare`,
`kSupportsComplementaryOutput`, `kSupportsDmaBurst`).

The runtime currently consumes ~10% of that surface: the HAL
exposes `configure / start / stop / set_period / get_count /
is_running`. Capture / compare / encoder / center-aligned /
one-pulse / break input / complementary outputs / dead-time —
none of these are reachable today.

modm covers Timer end-to-end: every channel addressable, every
capability exposed, type-system enforces "this peripheral supports
encoder mode" at compile time. Alloy already publishes the data
needed; this change is plumbing.

Scope: lift every published `TimerSemanticTraits<P>` field and
capability flag into typed HAL methods, capability-gated via
`if constexpr`. Wire the IRQ levers into
`async::timer::wait_period / delay` from `complete-async-hal`. Ship
`timer_probe_complete` example. Document in `docs/TIMER.md`.

## What Changes

### `src/hal/timer.hpp` — extended HAL surface (additive only)

- **Counter mode**
  - `enum class CountDirection { Up, Down }`.
    `set_direction(CountDirection)` — gated on `kDirectionField.valid`.
  - `enum class CenterAligned { Disabled, Mode1, Mode2, Mode3 }`.
    `set_center_aligned(CenterAligned)` — gated on `kHasCenterAligned`.
  - `set_one_pulse(bool)` — gated on `kHasOnePulse`.
- **Prescaler / period (raw + frequency)**
  - `set_prescaler(std::uint16_t)` — direct register write.
  - `set_frequency(std::uint32_t hz)` — resolves PSC + ARR from
    the kernel clock; returns `InvalidArgument` if rate cannot be
    realised within ±2 %.
  - `frequency() -> std::uint32_t` — returns the realised frequency.
  - `set_auto_reload_preload(bool)` — gated on
    `kAutoReloadPreloadField.valid`.
  - `set_period_preload(bool)` — gated on `kPreloadField.valid`.
- **Capture / compare channels (gated on `kHasCompare` / `kHasCapture`)**
  - `enum class CaptureCompareMode { Frozen, Active, Inactive,
    Toggle, ForceLow, ForceHigh, Pwm1, Pwm2 }`.
  - `set_channel_mode(std::uint8_t channel, CaptureCompareMode)`.
  - `set_compare_value(std::uint8_t channel, std::uint32_t)`.
  - `read_capture_value(std::uint8_t channel) -> std::uint32_t`.
  - `enable_channel_output(std::uint8_t channel, bool)` — gated on
    `kOutputEnableField.valid`.
  - `set_channel_output_polarity(std::uint8_t channel,
    Polarity)` — `Active` is high; `Inverted` is low.
- **Complementary outputs (gated on `kHasComplementaryOutputs`)**
  - `enable_complementary_output(std::uint8_t channel, bool)`.
  - `set_complementary_polarity(std::uint8_t channel, Polarity)`.
- **Dead-time (gated on `kHasDeadtime`)**
  - `set_dead_time(std::uint8_t cycles)`.
- **Break input (gated on `kHasBreakInput`)**
  - `enable_break_input(bool)`,
    `set_break_polarity(Polarity)`,
    `break_active() -> bool`,
    `clear_break_flag()`.
- **Encoder mode (gated on `kHasEncoder`)**
  - `enum class EncoderMode { Disabled, Channel1, Channel2, BothChannels }`.
  - `set_encoder_mode(EncoderMode)`,
    `set_encoder_polarity(EncoderPolarity)`.
- **Status / interrupts (typed)**
  - `update_event() -> bool`, `clear_update_event()`.
  - `channel_event(std::uint8_t channel) -> bool`,
    `clear_channel_event(std::uint8_t)`.
  - `enum class InterruptKind { Update, ChannelCompare, Trigger,
    Break, Update_DMA, Channel_DMA, Trigger_DMA }`.
  - `enable_interrupt(InterruptKind, std::uint8_t channel = 0)` /
    `disable_interrupt(...)`.
- **NVIC vector lookup**
  - `irq_numbers() -> std::span<const std::uint32_t>` — same shape
    as UART / SPI / I2C.
- **Async sibling**
  - `async::timer::wait_event(InterruptKind, std::uint8_t channel = 0)` —
    extends `complete-async-hal`'s `async::timer::wait_period /
    delay`.

### `examples/timer_probe_complete/`

Targets `nucleo_g071rb` TIM3. Configures:
- 1 kHz update rate
- Channel 1 in Pwm1 mode at 50 % duty
- Channel 2 in input-capture mode (rising edge timestamp)
- Update interrupt async `wait_event(Update)`

Mirrors a modm `Timer` recipe with each lever.

### Docs

`docs/TIMER.md` — comprehensive guide: model, prescaler / period
math, capture / compare / encoder recipes, complementary +
dead-time, break input, status flags, async wiring, modm
migration table.

## What Does NOT Change

- Existing Timer API (`configure / start / stop / set_period /
  get_count / is_running`) is unchanged. New methods are additive.
- Timer tier in `docs/SUPPORT_MATRIX.md` stays `foundational` —
  hardware spot-checks for new levers land per board with the
  existing 3-board matrix.
- Descriptor-side fields are NOT changed.

## Out of Scope (Follow-Up Changes)

- **`add-timer-channel-typed-enum` (alloy-codegen).** Mirror of the
  ADC/UART/SPI/I2C typed-enum follow-ups: emits
  `TimerChannelOf<P>::type` per peripheral so `set_channel_mode`
  takes a typed channel id at compile time, not a raw `u8`.
- **DMA burst** (`kSupportsDmaBurst`). Descriptor publishes the
  flag; full DMA-burst recipe (CCR1..CCR4 streamed in one DMA
  request) ships as a separate `add-timer-dma-burst` change.
- **Slave / master mode trigger chaining.** TIMx feeds TIMx+1 via
  ITRx. Tracked as `add-timer-trigger-chain` once we have a real
  use case (encoder + counter pair).
- **Hardware spot-checks.** Land in
  `validate-timer-coverage-on-3-boards`.
- **ESP32 / RP2040 / AVR-DA timer coverage.** Gated on the
  in-flight kernel-clock-traits + irq-vector-traits work in
  alloy-codegen reaching those families.
