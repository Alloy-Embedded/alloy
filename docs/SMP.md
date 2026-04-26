# Alloy SMP / Multi-Core Programming Guide

Alloy supports symmetric multi-processing on the ESP32 (Xtensa LX6 dual-core)
and Raspberry Pi Pico (RP2040 dual Cortex-M0+). This guide covers the memory
ordering model, the SMP-safe primitives, and the platform startup sequences.

---

## Memory ordering: ALLOY_SINGLE_CORE

Every platform cmake file defines `ALLOY_SINGLE_CORE` as a preprocessor macro:

| Platform | Value | Meaning |
|----------|-------|---------|
| same70, stm32f4, stm32g0, avr-da, esp32c3, linux | `1` | Single-core only |
| esp32, esp32s3, rp2040 | `0` | Dual-core SMP |

Code in `src/runtime/` uses `#if ALLOY_SINGLE_CORE` to select the cheapest
correct memory barrier for the target:

```cpp
#if ALLOY_SINGLE_CORE
    // Compiler-only barrier: prevents reordering across ISR boundaries.
    // The CPU has no out-of-order observation at the ISR boundary on single-core parts.
    std::atomic_signal_fence(std::memory_order_release);
#else
    // Full CPU memory barrier: ensures visibility across cores.
    std::atomic_thread_fence(std::memory_order_release);
#endif
```

**Rule of thumb**: if your producer and consumer share a CPU core (even if
one is an ISR), `atomic_signal_fence` is sufficient. If they run on _different_
cores, you need `atomic_thread_fence` or, better, use `CrossCoreChannel` /
`SharedEvent` which apply the correct ordering unconditionally.

---

## Channel vs CrossCoreChannel

| | `Channel<T,N>` | `CrossCoreChannel<T,N>` |
|---|---|---|
| Header | `runtime/tasks/channel.hpp` | `runtime/cross_core_channel.hpp` |
| `head_` / `tail_` type | `std::size_t` (plain) | `std::atomic<std::size_t>` |
| Fence (single-core build) | `atomic_signal_fence` | `atomic_thread_fence` (always) |
| Fence (SMP build) | `atomic_thread_fence` | `atomic_thread_fence` (always) |
| False-sharing guard | No | `alignas(64)` between head and tail |
| Intended use | ISR → task on the **same core** | Producer on one core, consumer on another |
| `wait()` / `co_await` | Yes (`OnEventAwaiter`) | Not provided |

For ISR → task on a single core, prefer `Channel<T,N>` — it avoids RMW
atomics entirely (important on Cortex-M0+ which has no `ldrex/strex`).

For cross-core data pipes, always use `CrossCoreChannel<T,N>`.

```cpp
// Correct: core 1 pushes, core 0 pops
static alloy::tasks::CrossCoreChannel<std::uint32_t, 64> g_channel;

// Core 1 entry
void core1_main() {
    while (true) {
        g_channel.try_push(sensor_read());
        sleep_ms(10);
    }
}

// Core 0 loop
void core0_loop() {
    while (auto val = g_channel.try_pop()) {
        process(*val);
    }
}
```

---

## Event vs SharedEvent

| | `Event` | `SharedEvent` |
|---|---|---|
| Header | `runtime/tasks/event.hpp` | `runtime/shared_event.hpp` |
| `signaled_` type | `bool` (plain) | `std::atomic<bool>` |
| `signal()` fence | `atomic_signal_fence(release)` | `store(memory_order_release)` |
| `consume()` fence | `atomic_signal_fence(acquire)` | `load(memory_order_acquire)` |
| Intended use | ISR on the **same core** as the scheduler | Producer on a different core |

Use `SharedEvent` when the signalling core is different from the consuming
core. Both types have the same API (`signal()`, `consume()`, `is_signaled()`).

---

## SharedScheduler and CoreAffinity

`SharedScheduler<N, B>` owns two `Scheduler<N, B>` instances and a TAS
spinlock for safe `CoreAffinity::Any` spawning.

```cpp
#include "runtime/tasks/shared_scheduler.hpp"

// N = max tasks per core, B = max frame bytes
alloy::tasks::SharedScheduler<8, 512> sched;

// Pinned tasks
sched.spawn([] { return core0_task(); }, Priority::Normal, CoreAffinity::Core0);
sched.spawn([] { return core1_task(); }, Priority::Normal, CoreAffinity::Core1);

// Load-balanced (round-robin at spawn time, under spinlock)
sched.spawn([] { return background_task(); }, Priority::Low, CoreAffinity::Any);

// Each core drives its own scheduler from its main loop
// Core 0:
while (sched.tick(0)) {}

// Core 1:
while (sched.tick(1)) {}
```

`CoreAffinity::Any` tasks are distributed round-robin to a specific core at
spawn time — not at runtime. There is no task migration after spawn.

### TAS Spinlock

