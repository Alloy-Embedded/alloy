# Tasks: Cooperative Coroutine Scheduler

Alloy ships a small, opt-in concurrency layer for application code. It is
**not** an RTOS. There is no preemption, no per-task stack, no mutex, and no
priority inversion -- by construction. What you get instead is a way to write
several independent flows of work as linear-looking C++23 coroutines that
share a single CPU stack and switch only at `co_await` points the user can see.

If the existing super-loop fits your application, ignore this layer. Including
its headers costs nothing on the link side.

If you have more than one independent flow (a blink, a UART console, a Modbus
poll, a button debouncer, a sensor sampler), this layer turns the resulting
state-machine spaghetti into something readable.

```cpp
auto blink() -> Task {
    while (true) {
        board::led::toggle();
        co_await alloy::tasks::delay(500ms);
    }
}

auto uart_echo() -> Task {
    while (true) {
        while (auto byte = uart_rx.try_pop()) {
            while (!uart_tx.try_send(*byte)) {
                co_await uart_tx.wait_space();
            }
        }
        co_await uart_rx.wait();
    }
}

int main() {
    board::init();
    alloy::tasks::Scheduler<4, 256> sched;
    sched.set_time_source(time_now);
    sched.spawn([] { return blink(); },     alloy::tasks::Priority::Normal);
    sched.spawn([] { return uart_echo(); }, alloy::tasks::Priority::High);
    sched.run();
}
```

The rest of this document is the reference for everything that goes into the
snippet above.

## Quick start

1. **Pick a `Scheduler` size.** Two template parameters:
   `Scheduler<MaxTasks, MaxFrameBytes>`. `MaxTasks` is how many `Task`s can be
   spawned at once. `MaxFrameBytes` is the size of one slot in the coroutine
   frame pool; it must be large enough for the biggest coroutine you spawn.
   For tasks that only `delay`, `yield_now`, `on(event)`, and operate on a few
   locals, ~256 bytes is comfortable on Cortex-M0+ at `-Os`. The scheduler
   refuses to allocate frames bigger than that and reports
   `SpawnError::FrameTooLarge` -- you bump the parameter and rebuild.

2. **Install a time source.** The scheduler does not assume any specific tick
   timer. Hand it a callable that returns
   `alloy::runtime::time::Instant`:

   ```cpp
   sched.set_time_source([] {
       return alloy::runtime::time::Instant::from_micros(
           alloy::hal::SysTickTimer::micros<board::BoardSysTick>());
   });
   ```

3. **Spawn tasks with priorities.** Always pass a *factory* lambda, not a
   pre-built `Task`. The scheduler installs the pool around the factory call
   so the coroutine's `operator new` can find it.

   ```cpp
   sched.spawn([] { return my_task(); }, Priority::High);
   ```

4. **Run.** `sched.run()` is the trivial `while (tick()) {}` loop. Or call
   `tick()` yourself if you need to interleave with other code. Both never
   return until you call `sched.request_stop()`.

## Footprint

| Configuration | Flash (text+rodata) | RAM (data+bss) |
|---|---|---|
| `Scheduler<3, 256>` + 3 tasks (blink + producer + consumer + UART RX channel) on `nucleo_g071rb`, `-Os` | 5132 B | 4696 B |

This is the entire example app, including alloy's HAL and descriptor runtime.
The scheduler itself is roughly 3 KB of code; the rest is per-task pool RAM
(`MaxTasks * MaxFrameBytes`) plus a few bytes of bookkeeping.

## Priority

Four levels, declared at spawn time, immutable thereafter:

```cpp
enum class Priority : uint8_t { High = 0, Normal = 1, Low = 2, Idle = 3 };
```

The scheduler always resumes the highest-priority *Ready* task at the next
yield point. Within a level, FIFO. There is no priority inheritance, no
aging, no boosting -- cooperative scheduling means a `priority::low` task
that holds the CPU between `co_await` points runs until its next yield, then
the highest-priority ready task wins. Nothing is preempted; nothing
inverts.

## Awaiters

### `delay(duration)`

Suspend until a deadline measured against the installed time source. Returns
`Result<void, Cancelled>` -- the inner value is `Cancelled` if the task's
cancellation token fired during the wait.

