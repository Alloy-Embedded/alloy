# Tasks: Extend RTC Coverage

## 1. Hour mode + prescaler + calendar setters

- [x] 1.1 `enum class HourMode { Hour24, Hour12AmPm }`.
      `set_hour_mode(HourMode)`.
- [x] 1.2 `set_async_prescaler(std::uint8_t)` and
      `set_sync_prescaler(std::uint16_t)` — both gated on
      `kPrescalerRegister.valid`.
- [x] 1.3 `struct Time { hours, minutes, seconds, pm }`,
      `struct Date { year, month, day, weekday }`.
      `set_time(Time)`, `set_date(Date)`, `current_time() -> Time`,
      `current_date() -> Date`.

## 2. Alarm + calendar event + tamper

- [x] 2.1 `struct AlarmConfig { day_match, hours, minutes,
      seconds, use_weekday, pm }`. `set_alarm(AlarmConfig)` —
      gated on `kHasAlarm`.
- [x] 2.2 Calendar event (`kHasCalendar`):
      `enable_calendar_event(bool)`,
      `calendar_event_pending()`,
      `clear_calendar_event()`.
- [x] 2.3 Tamper / second / time-event clear methods — each gated
      on the matching `kClearTamperErrorField` /
      `kClearSecondField` / `kClearTimeEventField`.

## 3. Interrupts + backup registers + IRQ vector

- [x] 3.1 `enum class InterruptKind { Alarm, AlarmB, Wakeup,
      CalendarEvent, Tamper, Second, TimeEvent }`.
      `enable_interrupt` / `disable_interrupt` — per-kind gated.
- [x] 3.2 Backup registers (`kBackupRegister.valid`):
      `read_backup(std::uint8_t)`,
      `write_backup(std::uint8_t, std::uint32_t)`.
      Note: no `kBackupRegisterBase` in any current DB — always returns
      NotSupported. DB task deferred (10.6).
- [x] 3.3 `irq_numbers() -> std::span<const std::uint32_t>`.

## 4. Compile tests + async + example + HW

- [x] 4.1 Extend `tests/compile_tests/test_rtc_api.cpp`.
- [x] 4.2 `async::rtc::wait_for(InterruptKind)` runtime sibling +
      compile test entry.
- [ ] 4.3 `examples/rtc_probe_complete/`: targets `same70_xplained`,
      24h mode, alarm at 60 s offset, async wait_for(Alarm) toggles
      LED.
- [ ] 4.4 Mirror on `nucleo_g071rb` and `nucleo_f401re`.
- [ ] 4.5 SAME70 / G0 / F4 60-second alarm spot-check.
- [ ] 4.6 Update `docs/SUPPORT_MATRIX.md` `rtc` row.

## 5. Documentation + follow-ups

- [ ] 5.1 `docs/RTC.md` — comprehensive guide.
- [ ] 5.2 Cross-link from `docs/ASYNC.md`.
- [ ] ] 5.3 File `add-rtc-subsecond-precision-validation` follow-up.

## 10. Device-database follow-ups (deferred)

- [ ] 10.6 Publish `kBackupRegisterBase` (indexed field) for STM32 RTC BKPxR.