`TasSpinlock` (a byte-sized `std::atomic<uint8_t>`) is `alignas(64)` to prevent
false sharing with the scheduler state on adjacent cache lines. Only the
`spawn(Any)` path takes the spinlock; `tick()` and pinned spawns are lock-free.

---

## ESP32 second-core startup

Call `board::start_app_cpu(fn)` **once**, from core 0, after `board::init()`:

```cpp
#include "boards/esp32_devkit/board.hpp"

extern "C" void alloy_main() {
    board::init();
    board::start_app_cpu([] { core1_main(); });
    // Core 0 continues here; core 1 runs core1_main()
}
```

Internally, `start_app_cpu`:
1. Stores `fn` in a static variable.
2. Writes the address of `_alloy_appcpu_entry` (ASM trampoline) to
   `DPORT_APPCPU_CTRL_D_REG` (0x3FF00044).
3. Enables the APP_CPU clock gate (`DPORT_APPCPU_CTRL_B_REG` bit 0).
4. Un-stalls the APP_CPU (`DPORT_APPCPU_CTRL_C_REG` bit 0 = 0).
5. Pulses reset (`DPORT_APPCPU_CTRL_A_REG` bit 0: assert then deassert).

The trampoline (`_alloy_appcpu_entry` in `startup.S`) resets Xtensa register
windows, sets `a1` to `_appcpu_stack_top` (8 KB DRAM section), then `call0`s
`_alloy_appcpu_user_entry`, which calls `fn`.

### APP_CPU stack

The ESP32 linker script (`esp32.ld`) reserves 8 KB in DRAM:

```
.appcpu_stack (NOLOAD) : ALIGN(16) {
    _appcpu_stack_bottom = .;
    . = . + 8192;
    _appcpu_stack_top = .;
} > DRAM
```

---

## RP2040 second-core startup

Call `board::launch_core1(fn)` **once**, from core 0:

```cpp
#include "boards/raspberry_pi_pico/board.hpp"

extern "C" void alloy_main() {
    board::launch_core1([] { core1_main(); });
    // Core 0 continues; core 1 runs core1_main()
}
```

Internally, `launch_core1` performs the **5-word SIO FIFO handshake**
(RP2040 TRM §2.8.2):

| Step | Word sent | Meaning |
|------|-----------|---------|
| 0 | `0` | Flush — core 1 resets its state machine |
| 1 | `0` | Second flush |
| 2 | VTOR | Vector table base (read from `SCB_VTOR` at 0xE000ED08) |
| 3 | SP | `_core1_stack_top` (8 KB `.core1_stack` section in SRAM) |
| 4 | PC | `fn` entry point |

After each word, core 0 waits for core 1 to echo it back. If the echo
doesn't match, the handshake restarts from word 0. On success, the RP2040
ROM sets core 1's VTOR, SP, and PC, then lets it run.

### Core 1 stack

The RP2040 linker script (`rp2040.ld`) reserves 8 KB in SRAM:

```
.core1_stack (NOLOAD) : ALIGN(8) {
    _core1_stack_bottom = .;
    . = . + 8192;
    _core1_stack_top = .;
} > RAM
```

---

## Common pitfalls

### False sharing

`CrossCoreChannel` places `head_` and `tail_` on separate 64-byte cache
lines (`alignas(64)`). If you share other data between cores, ensure
frequently-written fields don't share a cache line:

```cpp
struct Shared {
    alignas(64) std::atomic<uint32_t> core0_counter{0};
    alignas(64) std::atomic<uint32_t> core1_counter{0};
};
```

### Spinlock priority inversion

`TasSpinlock` (used inside `SharedScheduler::spawn(Any)`) busy-spins.
On bare-metal, priority inversion is not an issue — both cores run at the
same privilege level. Avoid holding the spinlock for long; `spawn` is the
only code path that does so.

### Stack sizing

The default 8 KB per-core stack is suitable for shallow coroutine frames.
If core 1 runs deeply nested coroutines or large local arrays, increase the
`_appcpu_stack` / `.core1_stack` section size in the linker script.

### Using Channel across cores

`Channel<T,N>` uses a plain `std::size_t` for `head_` and `tail_`. Even
with `atomic_thread_fence` (the SMP build path), this is technically
undefined behaviour under the C++ memory model (data race on non-atomic
objects). For cross-core use, always prefer `CrossCoreChannel<T,N>`.

---

## See also

- [`src/runtime/cross_core_channel.hpp`](../src/runtime/cross_core_channel.hpp)
- [`src/runtime/shared_event.hpp`](../src/runtime/shared_event.hpp)
- [`src/runtime/tasks/shared_scheduler.hpp`](../src/runtime/tasks/shared_scheduler.hpp)
- [`examples/esp32_dual_core/`](../examples/esp32_dual_core/)
- [`examples/rp2040_dual_core/`](../examples/rp2040_dual_core/)
- [`tests/unit/test_cross_core_channel.cpp`](../tests/unit/test_cross_core_channel.cpp)
- [`tests/unit/test_shared_scheduler.cpp`](../tests/unit/test_shared_scheduler.cpp)
