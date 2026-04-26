# public-hal-api Spec Delta: RTC Coverage Extension

## ADDED Requirements

### Requirement: RTC HAL SHALL Expose Hour Mode And Prescaler Setters

The `alloy::hal::rtc::handle<P>` MUST expose
`enum class HourMode { Hour24, Hour12AmPm }` plus
`set_hour_mode(HourMode)`. The HAL MUST also expose
`set_async_prescaler(std::uint16_t)` and
`set_sync_prescaler(std::uint16_t)` whenever
`kPrescalerRegister.valid`. Backends without the prescaler MUST
return `core::ErrorCode::NotSupported`.

#### Scenario: STM32 RTC accepts 12h or 24h mode

- **WHEN** an application calls
  `rtc.set_hour_mode(HourMode::Hour12AmPm)` after entering init
  mode
- **THEN** the call succeeds and FMT bit is programmed to 1

### Requirement: RTC HAL SHALL Expose Typed Time / Date / Alarm Structs

The HAL MUST expose `struct Time`, `struct Date`,
`struct AlarmConfig`, plus `set_time(Time)`, `set_date(Date)`,
`current_time() -> Time`, `current_date() -> Date`,
`set_alarm(AlarmConfig)` (gated on `kHasAlarm`). The HAL MUST
convert to / from BCD internally so callers never deal with the
raw register encoding.

#### Scenario: Application sets time without dealing with BCD

- **WHEN** an application calls
  `rtc.set_time(Time{14, 30, 0, AmPm::Pm24h})`
- **THEN** the RTC TR register is programmed with the BCD-encoded
  equivalent of 14:30:00
- **AND** `rtc.current_time()` returns the same Time struct
  (modulo elapsed seconds)

### Requirement: RTC HAL SHALL Expose Calendar Event / Tamper / Backup Registers Per Capability

The HAL MUST expose, each gated:

- Calendar event (`kHasCalendar`):
  `enable_calendar_event(bool)`,
  `calendar_event_pending()`,
  `clear_calendar_event()`.
- Tamper / second / time-event clear methods — gated on
  the matching descriptor clear-side fields.
- Backup registers (`kBackupRegister.valid`):
  `read_backup(std::uint8_t idx) -> std::uint32_t`,
  `write_backup(std::uint8_t idx, std::uint32_t)`.

#### Scenario: Backup register survives reset

- **WHEN** an application calls
  `rtc.write_backup(0, 0xCAFE'F00D)`, asserts a software reset,
  and reads back via `rtc.read_backup(0)`
- **THEN** the read returns `0xCAFE'F00D`

### Requirement: RTC HAL SHALL Expose Typed Interrupt Setters And IRQ Number List

The HAL MUST expose
`enum class InterruptKind { Alarm, AlarmB, Wakeup, CalendarEvent,
Tamper, Second, TimeEvent }` plus
`enable_interrupt(InterruptKind)` /
`disable_interrupt(InterruptKind)` — each kind gated on the
corresponding control-side IE field. The HAL MUST also expose
`irq_numbers() -> std::span<const std::uint32_t>`.

#### Scenario: Wakeup interrupt is gated on the wakeup field

- **WHEN** an application calls
  `rtc.enable_interrupt(InterruptKind::Wakeup)` on a peripheral
  whose `kWakeupInterruptEnableField.valid` is false
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Async RTC Adapter SHALL Add wait_for(InterruptKind)

The runtime `async::rtc` namespace MUST expose
`wait_for<P>(handle, InterruptKind kind)` so a coroutine can
`co_await` an Alarm / Wakeup / Tamper event.

#### Scenario: Coroutine wakes on alarm fire

- **WHEN** a task awaits
  `async::rtc::wait_for<RTC>(rtc, InterruptKind::Alarm)` after
  configuring an alarm at 60 s offset
- **THEN** the task resumes when the alarm fires and the awaiter
  returns `Ok`
