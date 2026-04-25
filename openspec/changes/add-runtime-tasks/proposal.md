# Add Runtime Tasks (Cooperative Coroutine Scheduler)

## Why
Application code on top of Alloy today is a single super-loop. That works for one or
two state machines but breaks down the moment a real product needs a blink, a UART
console, a Modbus poll, a sensor sampler, and a button debouncer running side by side.
Users currently have three options and none of them are good:

- write everything as hand-rolled state machines in `main()` (verbose, error-prone),
- pull in FreeRTOS or another full RTOS (doubles the footprint and reintroduces every
  RTOS hazard the project explicitly avoids -- preemption races, priority inversion,
  per-task stacks, mutex hierarchies),
- or invent something per-project.

The maintainer is implementing ESP32 features right now and needs a scheduling model
that lets user code be expressed as a handful of small concurrent flows. The
constraints are explicit: simpler than FreeRTOS, easy, fast, small footprint, safe.

C++20 coroutines plus a single-stack cooperative scheduler hits every constraint:

- **Easy**: code looks linear (`co_await delay(100ms)`), not state-machine spaghetti.
- **Fast**: no full context switch -- scheduling is "resume the next coroutine handle".
- **Small**: ~3-4 KB code + small pool RAM. Verified achievable on a Cortex-M0+.
- **Safe**: no preemption between awaits => no races between tasks, no mutex needed,
  no priority inversion. Stack overflow only possible on the single application stack
  (one analysis target instead of N).

This change introduces that scheduler as a new opt-in runtime capability. Code that
sticks to the existing super-loop pays nothing for it.

## What Changes

### A new capability: `runtime-tasks`
- Spec located at `openspec/specs/runtime-tasks/spec.md`.
- A `Task<T>` coroutine type and a single-stack scheduler that resumes ready tasks
  in priority order.
- Pool-allocated coroutine frames -- no heap allocation in steady state. The user
  declares scheduler capacity at compile time (max tasks, max frame size), and
  spawning a task that does not fit returns a typed error rather than allocating.
- Four priority levels (`high`, `normal`, `low`, `idle`), static at spawn time. The
  scheduler always resumes the highest-priority ready task; within a level, FIFO.
  No priority inheritance (cooperative => no PI possible by construction).
- Awaiters covering the common cases: `alloy::tasks::delay(duration)`,
  `alloy::tasks::yield_now()`, `alloy::tasks::on(event)`,
  `alloy::tasks::until(predicate)`.
- Cancellation through a `cancellation_token` that the user can pass into a spawn
  call; awaiters return `Result<T, Cancelled>` when the token fires.

### Code under `src/runtime/tasks/`
- `task.hpp` -- `Task<T>` promise/handle, awaiter contract.
- `scheduler.hpp` -- `scheduler<MaxTasks, MaxFrameBytes>` with `spawn`, `run`, `tick`.
- `priority.hpp` -- the four-level enum and ready-queue.
- `awaiters.hpp` -- `delay`, `yield_now`, `on`, `until`, `any_of`, `all_of`.
- `pool.hpp` -- the fixed-capacity coroutine frame pool. `operator new`/`delete`
  hooks in `Task::promise_type` route allocation through the active pool; spawning
  a task whose frame exceeds the slot size fails fast at compile time when the
  size is known, otherwise at runtime with a typed error.
- Header-only where it makes sense; small `.cpp` units only for code that does not
  need to be templated.

### Examples and tests
- `examples/tasks_blink_uart` -- two coroutines: a blink at 500 ms and a UART
  character echo. Demonstrates the canonical app shape.
- `examples/tasks_priorities` -- shows static-priority scheduling between three
  tasks, one of which is `priority::high` and starves a `priority::idle` task as
  expected.
- Host tests cover: round-robin within a priority, priority preemption between
  resume points, cancellation during a `delay`, pool exhaustion errors, and a
  fuzz test on the timer wheel.

### What this change does NOT do
- No preemptive scheduling. No multi-stack. No SMP. If the maintainer wants either
  Xtensa LX6 core to run alloy tasks, they spawn one `scheduler` per FreeRTOS task
  (the IDF-managed mode) or one per core stub (the bare-metal mode). The scheduler
  itself is single-threaded by design.
- No ESP-IDF-specific helper. Users who run `scheduler::run()` inside a FreeRTOS
  task write their three lines themselves; a generic helper waits until a real
  ergonomic gap appears.
- No HAL changes. The HAL stays blocking-first. Coroutine adapters that
  `co_await uart.read_async(...)` build on the existing `runtime-async-model`
  primitives. If a future change wants every HAL call to be coroutine-aware, that
  is a separate proposal.

## Impact

- Affected specs:
  - `runtime-tasks` (new capability spec; created in this change)

- Affected code:
  - new tree: `src/runtime/tasks/` plus `examples/tasks_*` and `tests/unit/`.
  - no changes to existing HAL, board, or descriptor surfaces.

- Hard prerequisites:
  - Compiler with C++20 coroutines support. Both the in-tree
    `arm-none-eabi-gcc 13.x` and `xtensa-esp-elf-gcc 13.x` toolchains have
    `__cpp_coroutines`. Verified on the foundational boards. AVR is not
    supported by this capability and the spec records that limitation.

- Out of scope for this change:
  - Multi-stack / preemptive scheduler. Tracked separately if a real-time
    workload ever requires it.
  - ESP-IDF integration helpers.
  - Coroutine-aware HAL surface changes.
  - SMP/multi-core scheduling. Single-threaded per scheduler instance is the
    contract; users who want both ESP32 cores spawn two schedulers.
