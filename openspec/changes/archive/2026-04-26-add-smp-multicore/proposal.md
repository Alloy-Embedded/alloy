# Add SMP / Multi-Core Support

## Why

Two of Alloy's most important target platforms have multiple cores:

- **ESP32** — dual Xtensa LX6 (PRO_CPU + APP_CPU), 240 MHz each
- **RP2040 / RP2350** — dual Cortex-M0+ (core0 + core1)

Today Alloy's coroutine scheduler (`src/runtime/tasks/`) is single-core and cooperative.
All ESP32 and RP2040 examples run only on core0, leaving the second core idle. This is
a significant waste of the most common dual-core MCUs on the market.

More critically, the existing `Event`, `Channel`, and `CancellationToken` primitives use
`compiler_barrier`-only fencing (`atomic_signal_fence`). This is correct and sufficient
on single-core Cortex-M (where ISR preempts the task on the same core). On dual-core
platforms, a task on core1 writing to a `Channel` that a task on core0 reads requires
a full memory barrier (`atomic_thread_fence`) to prevent the other core from observing
stale data. Without this, the existing primitives are **unsound** on SMP.

This change:
1. Fixes the memory ordering of `Event`, `Channel`, and `CancellationToken` for SMP
   correctness without regressing single-core performance.
2. Adds a `core_affinity` parameter to `Scheduler::spawn` so tasks can be pinned to
   a specific core.
3. Adds a second scheduler instance and a startup hook for the second core on ESP32
   and RP2040.
4. Adds a cross-core message queue (`CrossCoreChannel<T, N>`) for safe inter-core
   communication.

## What Changes

### Memory ordering fix (`src/runtime/`)

`event.hpp`, `channel.hpp`, `cancellation_token.hpp`: audit and fix all atomic
operations:
- Single-core paths keep `std::memory_order_relaxed` + `atomic_signal_fence` where
  sufficient.
- Paths that may communicate across cores use `std::memory_order_release` (producer)
  and `std::memory_order_acquire` (consumer).
- A `ALLOY_SINGLE_CORE` compile-time flag (set by single-core platforms: STM32, SAME70)
  keeps the existing `signal_fence`-only path as before — zero overhead on single-core.

### `CrossCoreChannel<T, N>` (`src/runtime/cross_core_channel.hpp`)

Lock-free SPSC ring buffer with acquire/release memory ordering on all accesses.
Suitable for one producer core and one consumer core. `try_push` / `try_pop` are
ISR-safe and cross-core-safe. Distinct from the existing `Channel<T, N>` which remains
ISR→task on the same core.

### `Scheduler` multi-core extension

`spawn(fn, priority, affinity, token)` — adds `CoreAffinity` parameter:
- `CoreAffinity::Any` — scheduler runs task on the core that picks it up (round-robin
  between schedulers).
- `CoreAffinity::Core0` / `CoreAffinity::Core1` — task runs only on the pinned core.

A `SharedScheduler<MaxTasks, MaxFrameBytes>` variant holds two `Scheduler` instances
(one per core) and a shared ready queue protected by a spinlock (TAS on a byte). The
shared queue is only for `Any`-affinity tasks. Core-pinned tasks go directly into the
per-core scheduler.

### ESP32 second core startup

`boards/esp32_devkit/board.cpp`: add `board::start_app_cpu(fn)` — starts the APP_CPU
(core1) via the ESP32 ROM `call_start_cpu1` entry point. The APP_CPU runs a minimal
startup sequence (set WINDOWBASE/WINDOWSTART, set stack pointer from a second stack
buffer) then calls `fn`. `board::start_app_cpu` takes a function pointer so the
application decides what runs on core1.

`examples/esp32_dual_core/`: core0 runs the LED blink scheduler. core1 runs a UART
echo scheduler. Both communicate via `CrossCoreChannel<char, 64>`. Demonstrates
independent schedulers on each core with inter-core messaging.

### RP2040 second core startup

`boards/raspberry_pi_pico/board.cpp`: add `board::launch_core1(fn)` — uses the RP2040
multicore launch sequence (FIFO push/pop handshake via `SIO_FIFO_WR` / `SIO_FIFO_RD`).
`examples/rp2040_dual_core/`: same pattern as ESP32 dual-core example.

### `SharedEvent` for cross-core ISR→task signaling

`src/runtime/shared_event.hpp`: `SharedEvent` — like `Event` but with full acquire/
release memory ordering. Used when an ISR on core0 needs to wake a task on core1.
Existing `Event` remains unchanged and is for same-core ISR→task only. The type names
encode the intended usage, preventing silent misuse.

## What Does NOT Change

- Single-core platforms (STM32, SAME70, nRF52, AVR) — `ALLOY_SINGLE_CORE` keeps the
  existing `atomic_signal_fence` path. No performance regression.
- The existing `Scheduler` API — `start_app_cpu`/`launch_core1` are additive board-
  level functions. Single-core applications do not need them.
- The zero-overhead gate — the `ALLOY_SINGLE_CORE` path is what the gate tests. The
  SMP path is only compiled in when `ALLOY_SMP=ON` is set by the platform cmake.

## Alternatives Considered

**FreeRTOS SMP port:** FreeRTOS has SMP support for ESP32 and RP2040. Using it would
mean adopting FreeRTOS task management, heap allocation, and RTOS concepts that Alloy
explicitly avoids. The cooperative scheduler is the right model; it just needs to be
extended to two cores.

**Single shared scheduler with a mutex:** A mutex on the ready queue introduces priority
inversion and blocking — incompatible with the cooperative model. The SPSC affinity
split (per-core scheduler + shared queue for `Any` tasks) avoids all mutual exclusion
on the fast path.
