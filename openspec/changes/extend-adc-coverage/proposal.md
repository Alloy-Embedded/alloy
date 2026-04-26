# Extend ADC Coverage To Match Published Descriptor Surface

## Why

Alloy's ADC HAL was scoped to "minimum viable" while the cross-vendor
descriptor pipeline matured. With the latest `alloy-devices` publication,
every foundational ST / Microchip / NXP device now exposes a rich
`AdcSemanticTraits` surface — capability flags, six typed register refs,
eighteen typed field refs, and four indexed-field patterns covering
per-channel enable / disable / status / bit-pattern selection.

The runtime currently consumes ~30% of that surface:
`configure / enable / start / ready / read / enable_dma / disable_dma /
configure_dma / data_register_address`. The remaining 70% — resolution,
alignment, continuous mode, per-channel sample time, sequence builder,
hardware-trigger source / edge, end-of-sequence, overrun, channel-bitmask
selection — sits unused on the device side, so applications either
hardcode raw register writes or accept the defaults.

modm sets the bar for what "complete ADC" looks like in C++ on
embedded: typed channel enums per peripheral, sequence builders,
calibration helpers, internal-channel awareness, async DMA scan with
half / full transfer events. Alloy already publishes the data needed
to match that bar; this change is plumbing.

Concrete scope: lift every published `AdcSemanticTraits` field into
typed HAL methods (capability-gated via `if constexpr` against
`semantic_traits::kHas…` / `kInvalid…` ref guards), wire the new DMA
levers into the existing `async::adc::scan_dma` from
`complete-async-hal`, ship a `analog_probe_complete` example exercising
every lever, and document the resulting surface in `docs/ADC.md`.

## What Changes

### `src/hal/adc.hpp` — extended HAL surface (additive only)

New methods on `handle<Peripheral>`. Every method is capability-gated
via `if constexpr` against the published trait field's `.valid` flag.
Unsupported levers return `core::ErrorCode::NotSupported` rather than
silently no-op.

- **Resolution / alignment**
  - `set_resolution(Resolution)` — `Resolution` is a typed enum
    (`Bits6`, `Bits8`, `Bits10`, `Bits12`, `Bits14`, `Bits16`); the
    `static_assert` clamps to `kResultBits` ceiling.
  - `set_alignment(Alignment)` — `Left` / `Right`.
- **Mode**
  - `set_continuous(bool)` — single-shot vs free-running.
  - `stop()` — the missing inverse of `start()`.
- **Sample time**
  - `set_sample_time(Channel, std::uint32_t ticks)` — per-channel; used
    when `kSampleTimeRegister.valid`.
- **Channel sequence**
  - `set_sequence(std::span<const Channel> channels)` — programmes
    ordered conversion sequence using `kSequenceRegister` +
    `kChannelBitPattern`.
  - `enable_channel(Channel)` / `disable_channel(Channel)` /
    `channel_enabled(Channel) -> bool` — for backends with
    independent per-channel enable bits (`kChannelEnablePattern`,
    `kChannelDisablePattern`, `kChannelStatusPattern`); the SAM-E70
    AFEC path).
- **Hardware trigger**
  - `set_hardware_trigger(TriggerSource source, TriggerEdge edge)` —
    `TriggerEdge` is `Disabled` / `Rising` / `Falling` / `Both`;
    `TriggerSource` is a typed enum bound to the descriptor's
    `kExternalTriggerSelectField` value space (per-peripheral).
- **Sequence read**
  - `read_sequence(std::span<std::uint16_t> samples)` — drains an
    `kHasDma`-disabled multi-channel scan via repeated `kDataField`
    reads gated on `kEndOfConversionField`.
- **Status**
  - `end_of_sequence() -> bool` (uses `kEndOfSequenceField`)
  - `overrun() -> bool` / `clear_overrun()` (uses `kOverrunField`)

### Typed per-peripheral channel enum

The runtime emitter SHALL produce a typed `Channel` enum scoped to each
ADC peripheral, derived from the existing channel-bit-pattern indexed
field. `ADC1::Channel` and `ADC2::Channel` are distinct types so an
application cannot accidentally pass an ADC2 channel to ADC1.

Channels include the published peripheral channels plus any internal
channels declared by the descriptor (Vrefint, VBat, temperature
sensor) — the indexed-field iteration covers them automatically.