```cpp
co_await alloy::tasks::delay(alloy::runtime::time::Duration::from_millis(500));
```

### `yield_now()`

Voluntary yield. The task stays Ready and is requeued at the back of its
priority FIFO.

```cpp
co_await alloy::tasks::yield_now();
```

Use to give peers at the same priority a turn during a long-running
computation that otherwise has no natural `co_await` point.

### `on(event)`

Suspend until an `alloy::tasks::Event` is signalled (typically by an ISR).
The event is consumed on resume.

```cpp
alloy::tasks::Event button_pressed;

extern "C" void EXTI4_15_IRQHandler() { button_pressed.signal(); }

auto handler() -> Task {
    while (true) {
        co_await alloy::tasks::on(button_pressed);
        // edge already debounced upstream; just react.
    }
}
```

### `until(predicate)`

Suspend until a predicate returns true. The scheduler polls the predicate
once per tick.

```cpp
extern bool radio_locked;

co_await alloy::tasks::until([&] { return radio_locked; });
```

The predicate runs in scheduler context, never from an ISR. Keep it cheap and
side-effect-free; it may be called many times before the condition flips.

The predicate captures by reference. Anything in the lambda's capture must
outlive the `co_await` -- because the lambda is a temporary in the await
expression, the C++ coroutine machinery extends its lifetime across the
suspension, so this is safe in normal use.

## Channels

`Channel<T, N>` is a single-producer / single-consumer ring buffer. The
producer calls `try_push(value)` (ISR-safe). The consumer calls `try_pop()`
(returns `std::optional<T>`) and `wait()` (suspends until the next push).

`T` must be trivially copyable. `N` must be a power of two.

The canonical consumer loop:

```cpp
while (true) {
    while (auto value = ch.try_pop()) {
        process(*value);
    }
    co_await ch.wait();
}
```

Drain everything ready, then suspend. Multiple values that arrive between two
consumer wakes are never lost.

If the producer fills the ring faster than the consumer drains it, `try_push`
returns false and `ch.drops()` increments. Production code samples `drops()`
periodically -- a steady rise means the ring is undersized or the consumer is
starved.

## UART helpers

Two header-only convenience wrappers sit on top of `Channel`. They do not
touch the alloy HAL UART -- application code owns the three lines that
bridge a vendor-specific RX-byte-ready or TX-empty signal into the channel.
This is intentional: each vendor exposes those signals differently (status
register polling, idle-line interrupt, descriptor-driven event), and tying
the helpers to one of those would break the others.

### `UartRxChannel<N>`

```cpp
alloy::tasks::UartRxChannel<32> uart_rx;

extern "C" void USART2_IRQHandler() {
    if (USART2->ISR & USART_ISR_RXNE_RXFNE) {
        const auto byte = static_cast<std::byte>(USART2->RDR);
        static_cast<void>(uart_rx.feed_from_isr(byte));
    }
}

auto echo() -> Task {
    while (true) {
        while (auto byte = uart_rx.try_pop()) {
            // process byte
        }
        co_await uart_rx.wait();
    }
}
```

### `UartTxChannel<N>`

The TX side has one extra concept: a *kick* callback. On most MCUs the UART
TXE interrupt fires only when the data register is empty *and* the interrupt
is enabled. When the application pushes the first byte into an empty queue,
the ISR is typically off; if nothing wakes it, the bytes never go out. The
kick callback runs on the empty -> non-empty transition so the application
can re-enable the TX-empty interrupt.

```cpp
alloy::tasks::UartTxChannel<32> uart_tx;

void board_init() {
    uart_tx.set_kick_callback([] { USART2->CR1 |= USART_CR1_TXEIE; });
}

extern "C" void USART2_IRQHandler() {
    if (USART2->ISR & USART_ISR_TXE_TXFNF) {
        if (auto byte = uart_tx.pop_for_isr()) {
            USART2->TDR = static_cast<uint32_t>(*byte);
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;  // queue drained, ISR off
        }
    }
}

auto sender() -> Task {
    for (auto byte : payload) {
        while (!uart_tx.try_send(byte)) {
            co_await uart_tx.wait_space();   // back-pressure
        }
    }
    while (!uart_tx.empty()) {
        co_await uart_tx.wait_drained();     // drain-to-empty
    }
}
```

