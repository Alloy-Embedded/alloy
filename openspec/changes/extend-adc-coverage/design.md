# Design: Extend ADC Coverage

## Context

The runtime currently exposes the bare-minimum ADC HAL. The descriptor
publishes a much richer surface that's been sitting unused. This
document captures the design decisions for closing the gap without
breaking the existing 3-board hardware matrix and without crossing the
runtime/device boundary.

## Goals

1. Match modm-class ADC completeness (resolution / alignment /
   sequence / sample-time / hardware trigger / continuous / overrun /
   per-channel enable) on every device the descriptor declares ADC
   for.
2. Stay schema-agnostic: the HAL reaches for typed
   `RuntimeRegisterRef` / `RuntimeFieldRef` / `RuntimeIndexedFieldRef`,
   not vendor schema names.
3. Compose cleanly with `complete-async-hal`'s `async::adc::scan_dma`
   so a sequence + DMA + EOS = single `co_await` round-trip.
4. Keep the existing API byte-compatible — every change is additive.

## Non-Goals

- ESP/RP/AVR-DA ADC, injected channels, analog watchdog, differential
  inputs, calibration sequences. All explicit follow-ups.
- Per-vendor optimisations. The HAL stays generic;
  `detail::runtime::modify_field` already yields one MMIO write per
  field touch.

## Key Decisions

### Decision 1: Capability gates via `if constexpr` on `.valid` flags

Every published trait field carries a `.valid` boolean. The HAL uses
`if constexpr (semantic_traits::kFooField.valid)` to either emit the
real implementation or fall through to `core::ErrorCode::NotSupported`.
The compiler eliminates the unused branch entirely — no runtime check,
no overhead on backends that don't support a feature.

This matches the existing pattern in
`enable() / start() / read() / enable_dma()` and is the obvious
extension. No new mechanism.

### Decision 2: Per-peripheral typed `Channel` enum

`ADC1::Channel` distinct from `ADC2::Channel`. Two implementation
options were on the table:

**(A) Generated enum** — codegen emits
`enum class Channel : std::uint8_t { CH0, CH1, …, VrefInt, VBat,
TempSensor };` per peripheral, with the count and names sourced from
the descriptor. Pro: zero runtime conversion; the type itself is the
documentation.

**(B) Strong-typed wrapper around `std::uint8_t`** — `template<auto P>
struct Channel { std::uint8_t index; };`. Pro: no codegen change.
Con: every API call site has to know the channel-index space and we
lose the IntelliSense list of available channels.

Going with **(A)**. It requires a small alloy-codegen extension to
emit the channel enum from the existing channel-bit-pattern indexed
field; once that lands, the HAL templates over `Channel` and the
typed enum value lights up downstream. Until codegen ships the enum,
the HAL accepts a `std::uint8_t` parameter as a transitional shim and
the build emits a warning when the channel exceeds `kChannelCount - 1`.

### Decision 3: `set_sequence` takes `std::span<const Channel>`

The argument is an ordered sequence of channel ids. The HAL writes
them into `kSequenceRegister` using `kChannelBitPattern` (one bit per
channel position) on G0/F4 ST or into `kChannelEnablePattern` (one
register per channel) on Microchip AFEC.

The two backends are unified by the indexed-field abstraction: the
HAL iterates the input span and calls `runtime::modify_indexed_field`
once per channel. The HAL doesn't care which underlying register
shape the descriptor used — that's `runtime_ops` plumbing.

If `set_sequence` is called with an empty span, the call clears the
existing sequence (sets every channel-position bit to 0). If called
with more channels than `kChannelCount`, the call returns
`core::ErrorCode::InvalidArgument`.

### Decision 4: `TriggerSource` typed enum is descriptor-derived

The descriptor's `kExternalTriggerSelectField` has a width (e.g. 3
bits → 8 sources) but doesn't carry the *names* of the sources. Names
live in the SVD / vendor docs.

Options:
- (i) Generate a per-peripheral `TriggerSource` enum from the SVD's
  field-value-enumerated entries (where present).
- (ii) Accept a raw `std::uint8_t` source index plus an `TriggerEdge`
  enum.

Going with **(ii)** for v1 because the SVD field-value enumeration is
inconsistent across vendors (G0 has named sources via SVD, AFEC
documents them in datasheet text only). Users pass the integer
they read from the datasheet; the HAL clamps against the field width.
A future codegen change can promote integers to a named enum once
descriptor coverage exists.

### Decision 5: Continuous + DMA + EOS = the canonical "scan" recipe

The most-requested recipe ("read N channels into a buffer at hardware
rate, give me a callback when the buffer is full") is built from:

1. `set_sequence(channels)` — programmes which channels in what order.
2. `set_continuous(true)` — auto-restart after each sequence.
3. `configure_dma(channel, circular=true)` — wires DMA to the data
   register; circular keeps wrapping.
4. `co_await async::adc::scan_dma<P>(adc, dma, samples)` — the
   wrapper signals on the DMA TC interrupt OR on
   `kEndOfSequenceField` set, whichever fires first.

The "fires first" detail matters: G0's DMA TC fires when the *DMA
buffer* fills, which equals "all channels in the sequence ×
circular-multiplier" sampled. EOS fires at every sequence boundary.
We expose both events; applications pick which they want via the
async wrapper variant.

### Decision 6: Internal channels are visible by default

Vrefint, VBat, temperature sensor — these are channels in the
descriptor's `kChannelCount` and the codegen-generated `Channel` enum
exposes them by name. Users who want raw mV values for those channels
will wrap with their own conversion (the HAL doesn't know the
calibration coefficients).

A future `extend-adc-calibration` change will expose those coefficients
as descriptor fields.

## Risks

- **`kHasChannelBitmaskSelect` semantics differ.** G0 uses one bit per
  channel position; Microchip uses one register per channel. The
  capability flag distinguishes them. Mitigation: the HAL has two code
  paths gated on the flag, each going through `runtime_ops`.
- **`set_resolution` against backends with non-uniform resolution
  enums.** ST G0 has a 2-bit RES field with values
  {00=12, 01=10, 10=8, 11=6}; AFEC has a 3-bit RES field with values
  {0=12, 1=14, 2=16-OS}. The HAL clamps a `Resolution` enum to
  `kResultBits` and writes the *integer* matching the descriptor's
  expected encoding. Mitigation: the value is hardcoded per schema in
  the codegen output (the descriptor's `kResolutionField` doesn't
  carry encoding, so v1 hardcodes the two known schemas; a future
  codegen extension can publish the encoding table).
- **`std::span<const Channel>` lifetime.** Callers must keep the span
  alive for the duration of the conversion; we document that explicitly
  and don't copy into the HAL because the buffer is consumed at
  configuration time, not during transfer.

## Migration

No source-code changes for existing users. The new methods are
additive. Existing examples continue to build and pass on the 3-board
matrix.

For users currently writing raw register hex to control resolution or
trigger source: the proposal's `docs/ADC.md` includes a one-page
"before / after" migration table (what hex you used to write → what
HAL call replaces it).
