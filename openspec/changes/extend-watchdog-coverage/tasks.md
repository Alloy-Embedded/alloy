# Tasks: Extend Watchdog Coverage

## 1. Window mode + early warning + status flags

- [ ] 1.1 `set_window(std::uint16_t cycles)`,
      `enable_window_mode(bool)` — gated on `kHasWindow`.
- [ ] 1.2 `enable_early_warning(std::uint16_t cycles_before_timeout)`,
      `early_warning_pending()`, `clear_early_warning()` — gated on
      `kEarlyWarningInterruptEnableField.valid`.
- [ ] 1.3 Status flags: `timeout_occurred`,
      `prescaler_update_in_progress`,
      `reload_update_in_progress`,
      `window_update_in_progress`, `error` — each gated on
      matching status field.
- [ ] 1.4 `set_reset_on_timeout(bool)` — gated on
      `kResetEnableField.valid`.

## 2. Kernel clock + interrupts + IRQ vector

- [ ] 2.1 `set_kernel_clock_source(KernelClockSource)` — gated.
- [ ] 2.2 `enum class InterruptKind { EarlyWarning }`.
      `enable_interrupt` / `disable_interrupt`.
- [ ] 2.3 `irq_numbers() -> std::span<const std::uint32_t>`.

## 3. Compile tests + async + example + HW

- [ ] 3.1 Extend `tests/compile_tests/test_watchdog_api.cpp`.
- [ ] 3.2 `async::watchdog::wait_for(InterruptKind)` runtime
      sibling + compile test.
- [ ] 3.3 `examples/watchdog_probe_complete/`: targets
      `nucleo_g071rb` IWDG + WWDG with window + early-warning
      handler dumping to backup register.
- [ ] 3.4 SAME70 / G0 / F4 spot-check: 60 s of refresh + intentional
      late-refresh triggering early-warning.
- [ ] 3.5 Update `docs/SUPPORT_MATRIX.md` `watchdog` row.

## 4. Documentation + follow-ups

- [ ] 4.1 `docs/WATCHDOG.md` — model, IWDG vs WWDG comparison,
      window-mode recipe, early-warning recipe.
- [ ] 4.2 Cross-link from `docs/ASYNC.md`.
