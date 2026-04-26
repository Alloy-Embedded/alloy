# public-hal-api Spec Delta: Timer Coverage Extension

## ADDED Requirements

### Requirement: Timer HAL SHALL Expose Counter Mode Setters Per Capability

The `alloy::hal::timer::handle<P>` MUST expose
`set_direction(CountDirection)` (gated on `kDirectionField.valid`),
`set_center_aligned(CenterAligned)` (gated on `kHasCenterAligned`,
with the typed enum covering `Disabled / Mode1 / Mode2 / Mode3`),
and `set_one_pulse(bool)` (gated on `kHasOnePulse`). Backends
without the capability MUST return `core::ErrorCode::NotSupported`.

#### Scenario: STM32 TIM3 supports up/down counting

- **WHEN** an application calls
  `tim.set_direction(CountDirection::Down)` on TIM3 of
  `nucleo_g071rb`
- **THEN** the call succeeds and the DIR bit is set

#### Scenario: Center-aligned on a backend without kHasCenterAligned is rejected

- **WHEN** an application calls
  `tim.set_center_aligned(CenterAligned::Mode1)` on a peripheral
  whose `kHasCenterAligned` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Timer HAL SHALL Expose Frequency-Aware Configuration

The HAL MUST expose `set_frequency(std::uint32_t hz)` resolving
PSC + ARR from the kernel clock; returns
`core::ErrorCode::InvalidArgument` if the realised rate falls
outside ±2 % of the requested rate. A sibling `frequency() ->
std::uint32_t` MUST return the realised rate. Direct
`set_prescaler(std::uint16_t)` and the existing `set_period(...)`
remain available for users who want raw register access.

#### Scenario: 1 kHz frequency on STM32G0 TIM3 succeeds

- **WHEN** an application calls
  `tim.set_frequency(1000u)` on TIM3 with the default 64 MHz
  kernel clock
- **THEN** PSC + ARR are programmed to a (PSC, ARR) pair that
  realises 1 kHz within ±2 %
- **AND** `tim.frequency()` returns a value within ±2 % of 1000

### Requirement: Timer HAL SHALL Expose Capture / Compare Channels Per Capability

The HAL MUST expose `enum class CaptureCompareMode { Frozen,
Active, Inactive, Toggle, ForceLow, ForceHigh, Pwm1, Pwm2 }` plus
the channel-addressed methods:

- `set_channel_mode(std::uint8_t channel, CaptureCompareMode)`
- `set_compare_value(std::uint8_t channel, std::uint32_t)`
- `read_capture_value(std::uint8_t channel) -> std::uint32_t`
- `enable_channel_output(std::uint8_t channel, bool)`
- `set_channel_output_polarity(std::uint8_t channel, Polarity)`

All gated on `kHasCompare || kHasCapture`. Out-of-range channel
ids MUST return `core::ErrorCode::InvalidArgument`.

#### Scenario: STM32G0 TIM3 channel 1 produces PWM at 50 % duty

- **WHEN** an application calls
  `tim.set_channel_mode(1, CaptureCompareMode::Pwm1)` then
  `tim.set_compare_value(1, ARR / 2)` on TIM3 of `nucleo_g071rb`
- **THEN** both calls succeed
- **AND** the channel outputs a 50 % duty waveform at the timer's
  configured frequency

#### Scenario: Channel id beyond kChannelCount is rejected

- **WHEN** an application calls
  `tim.set_channel_mode(5, CaptureCompareMode::Pwm1)` on TIM3
  (which has 4 channels)
- **THEN** the call returns `core::ErrorCode::InvalidArgument`

### Requirement: Timer HAL SHALL Expose Complementary Outputs / Dead-Time / Break Input / Encoder Per Capability

The HAL MUST expose, each gated independently on the corresponding
capability flag:

- Complementary outputs (`kHasComplementaryOutputs`):
  `enable_complementary_output(std::uint8_t, bool)`,
  `set_complementary_polarity(std::uint8_t, Polarity)`.
- Dead-time (`kHasDeadtime`):
  `set_dead_time(std::uint8_t cycles)`.
- Break input (`kHasBreakInput`): `enable_break_input(bool)`,
  `set_break_polarity(Polarity)`, `break_active() -> bool`,
  `clear_break_flag()`.
- Encoder (`kHasEncoder`):
  `enum class EncoderMode { Disabled, Channel1, Channel2,
  BothChannels }`,
  `set_encoder_mode(EncoderMode)`,
  `set_encoder_polarity(EncoderPolarity)`.

#### Scenario: Encoder mode on STM32 TIM3 counts quadrature edges

- **WHEN** an application calls
  `tim.set_encoder_mode(EncoderMode::BothChannels)` on TIM3 of
  `nucleo_g071rb`
- **THEN** the call succeeds and SMS is programmed to encoder
  mode 3
- **AND** `tim.get_count()` updates on every channel-1 / channel-2
  edge

#### Scenario: Encoder on a backend without kHasEncoder is rejected

- **WHEN** an application calls
  `tim.set_encoder_mode(EncoderMode::Channel1)` on a peripheral
  whose `kHasEncoder` is false (e.g. STM32 TIM6 / TIM7)
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Timer HAL SHALL Expose Typed Status / Interrupt Setters And IRQ Number List

The HAL MUST expose:

- `update_event() -> bool`, `clear_update_event()`.
- `channel_event(std::uint8_t channel) -> bool`,
  `clear_channel_event(std::uint8_t)`.
- `enum class InterruptKind { Update, ChannelCompare, Trigger,
  Break, Update_DMA, Channel_DMA, Trigger_DMA }`.
- `enable_interrupt(InterruptKind, std::uint8_t channel = 0)` /
  `disable_interrupt(InterruptKind, std::uint8_t channel = 0)` —
  per-kind gated on the corresponding control-side IE field.
- `irq_numbers() -> std::span<const std::uint32_t>` returning
  `kIrqNumbers`.

#### Scenario: Update interrupt is observable and clearable

- **WHEN** the timer rolls over while update events are enabled
- **THEN** `tim.update_event()` returns `true`
- **AND** `tim.clear_update_event()` returns `Ok` and the next
  call returns `false`

### Requirement: Async Timer Adapter SHALL Add wait_event(InterruptKind, channel)

The runtime `async::timer` namespace MUST expose
`wait_event<P>(handle, InterruptKind kind, std::uint8_t channel =
0) -> core::Result<operation<…>, core::ErrorCode>` so a coroutine
can `co_await` an Update / ChannelCompare / Break / Trigger event.
The default existing wrappers (`wait_period` / `delay`) are
unchanged.

#### Scenario: Coroutine wakes on channel-1 input capture

- **WHEN** a task awaits
  `async::timer::wait_event<TIM3>(tim, InterruptKind::ChannelCompare, 1)`
  with channel 1 configured for input capture
- **THEN** the task resumes when the channel-1 interrupt fires
  and the awaiter returns `Ok`
