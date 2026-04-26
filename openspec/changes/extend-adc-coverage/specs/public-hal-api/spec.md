# public-hal-api Spec Delta: ADC Coverage Extension

## ADDED Requirements

### Requirement: ADC HAL SHALL Expose Resolution And Alignment When The Descriptor Publishes Them

The `alloy::hal::adc::handle<Peripheral>` SHALL expose
`set_resolution(Resolution)` and `set_alignment(Alignment)` whenever
the corresponding descriptor field (`kResolutionField` /
`kAlignField`) is valid. `Resolution` MUST be a typed enum covering
the 6 / 8 / 10 / 12 / 14 / 16-bit values; the implementation MUST
`static_assert` against `kResultBits` so a request that exceeds the
peripheral's published ceiling fails at compile time.

#### Scenario: STM32G0 ADC1 honours 12 / 10 / 8 / 6-bit resolution

- **WHEN** an application targets `nucleo_g071rb` and calls
  `adc.set_resolution(Resolution::Bits10)` against ADC1
- **THEN** the call writes the encoded value into the `RES` field of
  the configuration register through `runtime_ops::modify_field`
- **AND** subsequent conversions return 10-bit-right-aligned data

#### Scenario: Requesting a resolution above the peripheral ceiling fails to compile

- **WHEN** an application targets a backend whose `kResultBits = 12`
  and calls `adc.set_resolution(Resolution::Bits16)`
- **THEN** the build fails with the `static_assert` "requested
  resolution exceeds the peripheral's kResultBits"

#### Scenario: Backends without a resolution field reject at runtime

- **WHEN** an application targets a backend whose `kResolutionField`
  is invalid
- **THEN** `set_resolution(...)` returns `core::ErrorCode::NotSupported`
- **AND** no MMIO write is performed

### Requirement: ADC HAL SHALL Expose Continuous Mode And Stop

The HAL SHALL expose `set_continuous(bool)` whenever
`kContinuousField` is valid, and SHALL expose `stop()` whenever
`kStopField` is valid. `stop()` is the explicit inverse of the
existing `start()`; together they let an application toggle
free-running without re-running `configure()`.

#### Scenario: SAME70 AFEC honours free-run on / off

- **WHEN** an application targets `same70_xplained` and toggles
  `set_continuous(true)` then `set_continuous(false)` on AFEC0
- **THEN** the `FREERUN` bit in `AFEC_MR` is set then cleared
- **AND** subsequent conversions follow the requested mode

#### Scenario: stop() on a backend without kStopField is NotSupported

- **WHEN** the backend descriptor reports `kStopField.valid == false`
- **THEN** `stop()` returns `core::ErrorCode::NotSupported`

### Requirement: ADC HAL SHALL Expose A Per-Channel Sample-Time Setter

The HAL SHALL expose `set_sample_time(Channel, std::uint32_t ticks)`
whenever `kSampleTimeRegister.valid`. The call MUST translate the
channel id and tick count into a write through the descriptor's
sample-time register; the encoding details (per-channel bit field
vs shared register) live in `runtime_ops`, not in the HAL.

#### Scenario: ADC1 sample time is independent per channel

- **WHEN** an application calls
  `adc.set_sample_time(Channel::CH0, 4)` followed by
  `adc.set_sample_time(Channel::Vrefint, 192)`
- **THEN** the two channels carry distinct sample-time configurations
- **AND** subsequent sequence conversions use the correct sample time
  for each channel

#### Scenario: Backends without sample-time control reject explicitly

- **WHEN** the backend descriptor reports
  `kSampleTimeRegister.valid == false`
- **THEN** `set_sample_time(...)` returns
  `core::ErrorCode::NotSupported`

### Requirement: ADC HAL SHALL Expose A Typed Channel Enum Per Peripheral

For every published `AdcSemanticTraits<P>`, the runtime SHALL expose
a typed `Channel` enum scoped to that peripheral. `ADC1::Channel` is
distinct from `ADC2::Channel` so the type system rejects
cross-peripheral channel mixing. The enum values MUST cover both
peripheral channels and any internal channels (Vrefint, VBat,
temperature sensor) declared by the descriptor.

#### Scenario: Cross-peripheral channel mixing fails to compile

- **WHEN** an application calls
  `adc1.set_sample_time(Adc2::Channel::CH3, 8)` (mixing types)
- **THEN** the build fails with a type-mismatch error
- **AND** no implicit conversion silently maps the wrong channel

#### Scenario: Internal channels are reachable by name

- **WHEN** an application calls
  `adc.read_sequence(...)` after
  `adc.set_sequence(std::array{Channel::CH0, Channel::Vrefint})`
- **THEN** the second sample in the result span is the Vrefint
  reading (raw, calibration deferred)

### Requirement: ADC HAL SHALL Expose A Sequence Builder

The HAL SHALL expose
`set_sequence(std::span<const Channel> channels)` whenever
`kSequenceRegister.valid` and at least one of `kChannelBitPattern`
/ `kChannelEnablePattern` is valid. The implementation MUST handle
both backends:

- **Bitmask sequence** (`kChannelBitPattern.valid`, ST G0/F4 path):
  one bit per channel position in the sequence register; the
  iteration writes the channel id at each position.
- **Per-channel enable register** (`kChannelEnablePattern.valid`,
  Microchip AFEC path): one register write per enabled channel
  through the indexed pattern.

An empty input span clears the sequence. A span larger than
`kChannelCount` returns `core::ErrorCode::InvalidArgument`.

#### Scenario: STM32G0 sequence accepts ordered channels

