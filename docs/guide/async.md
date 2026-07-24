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

## How it works

| Piece | Role |
|---|---|
| `task` | the coroutine return type; frame in `task_storage<N>` |
| `executor<MaxReady>` | ready-queue + timer list; `run_once()` resumes ready tasks, `run()` idles between wakes |
| `event` | reusable race-free wake primitive — `set()` from an ISR wakes a parked task |
| `delay(ms)` | suspend on the executor's timer list over `uptime_ms()` |
| `uart_reader` | `co_await read()` over a driver's RX interrupt |

Wakes are ISR-safe and lossless: the ready-queue **traps on overflow** rather than silently dropping
a wake, so size `MaxReady` at least as large as your task count. On Cortex-M the core sleeps in `WFI`
between events; on ESP32 v1 (no tick interrupt yet) the executor busy-polls, exactly as
`alloy::sleep_for` does there.

!!! note "Portability"
    The same `main.cpp` builds for every board with zero `#ifdef`. The one arch-specific piece —
    idling and interrupt masking — lives behind the `alloy::arch` seam, not in your code.
