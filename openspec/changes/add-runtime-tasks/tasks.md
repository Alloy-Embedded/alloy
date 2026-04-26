# Tasks: Runtime Tasks (Cooperative Coroutine Scheduler)

Each phase below leaves the tree in a working state and is independently
mergeable. Host-only tests cover every phase that does not need hardware.

## 1. Library skeleton
- [x] 1.1 Code lives in `src/runtime/tasks/` (no nested `include`/`src` split
      since alloy's existing runtime is flat). `scheduler.cpp` is added to
      the HAL static library when `arch != avr`; on host builds, the test
      compiles it directly because alloy-hal is INTERFACE there.
- [x] 1.2 Refuse to enable on `arch=avr` -- the CMakeLists guards both the
      lib source addition and the example `add_subdirectory`. The capability
      spec records the limitation.
- [ ] 1.3 README under `src/runtime/tasks/` (deferred to a follow-up; the
      OpenSpec proposal currently carries the layer description).

## 2. Pool allocator
- [x] 2.1 `pool.hpp`: `FramePool<Slots, BytesPerSlot>` with a 64-bit-word
      bitmap free list. `allocate(bytes)` returns `void*` or `nullptr` (full
      or oversized); `last_oversize_request()` exposes the rejected size so
      the scheduler can distinguish `FrameTooLarge` from `PoolFull`.
- [x] 2.2 Three pool unit tests in `test_tasks.cpp` cover alloc/dealloc
      round-trip, oversize rejection without consuming a slot, and
      capacity exhaustion.

## 3. Task type
- [x] 3.1 `Task` (single `void` flavour for v1; `Task<T>` with non-void
      returns is a follow-up). `TaskPromise` overrides `operator new`/
      `operator delete` to route through the active scheduler's pool via a
      `thread_local` handle set during `spawn`.
- [x] 3.2 `get_return_object_on_allocation_failure()` returns an empty
      `Task` instead of throwing; `spawn` checks `task.valid()` and reports
      `FrameTooLarge` or `PoolFull` per `pool.last_oversize_request()`.
- [x] 3.3 The `test_tasks.cpp` unit suite covers immediate completion,
      one-suspend round-trip, and cancellation observed during `delay`.
      Tests for typed return values land with `Task<T>` in a follow-up.

## 4. Priority and ready queue
- [x] 4.1 `priority.hpp` declares the four levels. The "ready queue" is
      implicit in the scheduler's tick: scan slots, pick the lowest
      priority-index Ready task, FIFO by slot index. O(MaxTasks) per tick
      is fine at the foundational scale (<= 8 tasks); a real
      `priority_queue` is a future optimisation if the budget tightens.
- [x] 4.2 `test_tasks.cpp` covers FIFO within a level (`yield_now` round-robin)
      and priority winning contention (high vs low).

## 5. Scheduler core
- [x] 5.1 `Scheduler<MaxTasks, MaxFrameBytes>` with `spawn(Fn, Priority,
      CancellationToken)`, `tick()` returning `bool`, `run()` looping on
      tick, and `request_stop()`. `spawn` takes a factory rather than a
      pre-built `Task` because the coroutine's `operator new` runs at the
      moment the factory is called -- the pool must be installed around
      that call, not around `spawn`.
- [x] 5.2 The scheduler holds a per-instance `FramePool` and exposes it to
      `TaskPromise` via thread-local pointers (`active_pool`,
      `active_alloc_fn`, `active_free_fn`).
- [x] 5.3 The `test_tasks.cpp` suite covers spawn-pool-exhaustion
      (`PoolFull`), spawn-frame-too-large (`FrameTooLarge`), priority order
      between High and Low tasks, FIFO within Normal.

## 6. Awaiters
- [x] 6.1 v1 ships `delay(Duration)`, `yield_now()`, and `on(Event&)`.
      `until(predicate)`, `any_of`, `all_of` remain follow-ups (more
      surface area, no behavioural blocker for the canonical example).
- [x] 6.2 The host suite covers `delay` against a mock clock,
      `yield_now`, and `on(event)` across three scenarios: the canonical
      ISR-stand-in pattern (signal->wake->resume), the pre-signalled
      shortcut (await_ready returns true, no suspension), and
      cancellation propagation (`Result<void, Cancelled>` after token fires).

## 7. ISR-to-task signaling
- [x] 7.1 `Event` (alloy::tasks::Event) is the v1 signal primitive. Single
      bit, edge-triggered, signal()-from-ISR safe through compiler-only
      atomic_signal_fence (no RMW => no libatomic dependency on Cortex-M0+).
      Event-with-data via SPSC ring buffer is a follow-up; for now the
      producer fills shared state before calling `signal()` and the consumer
      reads it after `co_await on(event)` returns.
- [ ] 7.2 Drop counter (relevant only once SPSC queues land); deferred.
- [ ] 7.3 Host test simulates the producer-consumer pattern within the same
      cooperative scheduler (no separate "ISR thread" needed; the producer
      task is the stand-in). Real ISR validation lands with hardware spot
      checks on the foundational boards.

## 8. Cancellation
- [ ] 8.1 Implement `cancellation_token`. Tokens hold a single atomic flag
      and a parent-link for chaining (cancelling a parent cancels all
      children).
- [ ] 8.2 Awaiters check the token before suspending and on each resume.
      Cancelled awaiters return `Result<T, Cancelled>`.
- [ ] 8.3 Cover with tests: a task cancelled mid-`delay`, mid-`on(event)`,
      and a parent token that propagates to a child task.

## 9. Examples
- [x] 9.1 `examples/tasks_blink_uart`: three coroutines exercising priority
      and `on(event)` together: blink_task (Normal) toggles the LED every
      500 ms; producer_task (Low) signals a shared Event every 1000 ms;
      consumer_task (High) awaits the event and bumps a counter. Builds
      for `nucleo_g071rb` at 4928 B `.text` + 160 B `.data` + 4504 B `.bss`
      = 9592 B total (3.88% flash, 12.63% RAM).
- [ ] 9.2 `examples/tasks_priorities`: deferred until v1 absorbs `on(event)`
      so the example shows real signalling instead of contention against
      a busy loop.
- [ ] 9.3 `examples/tasks_cancellation`: deferred until the token chaining
      contract is implemented (this commit only ships the simple flag).

## 10. Documentation
- [ ] 10.1 `docs/TASKS.md`: user guide -- spawn, await, priority, pool
      sizing recipe, footprint table, cancellation idiom, ISR signalling
      pattern, common gotchas.
- [ ] 10.2 `docs/SUPPORT_MATRIX.md`: add a `tasks` runtime-class entry,
      `representative` once host loopback tests pass, `compile-only` until
      hardware spot-checks land.
- [ ] 10.3 `docs/COOKBOOK.md`: add a tasks section with the canonical
      patterns from the examples.
- [ ] 10.4 Cross-link from `docs/QUICKSTART.md` so users discover the
      capability after `alloy new`.

## 11. Footprint gate
- [ ] 11.1 Add a CI job that builds `examples/tasks_blink_uart` for
      `nucleo_g071rb` and asserts `.text + .rodata + .data + .bss <= 12 KB`.
      Trips the build if a future change pushes the scheduler over budget.

## 12. ESP32 sanity (no IDF helper, just verify the model fits)
- [ ] 12.1 Build `examples/tasks_blink_uart` for `esp32c3_devkitm` and
      `esp32s3_devkitc` in CI; confirm the scheduler compiles against the
      Espressif toolchains. No IDF integration in this change.
