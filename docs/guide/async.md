# Async tasks (coroutines)

alloy ships an optional **cooperative coroutine runtime** so you can write concurrent firmware as
straight-line `co_await` code instead of hand-rolled state machines — the ergonomics of
[embassy](https://embassy.dev), in C++, with **no heap, no RTOS, and no exceptions**.

```cpp
#include <alloy/async/delay.hpp>
#include <alloy/async/executor.hpp>
#include <alloy/async/task.hpp>
#include <alloy/board.hpp>
using namespace alloy::async;
using namespace alloy::literals;

executor<4> sched;
task_storage<256> blink_frame;

task blink(task_storage<256>&) {
    for (;;) {
        board::led.toggle();
        co_await delay(200ms);        // suspends; the core idles or runs other tasks
    }
}

int main() {
    board::init();
    sched.spawn(blink(blink_frame));
    sched.run();                      // cooperative superloop; never returns
}
```

Two tasks with different periods just work — each is its own timeline, and the executor juggles
them on one stack with no preemption. That is the thing a single `sleep_for()` loop can't express.

## No heap — by construction

A C++ coroutine normally heap-allocates its frame. alloy forbids that structurally:

- Each task's frame lives in a **`task_storage<N>`** you declare (inline storage, embassy-style).
- The task's `operator new` **only** accepts that storage; the plain `operator new` is **deleted**,
  so a task written without a `task_storage&` first parameter **does not compile**. There is no heap
  path even in principle — a full `-nostdlib` link on Cortex-M0+, M7, and Xtensa pulls in zero
  `malloc`/`operator new`.

Frames are tiny (~40–70 bytes for a typical task). The default `task_storage<256>` has generous
headroom, so beginners never pick a number.

### If a frame doesn't fit

If your task's frame is bigger than its `task_storage<N>`, you get a **compile error** naming the
task (at `-Os`/`-O2`), not a silent runtime fault:

```
error: … alloy::async: coroutine frame does not fit its task_storage<N> — increase N
```

Bump `N` and rebuild. (At `-O0`/`-Og` the check falls back to a runtime trap, since the frame size
isn't a constant there.)

## Awaiting drivers

Drivers expose awaitables that suspend until the hardware is ready. The RX interrupt wakes the task
in **thread context** (never inside the ISR), so your task body is ordinary code:

```cpp title="illustrative: an excerpt of examples/async_io — `uart` and the enclosing task are the rest of that file"
#include <alloy/async/uart.hpp>

uart_reader<decltype(uart)> rx{uart};             // uart = board::debug_uart::open({...})

task echo(task_storage<256>&, decltype(rx)& r, decltype(uart)& u) {
    for (;;) {
        std::uint8_t b = co_await r.read();        // suspends until a byte arrives
        u.write(b);
    }
}
```

`uart_reader` buffers received bytes in a lock-free SPSC ring, so a burst that lands while your task
is busy is queued, not dropped. One task reads one UART at a time.

### SPI, I²C and DMA

The bus awaitables suspend for a whole **transfer**, not a byte — the driver's ISR moves every byte
and the task resumes once, in thread context, when the last one lands. Compare `bus.write(...)`,
which spins per byte.

```cpp title="illustrative: four awaits side by side — `co_await` needs the enclosing task, see examples/async_sensor"
#include <alloy/async/spi.hpp>
#include <alloy/async/i2c.hpp>
#include <alloy/async/dma.hpp>

alloy::async::spi_master  spi{bus};        // long-lived, outlives the tasks
alloy::async::i2c_master  i2c{i2c_bus};
alloy::async::dma_waiter  dma{chan};

co_await spi.transfer(out, in);            // one suspend, N bytes exchanged
bool ok = co_await i2c.write(0x08, out);   // false on NACK / bus error
ok      = co_await i2c.read(0x08, in);
co_await dma.run([&] { uart.write_dma_begin(chan, msg); });
uart.write_dma_end(chan);
```

### Awaiting a DMA ring boundary

`dma_waiter` parks a task on one transfer finishing. `ring_waiter` parks it on the *next
hardware-stable half* of a continuously refilling [ring](dma.md) — which is what a control loop
wants, since the transfer never finishes:

```cpp title="illustrative: a task body — `co_await` needs the enclosing task, see examples/async_io"
#include <alloy/async/dma.hpp>

alloy::dma::ring_storage<std::uint16_t, 256> storage;
auto stream = adc.ring(storage);           // outlives the task
alloy::async::ring_waiter waiter{stream};  // constructed after the ring, same lifetime

for (;;) {
    std::span<const std::uint16_t> half = co_await waiter.take();
    process(half);
}
```

A consumer that awaits more slowly than halves go stable does **not** deadlock: it wakes on the
next boundary, `take()` resynchronizes to the most recent half, and the skipped ones show up in
`stream.missed()`. One waiter per ring, like every other waiter here.

Three things are worth knowing before you use them:

- **The transfer starts inside the `co_await`**, after the task is parked. That closes the
  lost-wakeup race by construction — the completion interrupt has no window to fire into — but it
  means you cannot kick a transfer off, do other work, and await it later. Use the callback API
  (`transfer_async` + `busy()`) for that shape.
- **`dma_waiter` must be constructed before the first transfer.** The channel's config register can
  only be written while the channel is disabled, so the driver folds the interrupt enable in at
  setup based on whether a callback is registered. A waiter created inside the coroutine would arm
  nothing and park forever.
- **One task per bus.** A second task awaiting the same object traps in `waiter_slot`. A shared bus
  needs one owning task that fans out.

Do not mix an awaited transfer with the blocking calls on the same bus: the ISR consumes the very
flags the blocking loop waits for.

## What has actually been observed running

The honest split, because "it compiles" and "it ran" are different claims:

| Awaitable | Evidence |
|---|---|
| `spi_master`, `i2c_master`, `dma_waiter` | **Ran on the ISA.** The `async_io` example + `async_io.robot` assert, on an emulated STM32G071RB, that each task *suspended and was resumed by the peripheral interrupt* — the leg prints how many times the executor resumed it, and the same I/O done without suspending prints a different number and fails the test. |
| `uart_reader` | **Host unit tests only**, against a `mock_uart` double. No example uses it and no emulation leg exercises it; it has never been observed running on a real ISA. |
| `ring_waiter` | **Host unit tests only** — two cases in `tests/test_dma_ring.cpp`, covering that a parked task wakes once per boundary and that a slow consumer resynchronizes. No example uses it and no emulation leg exercises it. |

None of it has run on physical silicon — see the README's list of what is not done, and
[What is proven, and how](../reference/proof.md) for the same split across the whole framework.

## Concurrency doctrine — priorities, preemption, and what it costs

Every evaluation asks the same two questions. The short answers:

- **Priorities?** Yes, among **interrupts** — `alloy::irq::set_priority(line, level)` writes the NVIC
  priority field, and on ARMv7-M `alloy::irq::critical_section{level}` masks only lines at or below
  a level so a control-loop ISR keeps running through it. **No**, among **tasks**: the executor's
  ready queue is FIFO and has no priority field.
- **Preemption?** **No**, between tasks. **Yes**, of every task, by every interrupt.

That is the whole doctrine: *ISRs are the real-time tier; the executor is the "everything else"
tier.* A task never interrupts another task, so no task needs a lock against another task, which is
why nothing in `alloy::async` is a mutex. What replaces the RTOS priority table is the NVIC, which
is a preemptive priority scheduler already present in the silicon.

The rule that follows is uncomfortable and worth stating plainly:

!!! warning "The cooperative bargain"
    The worst-case wake latency of any task is the **tick quantisation plus the longest single
    non-yielding run of every other runnable task**. One task that computes for 5 ms without a
    `co_await` delays every other task by 5 ms. There is no preemption to save you, and the
    framework cannot detect it. Work that must not be delayed belongs in an ISR.

Measured, below: a 2 ms `co_await delay(2ms)` lands at 2000 µs alone, and at **6001 µs** beside a
task that runs 5 ms without yielding. That is the bargain, in numbers.

### How these numbers were obtained

Everything below comes from one program — `examples/concurrency_probe` — run under Renode, gated in
CI by `tests/emulation/concurrency_probe.robot`. Reproduce it with:

```console
$ cd examples/concurrency_probe && alloy emulate
```

- **Board:** `nucleo_g071rb` (STM32G071RB, **Cortex-M0+**, Thumb-1, 16 MHz HSI), built at the
  default `-Os` with arm-none-eabi-gcc 14.2.
- **Instrument:** the SysTick counter read at its native resolution (`systick_elapsed()`), not
  `uptime_us()` — 1 µs is 16 core clocks here, coarser than the things being timed.
- **Unit:** **instruction-equivalents (ieq)** — the measured interval divided by the cost of one
  instruction, calibrated in-run against a block of exactly 1000 straight-line `nop`s. The probe
  prints raw counter values alongside so the division can be checked.
- **Latency figures** are the mean of 1024 samples (each individual sample is quantised to a whole
  counter tick, so only the mean carries resolution; min/max are printed and are honestly coarse).
  **Cost figures** are batches of 512 iterations with a baseline batch subtracted.
- **Determinism, and how far to read a number:** two runs of the same binary produce byte-identical
  output — that is what lets a figure be quoted at all. Two *different* binaries do not: adding a
  line to this probe moved `irq raw` by 0.5 ieq and three `sched` rows by 0.4 ieq, because code
  layout moves them and because a 512-iteration batch is quantised to 1 µs (0.39 ieq). **Read these
  to about ±1 ieq.** The second decimal is printed because the probe computes it, not because it
  reproduces across builds; only the run that produced this table reproduces it exactly.

**The instrument validates against the disassembly.** `systick_elapsed()` compiles to six
instructions plus the caller's `bl`; the interval between two back-to-back stamps therefore spans
seven to eight instructions. Measured: **7.87 ieq**. That agreement is the reason the other figures
are quoted at all.

### Interrupt path

From the store that raises the interrupt to the first instruction of the handler:

| Path | ieq | What it is |
|---|---|---|
| strong `<NAME>_IRQHandler` | 16.87 | the vendor-style expert path — no framework in the way |
| via `alloy::irq::attach` | 29.87 | + weak wrapper → `alloy_irq_dispatch(n)` → chain walk |
| two handlers on one line | 43.00 | + a second chain node |

So **`alloy::irq` costs about 13 ieq over a raw vector**, and **each additional handler sharing a
line costs about 13 ieq**. If you need neither, define a strong `<NAME>_IRQHandler`: it overrides
alloy's weak wrapper entirely and you get the first row. (See
[the escape hatch](escape-hatch.md).)

### What delays an interrupt

Nothing in alloy delays an ISR except an interrupts-off critical section — so the question is how
long alloy holds one. Raising the line *from inside* a critical section measures exactly that:

| Masked region | ieq | Delta |
|---|---|---|
| empty `irq_save`/`irq_restore` | 36.87 | +7.0 over the unmasked path |
| 100-iteration loop | 537.50 | +500.6, i.e. **5.0 ieq per iteration** |

Interrupt latency tracks the masked region **1:1**. alloy's own critical sections are the ones in
`scheduler::signal()`, `executor::schedule()` and `event::set()` — each a handful of instructions
around a queue push, all visible in the `exec` figures below. There is no long masked region hiding
anywhere: the framework never masks across a loop, an I/O wait, or a call it does not control.

### Scheduler and executor cost

`alloy::scheduler` (the non-async table in `sched.hpp`), one `run_once()` superstep with every task
runnable and an empty body:

| Runnable tasks | superstep (ieq) | per task (ieq) |
|---|---|---|
| 1 | 12.87 | 12.87 |
| 2 | 71.00 | 35.50 |
| 4 | 147.25 | 36.75 |
| 8 | 298.75 | 37.34 |
| 16 | 578.87 | 36.12 |

The `N = 1` row is the odd one out and is not a typo: a one-entry table is small enough that `-Os`
optimises the superstep down to roughly a single dispatch, so the step from 1 to 2 tasks (+58 ieq)
is much larger than every step after it. Size for the marginal cost below, not for that row.

`alloy::async::executor`, one wake+resume of a coroutine parked on an `event` — that is the async
"task switch": the ISR-side `set()`, the ready-queue push, the dequeue, the coroutine resume, and
the re-suspend at the next `co_await`:

| Ready tasks | superstep (ieq) | per task (ieq) |
|---|---|---|
| 0 (empty poll) | 35.12 | — |
| 1 | 228.12 | 228.12 |
| 2 | 412.00 | 206.00 |
| 4 | 779.62 | 194.87 |
| 8 | 1515.12 | 189.37 |

Read the **marginal** cost, not the average: each extra runnable task adds **36.3 ieq** to a
`scheduler` superstep (measured from `N = 2` upwards) and **183.9 ieq** to an executor superstep.
Both are linear in the number of *runnable* tasks: there is no priority search and no per-task
scan of a ready list.

### What a suspended task costs

"Parked" is two different things, and only one of them is free:

| Executor state | empty superstep (ieq) |
|---|---|
| 8 tasks parked on an `event` | 34.75 |
| the same 8, plus 8 parked on `delay()` | 131.25 |

A task waiting on an `event` is in **no list the executor owns** — `set()` from the ISR pushes it
onto the ready queue and nothing polls it meanwhile, so eight of them cost the same as none. A task
inside `co_await delay(...)` is different: it holds a `timer_node`, and `run_once()` walks that list
on **every** superstep before it touches the ready queue. That is **≈12 ieq per sleeping task, per
superstep** — small, but O(sleeping tasks) rather than zero, and `delay()` is the most common idiom
in the framework. Sixteen sleepers on a busy-polled executor is a few thousand ieq per second of
pure list walking; if that matters, park on events driven by one hardware timer instead of giving
every task its own `delay()`.

(An earlier version of this page said an event-parked *or* timer-parked task cost nothing. The
`parked` rows above and the `verdict parked` line in the CI leg exist because that was measured and
was only half true.)

A coroutine resume costing ~5x a plain function-pointer dispatch is the price of `co_await`: the
frame's resume dispatch, the promise bookkeeping, and the awaiter's park/wake protocol. If a task is
a simple periodic poll, `alloy::scheduler` is the cheaper tool and is still there.

### Scheduling jitter

A task doing `co_await delay(2ms)` in a loop, measuring what it actually got, 16 samples:

| Condition | measured wake (µs) for a requested 2000 |
|---|---|
| alone on the executor | 1569 – 2000 |
| beside a task that runs 5 ms without yielding | 5993 – 6001 |

Two separate facts are in that table.

**`delay()` can fire early.** Its deadline is `uptime_ms() + n`, and `uptime_ms()` is truncated to
the 1 kHz tick, so a delay armed part-way through a tick loses that fraction. The bound is one full
tick early; 1569 µs is what this build happened to hit. `delay()` is a **tick-quantised timer, not a
one-shot with sub-tick accuracy** — for tighter timing use a hardware timer interrupt, and for
measuring elapsed time use `uptime_us()`.

**A non-yielding task is the dominant term.** 2 ms became 6 ms. Nothing in that number is an
emulator artifact: it is the hog's own 5 ms (measured by the hog in the same timebase) plus tick
quantisation.

### What these numbers are NOT

Renode is an instruction interpreter, not a cycle-accurate model. It advances virtual time by a
fixed slice per executed instruction. That has one large advantage — the measurements are perfectly
repeatable, which is why they can be a CI gate — and several hard limits:

- **Not silicon.** Nothing here has run on a physical STM32G071. Every figure is an emulation
  result.
- **No cycles-per-instruction model.** Renode charges a `nop` and a `ldr` the same. On a real
  Cortex-M0+ loads, stores and taken branches cost more, so ieq is a count of *work*, not of cycles.
  Converting ieq to nanoseconds by dividing by the core clock would be wrong.
- **No memory system.** Flash wait states, the prefetch buffer, caches (an M7 has them; the M0+ does
  not), bus contention and DMA cycle-stealing are all outside the model. On a real part running from
  flash at high clock these can dominate the numbers above.
- **Architectural exception entry is not charged.** The `irq` row absolute values contain almost
  none of the register-stacking cost real silicon pays on exception entry; that figure is in ARM's
  TRM for your core and is not alloy's to claim. **Only the differences between the three rows are
  alloy's, and only those are quoted as such.**
- **Not the peripheral's own delay.** The probe raises the NVIC line in software. A real edge also
  pays the peripheral's detect/synchronise path (a per-IP datasheet number) before the NVIC ever
  sees it.
- **One board, one core, one build.** Cortex-M0+ only. The `sched`/`exec`/`jitter` legs also ran
  unmodified on `nucleo_f722ze` (Cortex-M7, Thumb-2) and came out roughly 25% cheaper — 27.1 ieq per
  scheduler task, 144.5 ieq per executor resume — but that run's `irq vectored` verdict is **FAIL**
  by construction (vector 22 is `CAN1_SCE` there, not `TIM17`, so the strong handler is an orphan),
  so its interrupt figures are invalid and are not quoted. **RL78 and Xtensa are entirely
  unmeasured.**
- **No worst-case guarantee.** These are measured means and observed ranges, not a WCET analysis.
  alloy has no timing analysis tool, and the numbers here do not constitute one.

The CI leg asserts none of these figures. It asserts only the *ordering* properties the doctrine
rests on — that `alloy::irq` costs more than a raw vector, that a longer masked region delays an ISR
more than a short one, that a non-yielding task delays a sleeping one, and that timer-parked tasks
cost a superstep more than event-parked ones — because those survive a Renode upgrade and an
absolute number does not.

## How it works

| Piece | Role |
|---|---|
| `task` | the coroutine return type; frame in `task_storage<N>` |
| `executor<MaxReady>` | ready-queue + timer list; `run_once()` resumes ready tasks, `run()` idles between wakes |
| `event` | reusable race-free wake primitive — `set()` from an ISR wakes a parked task |
| `delay(ms)` | suspend on the executor's timer list over `uptime_ms()` |
| `waiter_slot` | the one park/wake primitive every driver awaitable composes — never re-derived |
| `uart_reader` | `co_await read()` over a driver's RX interrupt |
| `spi_master` | `co_await transfer(out, in)` — one suspend per exchange |
| `i2c_master` | `co_await write(addr, buf)` / `read(...)` — resumes with the ACK result |
| `dma_waiter` | `co_await run(start)` over a channel's completion interrupt |

Wakes are ISR-safe and lossless: the ready-queue **traps on overflow** rather than silently dropping
a wake, so size `MaxReady` at least as large as your task count. On Cortex-M the core sleeps in `WFI`
between events; on ESP32 v1 (no tick interrupt yet) the executor busy-polls, exactly as
`alloy::sleep_for` does there.

!!! note "Portability"
    The same `main.cpp` builds for every board with zero `#ifdef`. The one arch-specific piece —
    idling and interrupt masking — lives behind the `alloy::arch` seam, not in your code.
