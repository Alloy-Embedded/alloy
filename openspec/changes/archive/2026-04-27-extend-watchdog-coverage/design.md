# Design: Extend Watchdog Coverage

## Context

Watchdog HAL today is `enable / disable / refresh / configure`.
Descriptor publishes window mode, early-warning interrupt, and
five status flags + clear surface.

## Goals

1. Match modm-class watchdog completeness on every device.
2. Stay schema-agnostic via `if constexpr` capability gates.
3. Compose with `complete-async-hal`'s pattern.
4. Additive only.

## Key Decisions

### Decision 1: `set_window(cycles)` is raw cycle count

The HAL exposes raw `std::uint16_t cycles`; user converts from
their ms requirement using the resolved kernel clock + prescaler.
Floating-point ms-to-cycles conversion stays out of the HAL.

### Decision 2: Early-warning is interrupt-only, not async-blocking

`async::watchdog::wait_for(InterruptKind::EarlyWarning)` exists,
but the canonical use is to register an ISR that captures crash
state in a backup register *before* the inevitable reset, not to
"recover" — recovery is contradictory to watchdog semantics. The
docs make this explicit.

### Decision 3: `set_reset_on_timeout(bool)` consolidates two paths

Some devices have separate "watchdog enable" and "reset enable"
bits (allowing watchdog as a free-running timer without reset).
The HAL exposes the high-level intent; the field gate determines
which underlying bit gets touched.

## Risks

- **Watchdog can't be disabled on some devices.** Documented; the
  HAL's `disable()` returns `NotSupported` on those backends
  rather than silently no-op-ping.
- **Window-mode refresh discipline.** Refreshing too early
  triggers reset on STM32 WWDG. Documented; the HAL doesn't
  enforce timing — that's the application's job.

## Migration

No source changes for existing users. New methods are additive.
