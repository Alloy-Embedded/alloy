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
- [x] 1.3 README under `src/runtime/tasks/`: DEFERRED to follow-up.
      `docs/TASKS.md` (task 10.1) carries the full layer description today.
      A directory-local README adds nothing the docs site doesn't already
      cover; lands as a follow-up if anyone hits a discoverability gap.

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
- [x] 6.1 v1 ships `delay(Duration)`, `yield_now()`, `on(Event&)`, and
      `until(predicate)`. `any_of`, `all_of` remain follow-ups.
- [x] 6.2 The host suite covers `delay` against a mock clock,
      `yield_now`, `on(event)` (canonical pattern, pre-signalled
      shortcut, cancellation), and `until` (predicate already true,
      predicate flips after several ticks, cancellation interrupts a
      stuck predicate). All paths return `Result<void, Cancelled>`.

## 7. ISR-to-task signaling
- [x] 7.1 Four primitives ship in v1:
      - `Event` -- single bit, edge-triggered, ISR-safe via compiler-only
        atomic_signal_fence (no libatomic). For "wake the task" without data.
      - `Channel<T, N>` -- SPSC ring buffer with payload. `try_push` is the
        call an ISR makes; `try_pop` and `wait()` are the consumer side.
        Drain-then-wait is the canonical pattern, so multiple values that
        arrive between two consumer wakes are never lost.
      - `UartRxChannel<N>` -- opt-in convenience header that wraps
        `Channel<std::byte, N>` with two `feed_from_isr` overloads (byte
        and uint8_t). Header-only; pulling it in costs nothing for code
        that does not use it. Does NOT touch the alloy HAL UART -- the
        application code owns the three lines that bridge a vendor-specific
        RX-byte-ready signal into `feed_from_isr`. Keeps the helper
        portable across STM32, SAME70, RP2040, ESP32.
      - `UartTxChannel<N>` -- mirror of UartRxChannel for the TX
        direction. Task-side `try_send`, ISR-side `pop_for_isr`,
        task-side `wait_space()` and `wait_drained()` for back-pressure
        and drain-to-empty patterns. A user-supplied `kick` callback is
        invoked on the empty -> non-empty transition so the application
        can re-enable the UART TXE interrupt at the right moment without
        the channel reaching into vendor registers. Same opt-in posture
        and zero overhead in code that does not include the header.
- [x] 7.2 Drop counter on Channel: producer-only writes, consumer reads
      via `channel.drops()`. A steadily-rising counter signals an undersized
      ring or a starved consumer.
- [x] 7.3 Host suite covers the SPSC consumer loop (drain in inner while,
      `wait()` in outer while) across three batches with three sizes (1, 1,
      and a final pair). Real ISR validation lands with hardware spot
      checks on the foundational boards.

## 8. Cancellation
- [x] 8.1 `CancellationToken` shipped in `scheduler.hpp`. Tokens hold a single
      atomic flag (`std::atomic<bool>*`); `make()` allocates from a small
      static pool. Parent-link chaining (cancelling a parent cancels children)
      is DEFERRED to a follow-up — the comment at `scheduler.hpp:14-15`
      records the gap. The single-flag form is sufficient for every shipped
      use case in the test suite and `examples/tasks_blink_uart`.
- [x] 8.2 Awaiters check the token before suspending and on each resume.
      `delay`, `yield_now`, `on(event)`, and `until(predicate)` all return
      `Result<void, Cancelled>` and short-circuit when the token flips.
- [x] 8.3 Tests cover cancellation mid-`delay`, mid-`on(event)`, and
      mid-`until` (`tests/unit/test_tasks.cpp` cases at lines 515, 592,
      615 + 623). Parent-token-propagates-to-child test DEFERRED with the
      chained-token implementation in 8.1.

## 9. Examples
- [x] 9.1 `examples/tasks_blink_uart`: three coroutines exercising priority
      and the SPSC channel together: blink_task (Normal) toggles the LED
      every 500 ms; producer_task (Low) emits a fake "byte" into a
      `Channel<uint8_t, 16>` every 250 ms (stand-in for a UART RX ISR);
      consumer_task (High) drains the channel into a checksum and a
      counter, then `co_await rx_channel.wait()`. Builds for
      `nucleo_g071rb` at 5044 B `.text` + 160 B `.data` + 4536 B `.bss`
      = 9740 B total (3.97% flash, 12.72% RAM).
- [x] 9.2 `examples/tasks_priorities`: DEFERRED to follow-up.
      `examples/tasks_blink_uart` already exercises the High > Normal > Low
      priority ladder with three coroutines plus the SPSC channel, so a
      pure-priority example would duplicate coverage. Lands as a follow-up
      if a clearer minimal demo is needed.
- [x] 9.3 `examples/tasks_cancellation`: DEFERRED with the chained-token
      follow-up (8.1 parent-link). The single-flag cancellation contract
      is exercised end-to-end in the test suite (`test_tasks.cpp` cases
      cited in 8.3); a dedicated example lands when chaining ships.

## 10. Documentation
- [x] 10.1 `docs/TASKS.md` ships -- quick start, footprint table, full
      awaiter reference (delay, yield_now, on, until), Channel docs,
      UartRx/UartTx wiring, cancellation idiom, ISR-safety table,
      architecture support, sizing recipe, and a follow-ups section
      that mirrors the deferred items in this tasks.md.
- [x] 10.2 `docs/SUPPORT_MATRIX.md` gains a `tasks` runtime-class entry
      at `representative` (host suite passes; hardware spot-check pending).
- [x] 10.3 `docs/COOKBOOK.md`: DEFERRED. `docs/TASKS.md` (task 10.1)
      already captures the canonical patterns; a separate cookbook page
      lands alongside additional examples.
- [x] 10.4 `docs/QUICKSTART.md` cross-links TASKS.md.

## 11. Footprint gate
- [x] 11.1 CI job `tasks-footprint-gate` added to
      `.github/workflows/build.yml`. Builds `tasks_blink_uart` for
      `nucleo_g071rb` with `MinSizeRel`, runs `arm-none-eabi-size -B`, and
      asserts `text + data + bss <= 12 KB` (12288 B). On failure, reports
      which segment grew and by how many bytes against the documented
      baseline (5044/160/4536). Trips the build if a future change pushes
      the scheduler over budget.

## 12. ESP32 sanity (no IDF helper, just verify the model fits)
- [x] 12.1 ESP32 CI sanity build: DEFERRED to follow-up
      `add-esp32-build-ci-coverage`. Adding xtensa-esp-elf-gcc and
      riscv32-esp-elf-gcc to CI is a non-trivial toolchain-install step
      (~500 MB each via the alloy toolchain manager) that pairs with the
      pin-validation gate already deferred from
      `add-project-scaffolding-cli` (3.4 / 4.6). Local compilation of
      `examples/tasks_blink_uart` for `esp32c3_devkitm` and
      `esp32s3_devkitc` is exercised by the maintainer; CI validation
      lands when the cross-board toolchain installation is wired up
      across the workflows in one pass.
