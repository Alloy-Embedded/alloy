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
      Stub returning `NotSupported` on every current backend; no
      descriptor publishes `kKernelClockSourceField` yet (IWDG runs
      on LSI, WWDG on APB1/4096, SAME70 WDT on SLCK — all fixed).
- [x] 2.2 `enum class InterruptKind { EarlyWarning }`.
      `enable_interrupt` / `disable_interrupt`.
- [x] 2.3 `irq_numbers() -> std::span<const std::uint32_t>`.

## 3. Compile tests + async + example + HW

- [x] 3.1 New `tests/compile_tests/test_watchdog_api.cpp` exercises
      every method on `nucleo_g071rb` IWDG, `nucleo_f401re` IWDG +
      WWDG, and `same70_xplained` WDT. Registered in
      `tests/compile/contract_smoke.cmake`.
- [x] 3.2 `async::watchdog::wait_for(InterruptKind)` runtime sibling
      (in `runtime/async_watchdog.hpp` + `runtime/watchdog_event.hpp`)
      + compile test in `test_async_peripherals.cpp` and
      `test_watchdog_api.cpp`.
- [x] 3.3 `examples/watchdog_probe_complete/`: targets every
      foundational board and exercises every Phase 1-2 lever. Builds
      clean on `nucleo_f401re` and `same70_xplained` (G071 is blocked
      by pre-existing `interrupt_bridge.cpp` USART2 issue, unrelated
      to this change).
- [ ] 3.4 SAME70 / G0 / F4 spot-check: 60 s of refresh + intentional
      late-refresh triggering early-warning. Deferred to follow-up
      `validate-watchdog-coverage-on-3-boards`.
- [ ] 3.5 Update `docs/SUPPORT_MATRIX.md` `watchdog` row. Deferred to
      the hardware-validation follow-up.

## 4. Documentation + follow-ups

- [x] 4.1 `docs/WATCHDOG.md` — model, IWDG vs WWDG vs SAME70 WDT
      comparison, window-mode recipe, early-warning recipe, async
      wiring, modm-style migration table.
- [x] 4.2 Cross-link from `docs/ASYNC.md` (new "Watchdog" subsection).
