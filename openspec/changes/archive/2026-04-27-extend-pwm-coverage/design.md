# Design: Extend PWM Coverage

## Context

The runtime exposes the bare-minimum PWM HAL. The descriptor
publishes 13 capability flags + dedicated dead-time rise/fall +
fault input + master-output-enable + synchronized-update + paired
channel mapping. This document captures the design for closing the
gap.

## Goals

1. Match modm-class PWM completeness on every device the descriptor
   declares PWM for: complementary outputs with dead-time, fault
   input shutdown, synchronized update, carrier modulation,
   force-initialization.
2. Stay schema-agnostic: capability-flag + typed-field-ref absorbs
   STM32 advanced TIM vs SAM PWM vs NXP FlexPWM.
3. Compose with `complete-async-hal`'s pattern for async fault /
   update events.
4. Keep the existing API byte-compatible — additive only.

## Non-Goals

- Higher-level motor-control driver. Lives in a follow-up
  `add-motor-control-driver` once this HAL surface stabilises.
- ESP32 / RP2040 / AVR-DA PWM feature parity.

## Key Decisions

### Decision 1: Capability gates via `if constexpr`

Same pattern as ADC / UART / SPI / I2C / Timer.

### Decision 2: Dead-time as `(rise_cycles, fall_cycles)` pair

STM32 advanced TIM has a single 8-bit DTG field; SAM PWM has
separate DTHI / DTLI. The HAL exposes the pair as the cross-vendor
superset:

```cpp
set_dead_time(std::uint8_t rise_cycles, std::uint8_t fall_cycles);
```

Backends with a combined field (STM32) take `(rise + fall) / 2` as
the DTG value and document the loss-of-precision in `docs/PWM.md`.

### Decision 3: Fault / break / brake unified under one interrupt kind

The descriptor distinguishes `kHasFaultInput`, `kSupportsBrake`,
and `kSupportsCarrierModulation`. `Fault` and `Break` are
overlapping concepts — STM32 BKIN is functionally identical to
SAM PWM fault input. The HAL's `InterruptKind::Fault` covers both;
`InterruptKind::Break` is a separate kind on STM32 advanced timers
where break is distinct from fault input (rare). Backends without
either return `NotSupported`.

### Decision 4: `paired_channels` accessor uses `kPairedChannels` array

The descriptor publishes a `kPairedChannels` array mapping channel
index → its complementary partner. The HAL exposes
`static constexpr` accessor returning the partner; users don't
have to know the CH1 / CH1N convention.

### Decision 5: `set_update_synchronized(bool)` is the single sync knob

STM32 has a per-channel preload bit + a global update event;
SAM PWM has a single sync register controlling the boundary. The
HAL exposes a single `bool` toggle that turns on per-channel
preload + UEV-triggered transfer (STM32 path) or the SYNC bit
(SAM path). The descriptor's `kHasSynchronizedUpdate` capability
flag gates the method.

## Risks

- **Dead-time precision loss on STM32.** Documented; users who need
  separate rise / fall control on STM32 advanced TIM have to drop
  to raw register access.
- **Fault input wiring varies wildly.** STM32 BKIN goes through
  CR2; SAM PWM has its own fault matrix. The HAL only exposes
  enable / polarity / status; the specific fault routing
  (filter, source) is per-vendor and not yet in the descriptor.
  Tracked as `add-pwm-fault-routing` follow-up.

## Migration

No source-code changes for existing users. Methods are additive.
`docs/PWM.md` includes a "before / after" migration table.
