# public-hal-api Spec Delta: Watchdog Coverage Extension

## ADDED Requirements

### Requirement: Watchdog HAL SHALL Expose Window Mode And Early-Warning Interrupt Per Capability

The `alloy::hal::watchdog::handle<P>` MUST expose
`set_window(std::uint16_t cycles)` and
`enable_window_mode(bool)` whenever `kHasWindow`. The HAL MUST
expose `enable_early_warning(std::uint16_t cycles_before_timeout)`,
`early_warning_pending() -> bool`, and `clear_early_warning()`
whenever `kEarlyWarningInterruptEnableField.valid`. Backends
without the capability MUST return `core::ErrorCode::NotSupported`.

#### Scenario: STM32 WWDG fires early-warning interrupt before reset

- **WHEN** an application calls
  `wdg.enable_early_warning(50)` on WWDG with a 1 ms window
- **THEN** the early-warning interrupt fires 50 cycles before the
  reset would otherwise occur

#### Scenario: IWDG (no early-warning) returns NotSupported

- **WHEN** an application calls `wdg.enable_early_warning(50)` on
  STM32 IWDG (which lacks the early-warning interrupt)
- **THEN** the call returns `core::ErrorCode::NotSupported`

### Requirement: Watchdog HAL SHALL Expose Status Flags And Reset-On-Timeout Toggle

The HAL MUST expose, each gated on the matching descriptor
status field:

- `timeout_occurred() -> bool`
- `prescaler_update_in_progress() -> bool`
- `reload_update_in_progress() -> bool`
- `window_update_in_progress() -> bool`
- `error() -> bool`

The HAL MUST also expose `set_reset_on_timeout(bool)` (gated on
`kResetEnableField.valid`) so applications can run the watchdog
as a free-running timer without driving the system reset.

#### Scenario: Watchdog as free-running timer for early-warning only

- **WHEN** an application calls `wdg.set_reset_on_timeout(false)`
  then `wdg.enable_early_warning(...)` on a peripheral that
  supports both
- **THEN** subsequent timeouts fire the early-warning interrupt
  without resetting the system

### Requirement: Watchdog HAL SHALL Expose Typed Interrupt Setter And IRQ Number List

The HAL MUST expose
`enum class InterruptKind { EarlyWarning }` plus
`enable_interrupt(InterruptKind)` /
`disable_interrupt(InterruptKind)` (each kind gated on its IE
field), and `irq_numbers() -> std::span<const std::uint32_t>`.

#### Scenario: WWDG IRQ number is reachable for ISR install

- **WHEN** an application reads `decltype(wdg)::irq_numbers()` on
  STM32 WWDG
- **THEN** the span has size 1 and contains the WWDG NVIC IRQ id

### Requirement: Async Watchdog Adapter SHALL Add wait_for(InterruptKind)

The runtime `async::watchdog` namespace MUST expose
`wait_for<P>(handle, InterruptKind kind)` so a coroutine can
`co_await` an EarlyWarning event before the inevitable reset.
Documentation MUST make explicit that this is for crash-state
capture, not "recovery".

#### Scenario: Coroutine captures crash state on early warning

- **WHEN** a task awaits
  `async::watchdog::wait_for<WWDG>(wdg, InterruptKind::EarlyWarning)`
  with reset-on-timeout enabled
- **THEN** the task resumes when WWDG's EWI fires
- **AND** the task's body has a bounded number of cycles to dump
  state before the watchdog ultimately resets the system
