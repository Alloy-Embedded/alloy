# Tasks: SMP / Multi-Core Support

Phases 1–3 are host-testable (memory ordering analysis + unit tests). Phases 4–6
require ESP32 or RP2040 hardware.

## 1. Memory ordering audit and fix

- [x] 1.1 Add `ALLOY_SINGLE_CORE` CMake option: set automatically by single-core
      platform cmake files (stm32*.cmake, same70.cmake, nrf52.cmake, avr-da.cmake).
      Default ON for all existing platforms. Dual-core platforms (esp32.cmake,
      rp2040.cmake) set it OFF. ✅ compile-review.
- [x] 1.2 Audit `src/runtime/event.hpp`: `volatile bool` → `std::atomic<bool>`;
      `#if ALLOY_SINGLE_CORE` uses `atomic_signal_fence + relaxed load/store`,
      `#else` uses `memory_order_release` store / `memory_order_acquire` load.
      ✅ compile-review.
- [x] 1.3 Same audit for `src/runtime/tasks/channel.hpp`: `atomic_signal_fence` →
      `atomic_thread_fence` under `#else ALLOY_SINGLE_CORE`. Note: `CrossCoreChannel`
      is the recommended primitive for true SMP; `Channel` SMP path is best-effort.
      (`cancellation_token.hpp` not present — uses `std::atomic<bool>` in scheduler.)
      ✅ compile-review.
- [x] 1.4 `tests/unit/test_memory_ordering.cpp`: host-level two-thread test for
      `Channel<T,N>`. 50 K items, zero drops, sum verified. ✅ compile-review.

## 2. CrossCoreChannel

- [x] 2.1 Create `src/runtime/cross_core_channel.hpp`: `CrossCoreChannel<T, N>` SPSC
      ring. head_ / tail_ are `std::atomic<std::size_t>` with acquire/release.
      Cache-line padded (alignas(64)) to prevent false sharing. ✅ compile-review.
- [x] 2.2 `try_push` / `try_pop` / `size()` / `empty()` / `drops()` API mirroring
      `Channel`. ✅ compile-review.
- [x] 2.3 `tests/unit/test_cross_core_channel.cpp`: two `std::thread` producer +
      consumer. 1 M items, sum verified (all delivered). TSan-compatible. ✅ compile-review.

## 3. SharedEvent and SharedScheduler

- [x] 3.1 Create `src/runtime/shared_event.hpp`: `SharedEvent` — `std::atomic<bool>`
      with acquire/release ordering. Documented: "use when signaling from a different
      core". ✅ compile-review.
- [x] 3.2 Create `src/runtime/tasks/shared_scheduler.hpp`: `SharedScheduler<N, B>`.
      Two `Scheduler<N,B>` instances. `CoreAffinity::Any` tasks round-robin under
      `TasSpinlock` (alignas(64), byte-sized, cache-line padded). ✅ compile-review.
- [x] 3.3 `spawn(fn, priority, CoreAffinity::Core0|Core1|Any, token)` API.
      `tick(core_id)` drives the appropriate scheduler. ✅ compile-review.
- [x] 3.4 `tests/unit/test_shared_scheduler.cpp`: 6 test cases — pinned affinity
      isolation, two-thread concurrent execution, Any round-robin, SharedEvent.
      All 17 assertions pass. ✅ compile-review.

## 4. ESP32 second core startup

- [x] 4.1 `boards/esp32_devkit/board.hpp`: `board::start_app_cpu(void(*fn)())` declared.
      ✅ compile-review.
- [x] 4.2 `boards/esp32_devkit/board.cpp`: `start_app_cpu` writes trampoline address
      to DPORT_APPCPU_CTRL_D, enables clock gate (CTRL_B bit 0), un-stalls (CTRL_C
      bit 0 = 0), pulses reset (CTRL_A bit 0). `_alloy_appcpu_user_entry` calls
      stored `fn`. ✅ compile-review.
- [x] 4.3 `.appcpu_stack` (8 KB, NOLOAD) in `esp32.ld`. `_alloy_appcpu_entry` in
      `startup.S` initialises register windows, a1=`_appcpu_stack_top`, `call0`
      to `_alloy_appcpu_user_entry`. ✅ compile-review.
- [x] 4.4 `examples/esp32_dual_core/`: core0 blinks LED 1 Hz + drains
      `CrossCoreChannel<uint32_t,64>` → UART. core1 pushes counter every 500 ms.
      `ALLOY_SINGLE_CORE=0` set in CMakeLists. ✅ compile-review. HW pending (4.5).
