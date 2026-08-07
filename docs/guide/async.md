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

```cpp
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

```cpp
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

None of it has run on physical silicon — see the README's list of what is not done.

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
