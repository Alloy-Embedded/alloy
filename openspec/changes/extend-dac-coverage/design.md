# Design: Extend DAC Coverage

## Context

The runtime exposes the bare-minimum DAC HAL. Descriptor publishes
all the levers — per-channel addressing, hardware trigger, prescaler,
software reset, kernel clock, IRQ numbers — but the HAL only uses a
fraction.

## Goals

1. Match modm-class DAC completeness on every device the descriptor
   declares DAC for.
2. Stay schema-agnostic via `if constexpr` capability gates.
3. Compose with `complete-async-hal`'s pattern.
4. Additive only.

## Key Decisions

### Decision 1: Per-channel methods accept `std::uint8_t`

Same transitional shim used by Timer / PWM until
`add-dac-channel-typed-enum` lands in alloy-codegen. Out-of-range
returns `InvalidArgument`. Existing `write(value)` keeps targeting
channel 0; new `write_channel(ch, value)` covers channels 1+.

### Decision 2: `set_hardware_trigger(source, edge)` mirrors ADC

Source is a raw `std::uint8_t` clamped to the descriptor's
`kTriggerSelectField` width; edge is the typed `TriggerEdge` enum
(`Disabled`, `Rising`, `Falling`, `Both`). A future codegen change
promotes the source to a typed `DacTriggerSource<P>::type` enum.

### Decision 3: `InterruptKind` is the cross-vendor superset

`TransferComplete`, `Underrun`, `DmaComplete`. Each gated on the
corresponding control-side IE field.

## Risks

- **Multi-channel underrun status disambiguation.** STM32 has
  per-channel DMAUDRn flags; SAM has a single global flag. The
  HAL's `underrun()` returns true if ANY channel underran;
  `underrun_channel(ch)` returns per-channel state when supported.

## Migration

No source changes for existing users. `docs/DAC.md` carries the
before / after migration table.
