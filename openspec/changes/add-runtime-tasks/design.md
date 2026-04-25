# Design: Runtime Tasks (Cooperative Coroutine Scheduler)

## Context
The user-facing problem is "give me a way to write a few independent flows of work
without dragging FreeRTOS in." The architectural constraint is "do not weaken
`blocking-only-path` -- code that does not opt in pays zero." The C++ idiom that
solves both at once is a coroutine scheduler with a single stack and pooled frames.

This document records the decisions that shape the public API and explains why each
one was preferred over the alternatives.

## Goals / Non-Goals

Goals:
- Application code that wants concurrency reads top-to-bottom (`co_await delay(...)`,
  `co_await uart_read(...)`), not as state machines spread across files.
- Zero heap allocation in steady state. All coroutine frames live in user-declared
  pools sized at compile time.
- Predictable footprint: ~3-4 KB code, ~150-400 B per active task, runs on a
  Cortex-M0+ with 36 KB RAM.
- Static priority scheduling so the user can keep one critical task ahead of the
  background work without inventing their own ready queue.
- No preemption. Tasks switch only at `co_await` points the user can see in the
  source. No mutex, no priority-inheritance plumbing.

Non-Goals:
- Real-time guarantees beyond "highest-priority ready task runs at the next yield".
  Hard deadlines belong to ISRs, which already preempt the scheduler.
- Multi-core / SMP. One scheduler runs on one CPU thread.
- Mutex/semaphore/event-group primitives in the FreeRTOS sense. Cooperative
  scheduling makes them unnecessary; awaiters on shared events cover the cases
  that look like signaling.
- A coroutine-aware HAL. The HAL stays blocking-first. Coroutine adapters live
  outside the HAL surface.
- AVR support. The avr-gcc toolchain we ship pins on does not implement C++20
  coroutines. The spec records this and the build refuses to enable
  `runtime-tasks` on `arch=avr` rather than failing mid-compile.

## Decisions

### Decision: Coroutines as the primary API; reactor `Task` is a follow-up
- Rationale: matches the project's "Modern C++23" identity, gives the user
  linear-looking code, and aligns with where the broader ecosystem is heading
  (Embassy in Rust, Senders/Receivers in WG21 P2300, NVIDIA stdexec).
- Trade-off accepted: users who do not yet know `co_await` have a learning curve.
  Mitigated by examples and a short cookbook entry.
- Reactor-style `class Task { virtual void poll(); };` is intentionally deferred.
  If a class of state machines (e.g. ISR-driven debouncers) reads better as a
  reactor, that is a clean follow-up addition; we do not need to design both APIs
  up front.

### Decision: Pool allocation, not heap
- The `Task<T>::promise_type` overrides `operator new`/`delete` to bump-allocate
  from the active scheduler's pool. The pool has a fixed number of slots, each of
  a configurable max size in bytes. Spawning a coroutine whose frame does not fit
  returns `Result<task_id, SpawnError::FrameTooLarge>` -- it never touches `malloc`.
- Rationale: respects the project's no-heap discipline; gives the user a single
  knob (`scheduler<MaxTasks, MaxFrameBytes>`) that turns into a static RAM budget
  the linker map proves out.
- Trade-off: coroutine frame size is not standardized, so the user has to size the
  pool conservatively. The compiler-specific `__builtin_coro_size` (when
  available) lets us emit a `static_assert` for the common cases; the runtime
  fallback is a typed error rather than UB.
- Alternative considered: dynamic heap with allocator hook. Rejected because the
  point of an embedded scheduler is to know your RAM at link time.

### Decision: Four static priorities, no boosting
- `priority::high`, `priority::normal`, `priority::low`, `priority::idle`. Static
  at spawn time, immutable thereafter.
- Scheduler resumes the highest-priority ready task at each yield point. Within a
  priority level, FIFO.
- No priority inheritance, no aging, no boosting. None are needed because tasks
  do not preempt each other -- a `priority::low` task that holds the CPU between
  `co_await` points is well-formed; it just runs to its next yield, then the
  scheduler picks the next ready task. There is no lock to invert priority on.
- Rationale: every additional knob makes the scheduler less obvious. Four levels
  cover the cases users actually have ("critical", "normal", "background",
  "idle-only") and N FIFO queues is ~256 bytes total.
- Trade-off: applications that need fine-grained priorities will outgrow this
  model. Acceptable for v1.

