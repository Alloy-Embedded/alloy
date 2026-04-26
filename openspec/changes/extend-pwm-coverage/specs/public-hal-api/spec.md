# public-hal-api Spec Delta: PWM Coverage Extension

## ADDED Requirements

### Requirement: PWM HAL SHALL Expose Center-Aligned Mode And Per-Channel Polarity

The `alloy::hal::pwm::handle<P>` MUST expose
`enum class CenterAligned { Disabled, Mode1, Mode2, Mode3 }` plus
`set_center_aligned(CenterAligned)` (gated on
`kSupportsCenterAligned || kHasCenterAligned`) and
`enum class Polarity { Active, Inverted }` plus
`set_channel_polarity(std::uint8_t channel, Polarity)`. Backends
without the capability MUST return `core::ErrorCode::NotSupported`.

#### Scenario: STM32 TIM1 supports center-aligned mode 1

- **WHEN** an application calls
  `pwm.set_center_aligned(CenterAligned::Mode1)` on TIM1 of
  `nucleo_f401re`
- **THEN** the call succeeds and CMS is programmed to mode 1

### Requirement: PWM HAL SHALL Expose Complementary Outputs / Dead-Time / Fault Input / Master Output / Synchronized Update Per Capability

The HAL MUST expose, each gated independently:

- Complementary outputs (`kSupportsComplementary ||
  kHasComplementaryOutputs`):
  `enable_complementary_output(std::uint8_t, bool)`,
  `set_complementary_polarity(std::uint8_t, Polarity)`.
- Dead-time (`kSupportsDeadTime`):
  `set_dead_time(std::uint8_t rise_cycles,
  std::uint8_t fall_cycles)`. Combined-field backends MUST average
  rise + fall and document the precision loss.
- Fault input (`kSupportsFaultInput || kHasFaultInput`):
  `enable_fault_input(bool)`, `set_fault_polarity(Polarity)`,
  `fault_active() -> bool`,
  `clear_fault_flag() -> Result<void, ErrorCode>`.
- Master output: `enable_master_output(bool)` — gated on
  `kMasterOutputEnableField.valid`.
- Synchronized update (`kSupportsSynchronizedUpdate ||
  kHasSynchronizedUpdate`):
  `set_update_synchronized(bool)`.

#### Scenario: STM32 TIM1 drives 3-phase complementary with dead-time

- **WHEN** an application configures TIM1 of `nucleo_f401re` for
  3-phase PWM by calling
  `pwm.enable_complementary_output(1, true)`,
  `pwm.enable_complementary_output(2, true)`,
  `pwm.enable_complementary_output(3, true)`,
  `pwm.set_dead_time(80, 80)` (1 µs at 80 MHz),
  `pwm.enable_master_output(true)`
- **THEN** all calls succeed
- **AND** TIM1 outputs CH1/CH1N, CH2/CH2N, CH3/CH3N with the
  configured dead-time

#### Scenario: Dead-time on a backend without kSupportsDeadTime is rejected

- **WHEN** an application calls `pwm.set_dead_time(80, 80)` on a
  peripheral whose `kSupportsDeadTime` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: PWM HAL SHALL Expose Carrier Modulation And Force-Initialization Per Capability

The HAL MUST expose `set_carrier_modulation(bool)` (gated on
`kSupportsCarrierModulation`) and `force_initialize() ->
Result<void, ErrorCode>` (gated on `kSupportsForceInitialization`).
Backends without the capability MUST return
`core::ErrorCode::NotSupported`.

#### Scenario: Carrier modulation on a non-supporting backend is rejected

- **WHEN** an application calls
  `pwm.set_carrier_modulation(true)` on a peripheral whose
  `kSupportsCarrierModulation` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: PWM HAL SHALL Expose Typed Status / Interrupt Setters And Paired Channels

The HAL MUST expose:

- `channel_event(std::uint8_t channel) -> bool`,
  `clear_channel_event(std::uint8_t)`.
- `enum class InterruptKind { ChannelCompare, Update, Fault,
  Break, ChannelCompare_DMA, Update_DMA }`.
- `enable_interrupt(InterruptKind, std::uint8_t channel = 0)` /
  `disable_interrupt(InterruptKind, std::uint8_t channel = 0)` —
  per-kind gated on the corresponding control-side IE field.
- `static constexpr auto paired_channels(std::uint8_t channel) ->
  std::uint8_t` — returns the descriptor's `kPairedChannels`
  mapping for the channel's complementary partner.
- `irq_numbers() -> std::span<const std::uint32_t>` returning
  `kIrqNumbers`.

#### Scenario: Channel 1 reports its complementary pair as channel 1's CH1N

- **WHEN** an application reads
  `decltype(pwm)::paired_channels(1)` on STM32 TIM1
- **THEN** the value matches the descriptor's `kPairedChannels[1]`
  entry (the encoded id for CH1N)

#### Scenario: Fault interrupt is gated on fault input support

- **WHEN** an application calls
  `pwm.enable_interrupt(InterruptKind::Fault)` on a peripheral
  whose `kSupportsFaultInput` and `kHasFaultInput` are both false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Async PWM Adapter SHALL Add wait_event(InterruptKind, channel)

The runtime `async::pwm` namespace MUST expose
`wait_event<P>(handle, InterruptKind kind, std::uint8_t channel = 0)
-> core::Result<operation<…>, core::ErrorCode>` so a coroutine can
`co_await` a Fault, Update, or ChannelCompare event without
polling. The default existing wrappers / blocking PWM API are
unchanged.

#### Scenario: Coroutine wakes on fault-input event

- **WHEN** a task awaits
  `async::pwm::wait_event<TIM1>(pwm, InterruptKind::Fault)` while
  the timer is running and BKIN goes active
- **THEN** the task resumes when the fault interrupt fires and
  the awaiter returns `Ok`
