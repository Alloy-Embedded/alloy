# Alloy Watchdog HAL

`alloy::hal::watchdog::handle<P>` is the typed, descriptor-backed watchdog
abstraction. Every method is capability-gated via `if constexpr` against
the descriptor's published `WatchdogSemanticTraits<P>` field set —
methods whose underlying field is not published return
`core::ErrorCode::NotSupported`.

The watchdog is a safety device. **The early-warning interrupt is for
crash-state capture only** — recovery from a watchdog timeout defeats
the watchdog's safety guarantee. Do not use it as a soft-recovery path.

---

## Quick start

```cpp
#include "hal/watchdog.hpp"

auto wdg = alloy::hal::watchdog::open<alloy::device::PeripheralId::IWDG>();

// One-time refresh before main loop kicks the timer.
wdg.refresh();

while (true) {
    do_work();
    wdg.refresh();   // must hit before timeout
}
```

---

## IWDG vs WWDG (STM32) vs WDT (SAME70)

| Feature | STM32 IWDG | STM32 WWDG | SAME70 WDT |
|---------|-----------|-----------|------------|
| Independent clock | LSI (~32 kHz) | APB1 / 4096 | SLCK (~32 kHz) |
| Software disable | ✗ | ✗ | ✓ (`WDDIS`) |
| Window mode | ✗ | ✓ | ✓ |
| Early-warning IRQ | ✗ | ✓ (EWI) | ✓ (WDFIEN) |
| Reset enable bit | ✗ (always reset) | ✗ (always reset) | ✓ (`WDRSTEN`) |
| Refresh sequence | write `0xAAAA` to KR | write to T[6:0] | write `0xA5` key + WDRSTT |

The HAL exposes the high-level intent; the field gate determines which
underlying register / sequence gets touched.

---

## Phase 1: window mode

```cpp
wdg.set_window(0x40);              // raw cycles
wdg.enable_window_mode(true);      // keep the window value active
wdg.enable_window_mode(false);     // bypass — write window = max value
```

Refreshes earlier than the window value trigger an immediate reset on
STM32 WWDG; on SAME70 WDT the window is the lower bound of the legal
refresh interval. **The HAL does not enforce timing** — that's the
application's job.

`set_window(cycles)` returns `NotSupported` on backends with
`kHasWindow=false` (STM32 IWDG).

## Phase 1: early-warning interrupt

```cpp
wdg.enable_early_warning(50);                      // arm EWI bit
if (wdg.early_warning_pending()) {
    capture_crash_state();                          // dump to backup register
    static_cast<void>(wdg.clear_early_warning());
}
```

`cycles_before_timeout` is a hint — STM32 WWDG and SAME70 WDT fire EWI
at a hardware-fixed position (T[5:0] crosses 0x40 / WDV reaches 0). To
control the effective trigger, set the timeout / window via
`set_window`. Future backends with a separate threshold register may
honour the parameter directly.

STM32 IWDG has no early-warning surface — every method here returns
`NotSupported`.

## Phase 1: status flags + reset-enable

```cpp
wdg.timeout_occurred();                  // SR.WDUNF (SAM) / SR.EWIF (WWDG)
wdg.prescaler_update_in_progress();      // IWDG SR.PVU
wdg.reload_update_in_progress();         // IWDG SR.RVU
wdg.window_update_in_progress();
wdg.error();                             // SAM SR.WDERR

wdg.set_reset_on_timeout(false);         // free-running timer mode (SAM only)
```

`set_reset_on_timeout(false)` is the canonical way to use the watchdog
as a high-resolution timer that fires the early-warning interrupt
without resetting the system. STM32 IWDG / WWDG always reset on
timeout — the HAL returns `NotSupported`.

## Phase 2: typed interrupts + IRQ vector lookup

```cpp
using K = alloy::hal::watchdog::InterruptKind;
wdg.enable_interrupt(K::EarlyWarning);   // alias for enable_early_warning(0)
wdg.disable_interrupt(K::EarlyWarning);

constexpr auto irqs = decltype(wdg)::irq_numbers();
NVIC_EnableIRQ(static_cast<IRQn_Type>(irqs[0]));
```

Currently only `EarlyWarning` is published. Future backends may add
`Reset` (in cases where the reset cause needs an ISR for logging
before the actual reset), `WindowError`, etc.

## Phase 2: kernel clock source (deferred)

`set_kernel_clock_source(KernelClockSource)` exists in the API but
returns `NotSupported` on every current backend. STM32 IWDG runs from
LSI (no selector), STM32 WWDG runs from APB1/4096 (no selector),
SAME70 WDT runs from SLCK (no selector). The method is a forward-
compatible stub; a future device descriptor publishing
`kKernelClockSourceField` would activate it.

---

## Async wiring

```cpp
#include "async.hpp"

// Set up before arming the watchdog.
auto op = alloy::async::watchdog::wait_for<
    alloy::hal::watchdog::InterruptKind::EarlyWarning>(wdg);

// In a coroutine task running below the EWI handler's priority:
op.wait_until<SysTickSource>(time::Deadline::never());

// At this point, EWI has fired. The system WILL reset shortly —
// dump crash context to a backup register and let the reset happen.
backup_register::write(crash_dump_payload());

// Don't try to refresh — that just defeats the watchdog.
```

The vendor watchdog ISR is responsible for calling
`watchdog_event::token<P, InterruptKind::EarlyWarning>::signal()` when
EWI fires.

---

## Migration: from raw register hex (STM32 WWDG)

| Goal | Old (raw register write) | New (HAL call) |
|------|--------------------------|----------------|
| Set window 0x40 | `WWDG->CFR = (WWDG->CFR & ~0x7F) \| 0x40` | `wdg.set_window(0x40)` |
| Arm early-warning IRQ | `WWDG->CFR \|= (1 << 9)` | `wdg.enable_early_warning(0)` |
| Read EWIF | `(WWDG->SR & 1)` | `wdg.early_warning_pending()` |
| Clear EWIF | `WWDG->SR = 0` | `wdg.clear_early_warning()` |

---

## See also

- [`docs/ASYNC.md`](ASYNC.md) — async::watchdog::wait_for + crash-capture pattern
- [`docs/SUPPORT_MATRIX.md`](SUPPORT_MATRIX.md) — per-board watchdog tier
- [`examples/watchdog_probe_complete/`](../examples/watchdog_probe_complete/) — exercises every lever