### Decision: Single-threaded scheduler per instance
- Each `scheduler` instance runs on one CPU thread. Period.
- Multi-core users (ESP32 LX6/LX7, future RP2040 dual-core) instantiate one
  scheduler per core. The schedulers do not share state -- if a task needs to
  cross cores, it posts an event through a lock-free queue.
- Rationale: SMP schedulers are where bugs live. Keeping the contract explicitly
  single-threaded means we can promise "no race between tasks" without footnotes.

### Decision: Scheduler is opt-in; default super-loop is unaffected
- Code that does not include the tasks header pays nothing. The scheduler does
  not register interrupts, does not hook the SysTick beyond an opt-in tick
  source, and does not pull `Catch2` or `<chrono>` into the runtime build.
- Rationale: matches `blocking-only-path` discipline. The async adapter in alloy
  has always been optional and this change preserves that.

### Decision: Awaiters use the existing runtime-async-model primitives
- `alloy::tasks::on(event)` resolves through the same `runtime::event` plumbing
  that `runtime::event::completion::wait_for` already uses. The scheduler is the
  user-facing layer; the event/completion runtime stays as the kernel.
- Rationale: avoids parallel async stacks. One runtime, two facades.

### Decision: Cancellation through a typed token, not exceptions
- `cancellation_token` is a small RAII handle. `scheduler::spawn(task, prio,
  token)` registers the token; calling `token.request_cancel()` makes the task's
  next awaiter return `Result<T, Cancelled>`. The task drops back to its caller,
  unwinding RAII as usual.
- Rationale: `-fno-exceptions` is the default on ARM bare-metal in this project.
  Using exceptions for cancellation would force every consumer to flip that.
  Typed Result aligns with the rest of the runtime.

### Decision: ISR-to-task signaling via lock-free SPSC queue
- ISRs that need to wake a task post into a fixed-capacity SPSC queue tied to an
  event. The scheduler drains the queue on its next pass and resumes any task
  awaiting the event.
- Rationale: ISRs cannot safely call coroutine resume directly (resume runs user
  code). Posting to a queue is wait-free for the producer.
- Trade-off: queue capacity is a configuration knob; overflow drops the event
  and increments a counter the user can inspect. Spec requires the counter to
  exist so production code can detect drops.

## Risks / Trade-offs

- **Pool sizing.** A user under-sizes `MaxFrameBytes`, spawn returns
  `FrameTooLarge`, and they have to either bump the pool or refactor their
  coroutine. Mitigation: docs include a sizing recipe (`-fno-exceptions`,
  `-Os`, then `__builtin_coro_size` on the largest task) and the error message
  prints the actual frame size next to the configured slot size.
- **A misbehaving task starves the rest.** Cooperative => true. Mitigation:
  scheduler exposes a per-task "time since last yield" counter; users can wire
  it to a watchdog or simply log warnings. We do not auto-yield because that
  would reintroduce preemption hazards.
- **Compile-time tax.** Coroutines instantiate templates aggressively. On
  Cortex-M0+ with `-Os` the smoke app comes in at ~3.5 KB, but real apps that
  spawn many task types will see more. Mitigation: docs recommend extracting
  shared awaiter machinery to non-template helpers.
- **Coroutine ABI portability.** The coroutine ABI is stable in GCC and Clang
  but not bit-identical between them. We do not promise binary compatibility
  across compilers; only source. Spec states this explicitly.

## Migration Plan
This is additive. No in-tree code uses the new headers; users opt in by
including `<alloy/tasks/scheduler.hpp>`. The first internal users will be the
new task-based examples. Existing examples stay super-loop and continue to work.

## Open Questions
- **`any_of` / `all_of` shipping in v1?** Useful for "wait for first response,
  whichever wires fastest" patterns common in protocols. They add ~300 bytes of
  template machinery. Lean: yes, in v1; cheap and lets the master/slave Modbus
  code use them.
- **Should `delay(0ms)` be a hard sentinel for `yield_now()`?** Some schedulers
  treat `delay(0)` as an immediate yield; others treat it as "complete on next
  tick". Lean: explicit `yield_now()` exists; `delay(0)` is a separate concept
  meaning "clamp to next tick". Document, do not overload.
- **Wake granularity for `delay`.** A 1 ms tick is plenty for most apps but
  expensive for ultra-low-power sleep. Lean: scheduler asks the runtime for the
  next deadline and sleeps the CPU until then; SysTick frequency stays
  configurable per-board.
