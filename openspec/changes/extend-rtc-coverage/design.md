# Design: Extend RTC Coverage

## Context

RTC HAL today is `configure + alarm-interrupt-enable + alarm-pending`
+ init-mode helpers. Descriptor publishes the full calendar / alarm /
prescaler / hour-mode / tamper / backup-register surface.

## Goals

1. Match modm-class RTC completeness on every device the descriptor
   declares RTC for: hour mode, calendar, alarm with sub-second,
   tamper detection, backup registers, typed interrupts.
2. Stay schema-agnostic via `if constexpr` capability gates.
3. Compose with `complete-async-hal`'s pattern.
4. Additive only.

## Key Decisions

### Decision 1: Typed `Time` / `Date` / `AlarmConfig` structs

Rather than passing raw `std::uint32_t` representations of the
RTC's BCD calendar register, the HAL exposes typed structs with
named fields. The HAL converts to/from BCD internally.

```cpp
struct Time { uint8_t hours, minutes, seconds; AmPm am_pm; };
struct Date { uint16_t year; uint8_t month, day; Weekday weekday; };
struct AlarmConfig {
    DayMask  day_match;       // ignore-day / specific-date / weekday
    uint8_t  hours, minutes, seconds, subseconds;
    HourPrecision precision;  // which fields participate in match
};
```

### Decision 2: `InterruptKind` enum is the cross-vendor superset

`Alarm`, `AlarmB`, `Wakeup`, `CalendarEvent`, `Tamper`, `Second`,
`TimeEvent`. Each gated on its IE field. Backends without the
kind return `NotSupported`.

### Decision 3: Backup registers are addressable by index

The descriptor publishes `kBackupRegister` as an indexed-field
pattern. The HAL exposes `read_backup(idx)` /
`write_backup(idx, value)` clamped to the published count;
out-of-range returns `InvalidArgument`.

## Risks

- **Hour mode change while RTC running.** Some RTCs reject the
  change. The HAL documents that hour-mode change requires entering
  init mode first (existing API).
- **Sub-second alarm precision.** Varies wildly across devices.
  Documented; `HourPrecision` enum captures the field-mask matrix.

## Migration

No source changes for existing users. `docs/RTC.md` carries the
before / after table for users currently writing raw calendar
register hex.
