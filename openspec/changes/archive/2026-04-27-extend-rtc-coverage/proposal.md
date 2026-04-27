# Extend RTC Coverage To Match Published Descriptor Surface

## Why

`RtcSemanticTraits<P>` publishes calendar / alarm / time / date
registers, hour-mode + write-protection + init-mode fields,
prescaler register, and capability flags (`kHasAlarm`,
`kHasCalendar`, `kHasWriteProtection`), plus a typed interrupt
matrix (alarm-IE, calendar-event-IE, tamper-error clear, plus
status-side flags for each).

The runtime currently consumes ~50%: HAL exposes
`configure / enable_write_access / enter_init_mode /
leave_init_mode / init_ready / enable_alarm_interrupt /
alarm_pending`. Hour-mode, prescaler programming, calendar event,
typed interrupt enable/disable, second-resolution alarm match,
sub-second register, tamper-error clear — none reachable today.

modm covers RTC end-to-end: 12/24-hour mode, BCD encoding,
sub-second alarms, periodic auto-wake, tamper detection. Alloy
already publishes the data needed; this change is plumbing.

## What Changes

### `src/hal/rtc.hpp` — extended HAL surface (additive only)

- **Hour mode**: `set_hour_mode(HourMode)` —
  `Hour24` / `Hour12AmPm`.
- **Prescaler**: `set_async_prescaler(std::uint16_t)`,
  `set_sync_prescaler(std::uint16_t)` — both gated on
  `kPrescalerRegister.valid`.
- **Time / date typed setters**:
  `set_time(std::uint8_t hours, std::uint8_t minutes,
  std::uint8_t seconds, AmPm = Pm24h)`,
  `set_date(std::uint16_t year, std::uint8_t month,
  std::uint8_t day, Weekday)`,
  `current_time() -> Time`, `current_date() -> Date`.
- **Alarm** (gated on `kHasAlarm`):
  `set_alarm(AlarmConfig)` where `AlarmConfig` carries day-mask /
  hour / minute / second / sub-second fields.
- **Calendar event** (gated on `kHasCalendar`):
  `enable_calendar_event(bool)`, `calendar_event_pending() -> bool`,
  `clear_calendar_event()`.
- **Tamper / second / time-event clear**: each gated on the
  corresponding clear-side field.
- **Typed interrupts**:
  `enum class InterruptKind { Alarm, AlarmB, Wakeup, CalendarEvent,
  Tamper, Second, TimeEvent }`.
  `enable_interrupt` / `disable_interrupt` — per-kind gated.
- **Backup registers** (gated on `kBackupRegister.valid`):
  `read_backup(std::uint8_t idx) -> std::uint32_t`,
  `write_backup(std::uint8_t idx, std::uint32_t)`.
- **NVIC vector lookup**: `irq_numbers() ->
  std::span<const std::uint32_t>`.
- **Async sibling**: `async::rtc::wait_for(InterruptKind)`.

### `examples/rtc_probe_complete/`

Targets `same70_xplained` RTC. Configures 24-hour mode, sets
date+time from compile-time stamp, alarm at 60 s offset, async
`wait_for(Alarm)` task that toggles LED on alarm fire.

### Docs

`docs/RTC.md` — model, calendar/alarm recipe, sub-second resolution,
tamper, backup registers, async wiring, modm migration table.

## What Does NOT Change

- Existing RTC API unchanged. Additive only.
- Tier stays `foundational`.

## Out of Scope (Follow-Up Changes)

- Sub-second resolution alarm precision validation (descriptor
  surface needs a sub-second-resolution field publication that's
  not uniform yet).
- ESP32 / RP2040 RTC — gated on alloy-codegen.
- HW spot-check follow-up.

## Alternatives Considered

`HourMode` as bool (24h / 12h) rejected — typed enum reads cleaner
at call sites (`set_hour_mode(HourMode::Hour24)`).