- **WHEN** an application calls
  `adc.set_sequence(std::array{Channel::CH4, Channel::CH0,
   Channel::CH1})` against ADC1 on `nucleo_g071rb`
- **THEN** the next conversion sweep visits CH4 → CH0 → CH1 in that
  order
- **AND** `read_sequence(samples)` returns the three samples in the
  same order

#### Scenario: SAME70 AFEC sequence uses per-channel enable

- **WHEN** an application calls
  `adc.set_sequence(std::array{Channel::CH3, Channel::CH7})` on
  AFEC0
- **THEN** the implementation writes through `kChannelEnablePattern`
  rather than the bitmask path
- **AND** the conversion sweep visits CH3 then CH7

#### Scenario: Oversize span is rejected

- **WHEN** an application passes a span with more entries than
  `kChannelCount`
- **THEN** the call returns `core::ErrorCode::InvalidArgument`
- **AND** no MMIO write is performed (the previous sequence stays
  intact)

### Requirement: ADC HAL SHALL Expose Per-Channel Enable When The Backend Supports It

The HAL SHALL expose `enable_channel(Channel)`,
`disable_channel(Channel)`, and `channel_enabled(Channel) -> bool`
whenever `kChannelEnablePattern.valid` (paired with the corresponding
`kChannelDisablePattern` and `kChannelStatusPattern`). On backends
that use bitmask-style sequence control instead, these calls return
`core::ErrorCode::NotSupported`.

#### Scenario: SAME70 AFEC channels are individually toggleable

- **WHEN** an application calls
  `adc.enable_channel(Channel::CH3)` then
  `adc.channel_enabled(Channel::CH3)` on AFEC0
- **THEN** the second call returns `true`
- **AND** a subsequent `adc.disable_channel(Channel::CH3)` flips it
  back

### Requirement: ADC HAL SHALL Expose Hardware Trigger Configuration

The HAL SHALL expose
`set_hardware_trigger(std::uint8_t source, TriggerEdge edge)`
whenever both `kExternalTriggerEnableField` and
`kExternalTriggerSelectField` are valid. `TriggerEdge` covers
`Disabled`, `Rising`, `Falling`, `Both`. The `source` integer is
clamped at compile time to the descriptor's
`kExternalTriggerSelectField` width.

#### Scenario: ADC1 starts on TIM3 update edge

- **WHEN** an application calls
  `adc.set_hardware_trigger(/*TIM3_TRGO source idx*/ 4u,
  TriggerEdge::Rising)` against ADC1 on `nucleo_g071rb`
- **THEN** subsequent TIM3 update events kick off a conversion sweep
  without any further `start()` call
- **AND** disabling via `set_hardware_trigger(0, TriggerEdge::Disabled)`
  reverts to software-start mode

### Requirement: ADC HAL SHALL Expose End-Of-Sequence And Overrun Status

The HAL SHALL expose
- `end_of_sequence() -> bool` (gated on `kEndOfSequenceField`)
- `overrun() -> bool` (gated on `kOverrunField`)
- `clear_overrun() -> core::Result<void, core::ErrorCode>` (clears
  the field)

These status accessors let applications detect lost samples and
react (back off the trigger rate, drain the buffer, log).

#### Scenario: Overrun is observable and clearable

- **WHEN** the application is too slow draining a continuous sequence
  and `kOverrunField` flips
- **THEN** `adc.overrun()` returns `true`
- **AND** `adc.clear_overrun()` returns `Ok` and the next
  `adc.overrun()` returns `false`

### Requirement: ADC HAL SHALL Provide A Polled Sequence-Read Helper

The HAL SHALL expose
`read_sequence(std::span<std::uint16_t> samples)` whenever
`kEndOfConversionField` and `kDataField` are valid. The
implementation MUST loop over the input span, polling
`kEndOfConversionField` and reading `kDataField` for each entry. It
MUST short-circuit with `core::ErrorCode::Overrun` if `kOverrunField`
flips during the loop.

#### Scenario: Polled four-channel sequence read returns four samples

- **WHEN** an application calls
  `adc.read_sequence(samples)` with a 4-element span after
  `adc.set_sequence(std::array{Channel::CH0, Channel::CH1,
   Channel::CH2, Channel::CH3})` and `adc.start()`
- **THEN** the call returns `Ok` and the span contains four samples
  in conversion order
- **AND** the sequence is conceptually identical to running four
  single conversions back to back

### Requirement: Async ADC Scan SHALL Honour End-Of-Sequence Or DMA Transfer-Complete Per Caller

The runtime `async::adc::scan_dma<P>` wrapper MUST accept a
`CompletionTrigger` argument selecting between
`DmaTransferComplete` (signals when the DMA buffer fills) and
`EndOfSequence` (signals at every sequence boundary). The default
MUST stay `DmaTransferComplete` for backward compatibility with the
adapter shipped in `complete-async-hal`.

#### Scenario: EndOfSequence trigger wakes the task per sequence

- **WHEN** an application awaits
  `async::adc::scan_dma<ADC1>(adc, dma, samples,
  CompletionTrigger::EndOfSequence)` with a 4-channel sequence and
  a 64-sample circular DMA buffer
- **THEN** the task resumes every time the four channels have been
  sampled (16 wakes per buffer fill)
- **AND** `adc.end_of_sequence()` is true on each resume

#### Scenario: Default trigger keeps existing async ADC code working

- **WHEN** an application calls the existing
  `async::adc::scan_dma<P>(adc, dma, samples)` without specifying
  `complete_on`
- **THEN** the wrapper defaults to `DmaTransferComplete`
- **AND** existing examples keep their pre-change behaviour byte-for-byte