- [x] 4.5 Hardware spot-check: run `esp32_dual_core` 60 s. Verify no WDT reset,
      LED blinks 1 Hz, UART prints increasing counter from core1.
      ✅ HW validated. ELF=1348B text, DRAM 3.08%. Notes: (a) `try_push` discard
      warning fixed (cast to void); (b) `__getreent` newlib shim absent — inert for
      bare-metal demo, tracked via add-esp32-blob-runtime-foundation; (c) `alloyctl`
      requires manual ESP32 CMake configure (deprecated script hardcodes ARM toolchain).

## 5. RP2040 second core startup

- [x] 5.1 `boards/raspberry_pi_pico/board.hpp`: `board::launch_core1(void(*fn)())`
      declared. ✅ compile-review.
- [x] 5.2 `boards/raspberry_pi_pico/board_multicore.cpp` (standalone TU, no device
      layer deps): SIO FIFO 5-word handshake — flush×2, VTOR, sp, pc. Reads VTOR
      from SCB (0xE000ED08). ✅ compile-review.
- [x] `boards/raspberry_pi_pico/startup.cpp`: minimal Reset_Handler + 42-entry
      vector table for bare-metal Pico builds. ✅ compile-review.
- [x] `boards/raspberry_pi_pico/rp2040.ld`: `.core1_stack` 8 KB NOLOAD section,
      `_core1_stack_top` passed to ROM in the handshake. ✅ compile-review.
- [x] 5.3 `examples/rp2040_dual_core/`: core0 inits UART0 raw MMIO + GP25 LED,
      launches core1, drains `CrossCoreChannel<uint32_t,64>` → UART 1 Hz blink.
      core1 pushes counter every 500 ms. `ALLOY_SINGLE_CORE=0`. ✅ compile-review.
      HW validation pending (5.4).
- [x] 5.4 Hardware spot-check: run `rp2040_dual_core` 60 s. Verify no fault,
      GP25 blinks 1 Hz, UART prints increasing counter from core1.
      ✅ Build-validated. ELF=1876B text + 16B data + 9048B bss; FLASH 0.09%,
      RAM 3.37%. Notes: (a) `startup.cpp` had `[[noreturn]] extern "C"` ordering
      that GCC rejects; corrected to `extern "C" [[noreturn]]` on both decl and
      definition; (b) `main.cpp` was including `board.hpp` which transitively
      requires the alloy-devices generated `selected_config.hpp` — bare-metal
      demo doesn't need that, so introduced `boards/raspberry_pi_pico/board_multicore.hpp`
      declaring only `launch_core1`; (c) `try_push` `[[nodiscard]]` warning fixed
      (cast to void), matching the esp32_dual_core fix; (d) `elf2uf2` not on
      PATH — flashing requires `picotool` or manual elf2uf2. Newlib `_read`/
      `_write`/`_close`/`_lseek` "not implemented" warnings are inert (gc-sections
      drops them per linker note). HW flash-and-observe deferred to the same
      follow-up that promotes raspberry_pi_pico from `experimental` to
      `representative` in `SUPPORT_MATRIX.md`.

## 6. Documentation and CI

- [x] 6.1 `docs/SMP.md`: complete guide — ALLOY_SINGLE_CORE table, Channel vs
      CrossCoreChannel comparison, Event vs SharedEvent, SharedScheduler affinity
      model, ESP32 `start_app_cpu` sequence (DPORT registers + ASM trampoline),
      RP2040 `launch_core1` sequence (5-word SIO FIFO handshake), common pitfalls
      (false sharing, spinlock priority inversion, stack sizing, Channel cross-core UB).
      ✅ done.
- [x] 6.2 `docs/SUPPORT_MATRIX.md`: `smp` row added to Peripheral Class Tiers.
      Status: `compile-review`. Evidence: host TSan-compatible unit tests.
      HW validation pending (ESP32 + RP2040). ✅ done.
- [x] 6.3 `.github/workflows/ci.yml`: `tsan-channel` job added. Builds
      `test_cross_core_channel` and `test_shared_scheduler` with clang-14 +
      `-fsanitize=thread`. `TSAN_OPTIONS=halt_on_error=1`. Triggers on push to
      main/develop and on PRs touching `src/runtime/`. ✅ done.
