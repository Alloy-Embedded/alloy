# Design: Extend I2C Coverage

## Context

The runtime exposes the bare-minimum I2C HAL. The descriptor
publishes 30+ field refs plus speed / addressing / SMBus / PEC /
clock-stretching capability flags. This document captures the
design for closing the gap without breaking the SAME70 hardware
spot-check matrix.

## Goals

1. Match modm-class I2C completeness on every device the descriptor
   declares I2C for: variable speed (100k / 400k / 1M), 10-bit
   addressing, clock-stretching control, SMBus + PEC, all status
   flags.
2. Stay schema-agnostic: capability-flag + typed-field-ref absorbs
   STM32 TIMINGR-style timing vs SAM CWGR-style vs NXP LPI2C.
3. Compose with `complete-async-hal`'s I2C wrappers.
4. Keep the existing API byte-compatible — every change is additive.

## Non-Goals

- Full slave-mode HAL. Descriptor publishes the fields; shipping a
  typed `I2cSlave<C>` surface is a separate change.
- ESP32 / RP2040 / AVR-DA I2C parity. They get the same surface as
  the publication catches up.

## Key Decisions

### Decision 1: Capability gates via `if constexpr`

Same pattern as ADC / UART / SPI. `if constexpr (semantic_traits::
kFooField.valid)` / `if constexpr (kSupportsSmbus)`. Compiler
eliminates the unused branch.

### Decision 2: `set_clock_speed(u32)` with ±5 % validation, plus `SpeedMode` preset

I2C TIMINGR (modern STM32) and CWGR (SAM) encode timing as a
combination of prescaler + SCL-high + SCL-low cycles. Multiple
encodings can produce the same realised rate; the HAL picks one
that lands within ±5 % of the requested rate and returns
`InvalidArgument` otherwise.

The `SpeedMode { Standard100kHz, Fast400kHz, FastPlus1MHz }` enum
is a typed convenience preset that maps to known-good TIMINGR
values per kernel clock. Backends without `kSupportsFastPlus`
return `NotSupported` for `FastPlus1MHz`.

### Decision 3: `AddressingMode` typed enum at the HAL level

The HAL exposes `AddressingMode { Bits7, Bits10 }`. Master mode
sends the address in 7-bit or 10-bit format per
`set_addressing_mode`. Reads / writes accept `std::uint16_t`
either way; the HAL truncates to 7 bits when `Bits7` is selected.

### Decision 4: SMBus / PEC are independent gates

Some peripherals support SMBus framing without PEC; some support
PEC without SMBus framing. The descriptor publishes both flags
independently. The HAL gates each method on the corresponding
flag.

### Decision 5: `InterruptKind` enum is the cross-vendor superset

Same shape as UART / SPI. `Tx`, `Rx`, `Stop`, `Tc`, `AddrMatch`,
`Nack`, `BusError`, `ArbitrationLoss`, `Overrun`, `PecError`,
`Timeout`, `SmbAlert`. Each kind gated on the descriptor's
control-side IE field.

## Risks

- **TIMINGR computation realisability.** Some kernel-clock /
  baudrate pairs simply cannot be realised within ±5 %. Documented;
  users can request a `SpeedMode` preset which is guaranteed to
  match a published reference value.
- **10-bit addressing on backends with single-byte address
  registers.** STM32 TIMINGR-era I2C handles 10-bit; SAM TWIHS
  needs explicit bit-9 setup. Mitigated by the descriptor's
  `kAddressingBitField` width — peripherals that only support
  7-bit return `NotSupported` from `set_addressing_mode(Bits10)`.

## Migration

No source-code changes for existing users. New methods are
additive. `docs/I2C.md` includes a "before / after" migration
table for users currently writing raw TIMINGR hex.