## Cancellation

`CancellationToken` is a small flag. Pass one into `spawn` and call
`request()` on it from anywhere; the task's next `co_await` returns
`Result<T, Cancelled>` with `Cancelled::Yes`.

```cpp
auto token = CancellationToken::make();
sched.spawn([&] { return my_task(); }, Priority::Normal, token);

// Later, e.g. from a button ISR:
token.request();
```

Inside the task:

```cpp
auto my_task() -> Task {
    auto r = co_await alloy::tasks::delay(1s);
    if (r.is_err()) {
        // Cancelled -- clean up and exit.
        co_return;
    }
    // ...continue normally...
}
```

Parent -> child token chaining is a follow-up; v1 ships only the simple flag.

## Pool sizing

The scheduler refuses to allocate coroutine frames bigger than the configured
slot. When `spawn` returns `SpawnError::FrameTooLarge`, the error message
includes the actual frame size and the configured slot size; bump the
template parameter and rebuild.

A reasonable starting point: `Scheduler<8, 256>` for most foundational
boards. Coroutines that hold large arrays as local variables, or that compose
many awaiters in one expression, may need 384 or 512 bytes per slot.

You can measure exact frame sizes with the GCC/Clang builtin
`__builtin_coro_size(MyTask)` if your compiler supports it. The scheduler's
runtime check is the safety net regardless.

## ISR safety

| Primitive | ISR-safe? |
|---|---|
| `Event::signal()` | ✅ |
| `Channel::try_push()` | ✅ |
| `UartRxChannel::feed_from_isr()` | ✅ |
| `UartTxChannel::pop_for_isr()` | ✅ |
| `Scheduler::spawn()` | ❌ task context only |
| `Scheduler::tick()` | ❌ |
| Awaiters (`delay`, `on`, `until`, etc.) | task context only |

The ISR-safe operations use `std::atomic_signal_fence` (compiler-only
barrier). On single-core MCUs (every Cortex-M, RISC-V single-core, ESP32
single-core configs) this is sufficient because the ISR is just a function
call from the CPU's perspective. Cross-core SMP configurations (ESP32 LX6/LX7
dual-core) need real `std::atomic` ordering; that variant is a follow-up.

## Architectures

| Architecture | Status |
|---|---|
| `cortex-m0plus`, `cortex-m4`, `cortex-m7` | ✅ tested in CI (host) and cross-compiled |
| `riscv32` (ESP32-C3) | ✅ cross-compiles |
| `xtensa-lx7` (ESP32-S3) | ✅ cross-compiles |
| `xtensa-lx6` (ESP32 classic) | will work once the LX6 family lands |
| `avr` | ❌ avr-gcc 13.x has no C++20 coroutines; build refuses cleanly |

## Limits and follow-ups

The v1 surface is intentionally narrow. These are tracked but not yet
shipped:

- **`Task<T>` typed return values + `co_await task_id` join.** Today every
  task is `Task<void>`. Returning a value implies a join awaiter, which is
  its own design conversation.
- **`any_of` / `all_of` awaiter combinators** for "wait for first" / "wait
  for all" patterns.
- **Parent -> child cancellation chaining.** The token is a single flag;
  cancelling a parent does not currently propagate to children spawned with
  related tokens.
- **SMP-safe variants** of `Event`, `Channel`, `UartRxChannel`, and
  `UartTxChannel` for cross-core signalling on dual-core ESP32.
- **CI footprint gate.** A representative configuration exists; the
  enforcement step that fails the build on regression is pending.
- **Hardware bring-up.** Tests live on the host; the foundational boards
  build the example but no on-silicon spot-check has run yet.

## See also

- [`openspec/specs/runtime-tasks/spec.md`](../openspec/specs/runtime-tasks/spec.md)
  -- normative requirements.
- [`openspec/changes/add-runtime-tasks/design.md`](../openspec/changes/add-runtime-tasks/design.md)
  -- the decisions behind the v1 shape.
- [`examples/tasks_blink_uart/`](../examples/tasks_blink_uart/) -- working
  example.
- [`tests/unit/test_tasks.cpp`](../tests/unit/test_tasks.cpp) -- the host
  test suite that pins the contract.
