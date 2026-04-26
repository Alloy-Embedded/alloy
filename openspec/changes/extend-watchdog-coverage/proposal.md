# Extend Watchdog Coverage To Match Published Descriptor Surface

## Why

`WatchdogSemanticTraits<P>` publishes config / control / status /
window / reload registers, key + prescaler + timeout + window
fields, status flags (timeout / error / reload-update /
prescaler-update / window-update), reset-enable, restart, plus
`kHasWindow`, `kEarlyWarningInterruptEnableField`, kernel-clock
selector, and `kIrqNumbers`.

The runtime currently consumes ~30%: HAL exposes
`enable / disable / refresh / configure(Config)`. Window mode,
early-warning interrupt, status-flag observation, prescaler
control, kernel-clock source — none reachable today.

modm covers Watchdog with the full early-warning + window-mode
surface. Alloy already publishes the data needed; this change is
plumbing.

## What Changes

### `src/hal/watchdog.hpp` — extended HAL surface (additive only)

- **Window mode** (gated on `kHasWindow`):
  `set_window(std::uint16_t cycles)`,
  `enable_window_mode(bool)`.
- **Early-warning interrupt** (gated on
  `kEarlyWarningInterruptEnableField.valid`):
  `enable_early_warning(std::uint16_t cycles_before_timeout)`,
  `early_warning_pending() -> bool`,
  `clear_early_warning()`.
- **Status flags** (each gated on the matching status field):
  `timeout_occurred() -> bool`,
  `prescaler_update_in_progress() -> bool`,
  `reload_update_in_progress() -> bool`,
  `window_update_in_progress() -> bool`,
  `error() -> bool`.
- **Reset enable / disable**:
  `set_reset_on_timeout(bool)` — when true, the watchdog drives
  a system reset on timeout; when false, it only fires the
  early-warning interrupt (where supported).
- **Kernel clock source** (gated):
  `set_kernel_clock_source(KernelClockSource)`.
- **Typed interrupt enable**:
  `enum class InterruptKind { EarlyWarning }`.
  `enable_interrupt(InterruptKind)` /
  `disable_interrupt(InterruptKind)`.
- **NVIC vector lookup**: `irq_numbers() ->
  std::span<const std::uint32_t>`.
- **Async sibling**: `async::watchdog::wait_for(InterruptKind)` —
  for coroutines that want to react to early-warning before reset.

### `examples/watchdog_probe_complete/`

Targets `nucleo_g071rb` IWDG + WWDG. IWDG runs at 100 ms timeout
with 50 ms window; WWDG fires early-warning interrupt 10 ms
before reset. Async task in the early-warning handler dumps
crash context to a backup register before the reset.

### Docs

`docs/WATCHDOG.md` — model, IWDG vs WWDG comparison, window-mode
recipe, early-warning recipe, refresh discipline, async wiring,
modm migration table.

## What Does NOT Change

- Existing watchdog API unchanged. Additive only.
- Tier stays `foundational`.

## Out of Scope (Follow-Up Changes)

- ESP32 / RP2040 watchdog parity — gated on alloy-codegen.
- Hardware spot-checks → `validate-watchdog-coverage-on-3-boards`.

## Alternatives Considered

`enable_early_warning(u16 cycles)` could split into separate
`set_early_warning_cycles + enable_early_warning(bool)` — rejected
for ergonomics; one call covers the common case.
