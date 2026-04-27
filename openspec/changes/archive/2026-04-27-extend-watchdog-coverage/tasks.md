# Tasks: Extend Watchdog Coverage

## 1. Window mode + early warning + status flags

- [x] 1.1 `set_window(std::uint16_t cycles)`,
      `enable_window_mode(bool)` — gated on `kHasWindow`.
- [x] 1.2 `enable_early_warning(std::uint16_t cycles_before_timeout)`,
      `early_warning_pending()`, `clear_early_warning()` — gated on
      `kEarlyWarningInterruptEnableField.valid`.
- [x] 1.3 Status flags: `timeout_occurred`,
      `prescaler_update_in_progress`,
      `reload_update_in_progress`,
      `window_update_in_progress`, `error` — each gated on
      matching status field.
- [x] 1.4 `set_reset_on_timeout(bool)` — gated on
      `kResetEnableField.valid`.

## 2. Kernel clock + interrupts + IRQ vector

- [x] 2.1 `set_kernel_clock_source(KernelClockSource)` — gated.
      Note: no `kKernelClockSourceField` published in any current backend;
      API present, always returns NotSupported. DB task deferred (10.5).
- [x] 2.2 `enum class InterruptKind { EarlyWarning }`.
      `enable_interrupt` / `disable_interrupt`.
- [x] 2.3 `irq_numbers() -> std::span<const std::uint32_t>`.

## 3. Compile tests + async + example + HW

- [x] 3.1 Extend `tests/compile_tests/test_watchdog_api.cpp`.
- [x] 3.2 `async::watchdog::wait_for(InterruptKind)` runtime
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

## 10. Device-database follow-ups (deferred)

- [ ] 10.5 Publish `kKernelClockSourceField` in watchdog traits for
      STM32 devices that support IWDG clock mux (STM32H7+).
