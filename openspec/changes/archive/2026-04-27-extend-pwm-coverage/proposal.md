# Extend PWM Coverage To Match Published Descriptor Surface

## Why

The latest alloy-devices publication ships
`PwmSemanticTraits<P>` with capability flags
(`kSupportsBrake`, `kSupportsCarrierModulation`,
`kSupportsCenterAligned`, `kSupportsComplementary`,
`kSupportsDeadTime`, `kSupportsDma`, `kSupportsFaultInput`,
`kSupportsForceInitialization`, `kHasCenterAligned`,
`kHasComplementaryOutputs`, `kHasDeadtime`, `kHasFaultInput`,
`kHasSynchronizedUpdate`) plus dedicated dead-time rise/fall
fields, paired-channel mapping, sync register, master-output-enable,
clock prescaler, and full status / interrupt surface.

The runtime currently consumes ~30% of that surface: the HAL
exposes `configure / start / stop / set_frequency / set_period /
set_duty_cycle / configure_dma`. Complementary outputs, dead-time,
fault input, center-aligned mode, carrier modulation, synchronized
update, force-initialization — none of these are reachable today.

modm covers PWM end-to-end with full motor-control feature set:
3-phase complementary outputs with dead-time, fault input
shutdown, synchronized update across channels, brake / break
behaviour. Alloy already publishes the data needed; this change
is plumbing.

Scope: lift every published `PwmSemanticTraits<P>` field and
capability flag into typed HAL methods, capability-gated via
`if constexpr`. Wire the existing DMA path through the
`complete-async-hal` pattern. Ship `pwm_probe_complete` example.
Document in `docs/PWM.md`.

## What Changes

### `src/hal/pwm.hpp` — extended HAL surface (additive only)

- **Existing methods unchanged.** `configure / start / stop /
  set_frequency / set_period / set_duty_cycle / configure_dma`
  remain.
- **Center-aligned mode (gated on `kSupportsCenterAligned`)**
  - `enum class CenterAligned { Disabled, Mode1, Mode2, Mode3 }`.
  - `set_center_aligned(CenterAligned)`.
- **Polarity per channel**
  - `enum class Polarity { Active, Inverted }`.
  - `set_channel_polarity(std::uint8_t channel, Polarity)`.
- **Complementary outputs (gated on `kSupportsComplementary` /
  `kHasComplementaryOutputs`)**
  - `enable_complementary_output(std::uint8_t channel, bool)`,
    `set_complementary_polarity(std::uint8_t channel, Polarity)`.
- **Dead-time (gated on `kSupportsDeadTime`)**
  - `set_dead_time(std::uint8_t rise_cycles, std::uint8_t fall_cycles)`
    — descriptor publishes both rise and fall fields. Backends
    with only one (combined dead-time) take the average and ignore
    the second; documented.
- **Fault / break input (gated on `kSupportsFaultInput` /
  `kSupportsBrake` / `kHasFaultInput`)**
  - `enable_fault_input(bool)`, `set_fault_polarity(Polarity)`,
    `fault_active() -> bool`, `clear_fault_flag()`.
- **Master output enable (gated)**
  - `enable_master_output(bool)` — drives MOE bit on STM32
    advanced timers. Backends without it return `NotSupported`.
- **Synchronized update (gated on `kSupportsSynchronizedUpdate` /
  `kHasSynchronizedUpdate`)**
  - `set_update_synchronized(bool)` — when true, channel updates
    take effect at the next update event rather than immediately.
- **Carrier modulation (gated on `kSupportsCarrierModulation`)**
  - `set_carrier_modulation(bool)` — for IR-remote-style PWM
    on top of a sub-carrier.
- **Force initialization (gated on `kSupportsForceInitialization`)**
  - `force_initialize()` — kicks the timer to its initial state
    without waiting for an update event.
- **Per-channel status / interrupts (typed)**
  - `channel_event(std::uint8_t channel) -> bool`,
    `clear_channel_event(std::uint8_t)`.
  - `enum class InterruptKind { ChannelCompare, Update, Fault,
    Break, ChannelCompare_DMA, Update_DMA }`.
  - `enable_interrupt(InterruptKind, std::uint8_t channel = 0)` /
    `disable_interrupt(...)`.
- **Paired channels accessor**
  - `static constexpr auto paired_channels(std::uint8_t channel)
    -> std::uint8_t` — returns the complementary channel id from
    the descriptor's `kPairedChannels` array (e.g. CH1N is the
    pair of CH1).
- **NVIC vector lookup**
  - `irq_numbers() -> std::span<const std::uint32_t>`.
- **Async sibling**
  - `async::pwm::wait_event(InterruptKind, std::uint8_t channel = 0)`
    — extends `complete-async-hal`'s pattern.

### `examples/pwm_probe_complete/`

Targets `nucleo_f401re` TIM1 (advanced timer with full motor-
control surface). Configures:
- 20 kHz frequency
- Center-aligned mode 1
- Channels 1, 2, 3 with complementary outputs CH1N, CH2N, CH3N
- Dead-time = 1 µs (rise + fall)
- Fault input enabled (PA12)
- Master output enabled

3-phase motor-drive demo. Mirrors a modm `Pwm` recipe with each
lever.

### Docs

`docs/PWM.md` — comprehensive guide: model, frequency / duty
recipes, complementary + dead-time, fault input, synchronized
update, carrier modulation, status flags, async wiring, modm
migration table, motor-control reference.

## What Does NOT Change

- Existing PWM API (`configure / start / stop / set_frequency /
  set_period / set_duty_cycle / configure_dma`) is unchanged. New
  methods are additive.
- PWM tier in `docs/SUPPORT_MATRIX.md` stays `foundational` —
  hardware spot-checks for new levers land per board with the
  existing 3-board matrix.
- Descriptor-side fields are NOT changed.

## Out of Scope (Follow-Up Changes)

- **`add-pwm-channel-typed-enum` (alloy-codegen).** Mirror of the
  ADC/UART/SPI/I2C/Timer typed-enum follow-ups so
  `set_channel_polarity` takes a typed channel id at compile time.
- **3-phase motor-control reference driver.** A higher-level
  `MotorDriver<PWM, Encoder>` abstraction layered on top of this
  HAL surface. Lands as `add-motor-control-driver`.
- **Hardware spot-checks.** Land in
  `validate-pwm-coverage-on-3-boards`.
- **ESP32 / RP2040 / AVR-DA PWM coverage.** Gated on the in-flight
  alloy-codegen work reaching those families.

## Alternatives Considered

**Generate the entire PWM HAL from the schema id.** Same posture
as ADC/UART/SPI/I2C/Timer. Rejected for the same reason:
schema-aware code crosses the runtime/device boundary.

**Lock dead-time to a single setter.** Rejected — the descriptor
publishes rise + fall fields independently (SAM PWM has separate
DTHI / DTLI fields). The HAL accepts both and falls back when
backends only have one combined field.
