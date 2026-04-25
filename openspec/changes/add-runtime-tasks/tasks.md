# Tasks: Runtime Tasks (Cooperative Coroutine Scheduler)

Each phase below leaves the tree in a working state and is independently
mergeable. Host-only tests cover every phase that does not need hardware.

## 1. Library skeleton
- [ ] 1.1 Create `src/runtime/tasks/{include,src,tests}/` with a CMake
      integration that registers the static library `alloy::tasks` and gates
      its tests behind `ALLOY_BUILD_TESTS=ON AND PROJECT_IS_TOP_LEVEL`.
- [ ] 1.2 Refuse to enable on `arch=avr` -- the avr-gcc toolchain we ship
      pins on does not implement C++20 coroutines. The CMakeLists checks the
      arch and emits a clear `FATAL_ERROR` instead of failing mid-compile.
- [ ] 1.3 Add `src/runtime/tasks/README.md` describing the layer split and
      pointing at this OpenSpec change.

## 2. Pool allocator
- [ ] 2.1 Implement `pool.hpp`: `frame_pool<MaxSlots, MaxBytes>` with a
      bitmap-indexed slot table. `allocate(size)` returns `std::byte*` or
      `nullptr` when full / over-sized. `deallocate(ptr)` clears the slot.
- [ ] 2.2 Cover with `tests/test_pool.cpp`: alloc/dealloc round-trips,
      capacity-exhaustion path, oversize rejection, alignment correctness on
      both 4-byte and 8-byte hosts.

## 3. Task type
- [ ] 3.1 Implement `task.hpp`: `Task<T>` coroutine type plus its
      `promise_type`. Override `operator new`/`operator delete` to route
      through the active `frame_pool`. Provide `final_suspend` machinery so
      completed tasks signal the scheduler.
- [ ] 3.2 Define the `Result<T, Cancelled>` cancellation contract on the
      promise so awaiters can short-circuit when the token is fired.
- [ ] 3.3 Cover with `tests/test_task.cpp`: a coroutine that returns
      immediately, one that suspends once and resumes, one that propagates an
      `int` return, and one that observes cancellation.

## 4. Priority and ready queue
- [ ] 4.1 Implement `priority.hpp`: the four-level enum (`high`, `normal`,
      `low`, `idle`) and `ready_queue` -- four FIFO queues, one per level,
      with O(1) `push` / `pop_highest`.
- [ ] 4.2 Cover with `tests/test_ready_queue.cpp`: a deterministic sequence
      of pushes across levels yields the expected pop order.

## 5. Scheduler core
- [ ] 5.1 Implement `scheduler.hpp`: `scheduler<MaxTasks, MaxFrameBytes>`
      with `spawn(Task<T>&&, priority, cancellation_token = {})` returning
      `Result<task_id, SpawnError>`, `tick()` that drains pending events
      and advances ready coroutines once, and `run()` as the trivial
      `while(!stopped) tick();` loop.
- [ ] 5.2 The scheduler holds a per-instance `frame_pool` and exposes it to
      `Task<T>::promise_type` via a thread-local pointer set across
      `spawn` / resume.
- [ ] 5.3 Cover with `tests/test_scheduler.cpp`: round-robin within a
      priority, priority-up resumption (a `priority::high` task spawned
      from inside `priority::low` runs first), spawn after pool exhaustion
      returns `SpawnError::PoolFull`, spawn of a task whose frame exceeds
      slot returns `SpawnError::FrameTooLarge`.

## 6. Awaiters
- [ ] 6.1 Implement `awaiters.hpp`:
      - `delay(duration)` -- suspends until a SysTick-derived deadline.
      - `yield_now()` -- returns to scheduler, re-queues at end of own
        priority level.
      - `on(event)` -- suspends until the runtime-async-model event fires.
      - `until(predicate)` -- suspends; the scheduler polls `predicate()`
        on each tick and resumes when true.
      - `any_of(awaitables...)` / `all_of(awaitables...)` -- compose awaits.
- [ ] 6.2 Cover each awaiter with focused unit tests over a host
      time-source mock.

## 7. ISR-to-task signaling
- [ ] 7.1 Implement a lock-free SPSC queue keyed to a runtime event.
      Producer is wait-free; consumer is the scheduler.
- [ ] 7.2 Surface a drop counter the user can read after the fact so
      production code can detect saturated queues.
- [ ] 7.3 Test with a host-side simulator that posts from a separate
      "ISR" thread and asserts the consumer wakes within one tick.

## 8. Cancellation
- [ ] 8.1 Implement `cancellation_token`. Tokens hold a single atomic flag
      and a parent-link for chaining (cancelling a parent cancels all
      children).
- [ ] 8.2 Awaiters check the token before suspending and on each resume.
      Cancelled awaiters return `Result<T, Cancelled>`.
- [ ] 8.3 Cover with tests: a task cancelled mid-`delay`, mid-`on(event)`,
      and a parent token that propagates to a child task.

## 9. Examples
- [ ] 9.1 `examples/tasks_blink_uart`: blink at 500 ms + UART character
      echo, side by side. The canonical "two flows" demonstration.
- [ ] 9.2 `examples/tasks_priorities`: three tasks at three priorities;
      logs prove the highest-priority task wins every contention.
- [ ] 9.3 `examples/tasks_cancellation`: a parent task spawns two
      children, cancels them on a button press, and joins their results.

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
