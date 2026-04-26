# Tasks: SMP / Multi-Core Support

Phases 1–3 are host-testable (memory ordering analysis + unit tests). Phases 4–6
require ESP32 or RP2040 hardware.

## 1. Memory ordering audit and fix

- [ ] 1.1 Add `ALLOY_SINGLE_CORE` CMake option: set automatically by single-core
      platform cmake files (stm32*.cmake, same70.cmake, nrf52.cmake, avr-da.cmake).
      Default ON for all existing platforms. Dual-core platforms (esp32.cmake,
      rp2040.cmake) set it OFF.
- [ ] 1.2 Audit `src/runtime/event.hpp`: replace `atomic_signal_fence(memory_order_seq_cst)`
      with `#if ALLOY_SINGLE_CORE ... atomic_signal_fence ... #else ...
      atomic_thread_fence(memory_order_seq_cst) #endif`. Verify no single-core
      regression via the zero-overhead assembly gate.
- [ ] 1.3 Same audit for `src/runtime/tasks/channel.hpp` and
      `src/runtime/tasks/cancellation_token.hpp`. All `load`/`store` on shared
      atomic state use `acquire`/`release` under SMP.
- [ ] 1.4 `tests/unit/test_memory_ordering.cpp`: host-level reasoning test. Uses
      `std::thread` to simulate two cores. Verifies `Channel<T,N>` never delivers
      stale data under concurrent push/pop (Helgrind / TSan annotation).

## 2. CrossCoreChannel

- [ ] 2.1 Create `src/runtime/cross_core_channel.hpp`: `CrossCoreChannel<T, N>` SPSC
      ring with full `acquire`/`release` on all loads/stores. Always SMP-safe
      (unlike `Channel` which is single-core optimized).
- [ ] 2.2 `try_push` / `try_pop` / `size()` / `empty()` API mirroring `Channel`.
- [ ] 2.3 `tests/unit/test_cross_core_channel.cpp`: `std::thread` producer + consumer,
      verify ordering under TSan. 1M iterations, verify no drops.

## 3. SharedEvent and SharedScheduler

- [ ] 3.1 Create `src/runtime/shared_event.hpp`: `SharedEvent` — `Event` with
      acquire/release ordering. Documented: "use when signaling from a different core".
- [ ] 3.2 Create `src/runtime/tasks/shared_scheduler.hpp`: `SharedScheduler<N, B>`.
      Two internal `Scheduler` instances (one per core). Shared ready queue for
      `CoreAffinity::Any` tasks, protected by a TAS spinlock (byte-sized, cache-line
      padded to prevent false sharing).
- [ ] 3.3 `spawn(fn, priority, CoreAffinity::Core0|Core1|Any, token)` extended API.
      Single-core `Scheduler` ignores the affinity parameter.
- [ ] 3.4 `tests/unit/test_shared_scheduler.cpp`: host-level test. Two `std::thread`s
      each calling `scheduler.tick()` on their core index. Verify Core0-pinned tasks
      only run on thread 0, Core1-pinned on thread 1, Any tasks run on either.

## 4. ESP32 second core startup

- [ ] 4.1 `boards/esp32_devkit/board.hpp`: declare `board::start_app_cpu(void(*fn)())`.
- [ ] 4.2 `boards/esp32_devkit/board.cpp`: implement `start_app_cpu`. Write `fn` and
      stack pointer to the inter-core mailbox registers; APP_CPU ROM entry
      (`call_start_cpu1`) reads them and jumps to `fn`.
- [ ] 4.3 Add second core stack buffer (8 KB, `.bss` section) and linker script
      annotation. Startup sequence mirrors `startup.S` window init + stack setup.
- [ ] 4.4 `examples/esp32_dual_core/`: core0 runs blink scheduler at 1 Hz.
      core1 runs a UART echo scheduler. `CrossCoreChannel<char, 64>` carries echo
      data from core1 → core0 UART TX task. Verify both cores active concurrently.
- [ ] 4.5 Hardware spot-check: run `esp32_dual_core` for 60 seconds. Verify no watchdog
      reset, LED blinks at 1 Hz, UART echoes correctly.

## 5. RP2040 second core startup

- [ ] 5.1 `boards/raspberry_pi_pico/board.hpp`: declare `board::launch_core1(void(*fn)())`.
- [ ] 5.2 `boards/raspberry_pi_pico/board.cpp`: implement the RP2040 multicore
      launch sequence via SIO_FIFO (5-step push/pop handshake per SDK spec).
- [ ] 5.3 `examples/rp2040_dual_core/`: same pattern as ESP32 dual-core. Blink on
      core0, UART echo on core1. CrossCoreChannel between them.
- [ ] 5.4 Hardware spot-check: same criteria as ESP32.

## 6. Documentation and CI

- [ ] 6.1 `docs/SMP.md`: multi-core programming guide. Memory ordering rationale,
      `ALLOY_SINGLE_CORE` flag, `CrossCoreChannel` vs `Channel`, `SharedEvent` vs
      `Event`, `SharedScheduler` affinity model, ESP32 and RP2040 startup guide,
      common pitfalls (false sharing, spinlock priority inversion, stack sizing).
- [ ] 6.2 `docs/SUPPORT_MATRIX.md`: add `smp` row. ESP32 = `hardware`.
      RP2040 = `hardware`. All single-core platforms = `n/a`.
- [ ] 6.3 Add `tsan-channel` CI job: compiles and runs `test_cross_core_channel` and
      `test_shared_scheduler` under `-fsanitize=thread`. Fails on any data race.
      Runs on every PR touching `src/runtime/`.