### DMA integration

Existing `configure_dma()` stays; the new `set_sequence()` +
`set_continuous(true)` + `enable_dma(circular=true)` composition gives
modm-style continuous-scan-into-buffer.

The async wrapper from `complete-async-hal` (`async::adc::scan_dma`)
gains a richer signature: it now reads `kEndOfSequenceField` for
sequence completion rather than (or in addition to) the DMA TC interrupt,
allowing precise wake on "all channels in this sequence have a fresh
sample".

### `examples/analog_probe_complete/`

Exercises every new lever on `nucleo_g071rb` (one ADC, multiple
channels, continuous DMA scan, hardware trigger from TIM3 update,
overrun monitor, channel-by-channel sample time tuning). Mirrors a
modm `adc::scan` recipe.

### Docs

`docs/ADC.md` — new comprehensive guide. Replaces the scattered ADC
mentions in COOKBOOK.md / SUPPORT_MATRIX.md notes. Sections: model,
single-shot recipe, continuous-DMA recipe, hardware-trigger recipe,
internal-channel discovery, error semantics, modm migration table for
users moving over.

## What Does NOT Change

- Existing API (`configure/enable/start/ready/read/enable_dma/
  disable_dma/configure_dma/data_register_address`) is unchanged. Every
  new method is additive.
- ADC tier in `docs/SUPPORT_MATRIX.md` stays `foundational` —
  hardware spot-checks for the new levers land per board with
  the existing 3-board matrix (SAME70 / STM32G0 / STM32F4).
- The descriptor surface itself is not changed in this proposal — it's
  already published and stable.
- The DAC HAL is intentionally untouched; a parallel `extend-dac-coverage`
  is a future change if there's demand.

## Out of Scope (Follow-Up Changes)

- **ESP32 / RP2040 / AVR-DA ADC support.** The descriptors exist but
  set `kPresent = false` for ADC today. When alloy-devices publishes
  the schemas, an additive `add-adc-coverage-esp32` /
  `add-adc-coverage-rp2040` / `add-adc-coverage-avr-da` change
  enables the same HAL surface against the new traits.
- **Injected channels** (STM32 ADC injected sequence / SAME70 AFEC
  injected mode). The descriptor doesn't yet publish injected-mode
  fields. Tracked as `add-adc-injected-channels` (alloy-codegen +
  alloy follow-up pair).
- **Analog watchdog / window comparator.** Same status as injected.
  Tracked as `add-adc-watchdog`.
- **Differential channels** (where supported). Same status. Tracked
  as `add-adc-differential`.
- **Calibration sequence helpers.** ST/Microchip/NXP all expose
  calibration registers, but the descriptor doesn't yet publish
  calibration field refs uniformly. Tracked as `add-adc-calibration`
  once the descriptor surface lands.
- **Hardware spot-checks for the new levers.** Land per board with
  the existing `analog_probe` matrix in a follow-up
  `validate-adc-coverage-on-3-boards`.

## Alternatives Considered

**Generate the entire ADC HAL from the schema id.** Each schema
(`schema_alloy_adc_st_aditf4_v3_0_g0_cube`, `…_microchip_afec_s`,
`…_nxp_etc_v2_imxrt`) could drive a per-vendor template. Rejected —
schema-aware code violates the runtime/device boundary; the runtime is
supposed to reach for typed register / field refs, not for vendor
peripheral type names. The capability-flag + indexed-field-pattern
approach already absorbs every difference (e.g. AFEC's per-channel
enable register vs G0's bitmask sequence) without naming schemas.

**Wait for ESP/RP/AVR-DA descriptors before extending.** Rejected —
gating ST/Microchip/NXP coverage on Espressif's ADC publication blocks
the 3-board hardware matrix that's already foundational. The HAL's
capability gates already produce `NotSupported` cleanly when a trait is
absent; ESP/RP/AVR users see the same message they see today, and the
existing ADC HAL keeps working unchanged.

**Hide raw access behind helpers only.** Rejected — `data_register_address()`
is documented as "the address you give to your DMA controller for
custom transfer chains", which is exactly what advanced users need.
The new sequence / trigger / sample-time helpers are layered on top
without removing the escape hatch.
