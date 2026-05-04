# Alloy Async HAL

`alloy::async` is the runtime's bridge between **DMA / interrupt completion**
and the **cooperative coroutine scheduler** in `runtime-tasks`. It lets a
task suspend on a hardware operation — UART DMA, SPI DMA, I2C transfer,
ADC EOC, timer update, or GPIO edge — without busy-waiting and without
blocking other tasks on the same core.

There are two compile-time properties to remember:

1. **Async is opt-in.** Builds that do not include `async.hpp` (or any
   `async_*.hpp`) link no async symbols and pay no async RAM cost. The
   blocking HAL APIs in `hal/uart/`, `hal/spi/`, `hal/i2c/`, etc. are
   the default and remain unchanged.
2. **The same async model covers every peripheral.** UART set the pattern;
   SPI / I2C / ADC / Timer / GPIO follow it identically. If you know how
   to write `async::uart::write_dma`, you know how to write any of the
   others.

## The model

```
    ┌─────────────────────────┐
    │ task: co_await transfer │
    └────────────┬────────────┘
                 │ returns operation<token>
                 ▼
    ┌─────────────────────────┐
    │ HAL handle.write_dma()  │  starts the peripheral + arms IRQ
    └────────────┬────────────┘
                 │ task suspends until poll_status::ready
                 ▼
    ┌─────────────────────────┐
    │ vendor ISR              │  on completion: token::signal()
    └─────────────────────────┘
```

Every async function returns
`core::Result<operation<EventToken>, core::ErrorCode>`. The
`operation<EventToken>` type satisfies the alloy awaiter contract: the
scheduler polls `ready()` on each `tick()` and resumes the task when
the underlying token has been signalled. The vendor ISR is responsible
for calling `EventToken::signal()` from the hardware-specific completion
interrupt.

## Per-peripheral API

### UART (DMA + interrupt wait)

```cpp
#include "runtime/async_uart.hpp"

// DMA transfers.
auto op = co_await async::uart::write_dma(uart, tx_dma, buffer);
// Result<operation<dma_event::token<peripheral, signal_TX>>, ErrorCode>
```

DMA transfer-complete interrupt signals the token. `read_dma` mirrors it
with `signal_RX`.

**Single interrupt-event wait** — new, for IDLE-line / LIN-break / TC
patterns that do not involve DMA but need to suspend on a UART interrupt:

```cpp
#include "runtime/async_uart.hpp"
#include "runtime/uart_event.hpp"  // uart_event::token<P, Kind>

using alloy::hal::uart::InterruptKind;

// Arm IDLE-line interrupt, start DMA, wait for end-of-frame.
auto idle_op = async::uart::wait_for<InterruptKind::IdleLine>(uart);
uart.read_dma(rx_channel, rx_buf);
idle_op->wait_for<SysTickSource>(time::Duration::from_millis(100));
uart.disable_interrupt(InterruptKind::IdleLine);  // disarm
```

`wait_for<Kind>(port)` resets `uart_event::token<P, Kind>`, calls
`port.enable_interrupt(Kind)`, then returns
`operation<uart_event::token<P, Kind>>`. The vendor ISR calls `token::signal()`.

See [UART.md](UART.md) for the full per-vendor capability matrix and
the `InterruptKind` enum.

### SPI (DMA)

```cpp
#include "runtime/async_spi.hpp"

auto w = async::spi::write_dma(spi, tx_dma, tx_buf);    // signal_TX
auto r = async::spi::read_dma(spi, rx_dma, rx_buf);     // signal_RX
auto x = async::spi::transfer_dma(spi, tx_dma, rx_dma,  // both → signal_RX
                                   tx_buf, rx_buf);
```

`transfer_dma` enforces (via `static_assert`) that both DMA channels
target the same SPI peripheral. The completion token of the full-duplex
transfer is the RX-side `dma_event::token`; the RX DMA's
transfer-complete interrupt is what the application is waiting on
(TX completes earlier, RX is the bottleneck).

`async::spi::wait_for<Kind>(port)` is the interrupt-driven sibling — a
coroutine `co_await`s a typed `InterruptKind` (Rxne, ModeFault,
CrcError, …) without polling. The vendor ISR calls
`spi_event::token<P, Kind>::signal()` when the interrupt fires. See
[SPI.md](SPI.md) for the per-vendor capability matrix and the
`InterruptKind` enum.

### I2C (interrupt-driven)

```cpp
#include "runtime/async_i2c.hpp"

auto w  = async::i2c::write     <PeripheralId::I2C1>(i2c, addr, tx);
auto r  = async::i2c::read      <PeripheralId::I2C1>(i2c, addr, rx);
auto wr = async::i2c::write_read<PeripheralId::I2C1>(i2c, addr, tx, rx);
```

I2C uses interrupt-driven completion — DMA is not universally available
across STM32G0 / SAME70 / ESP32 I2C controllers. The event interrupt
(BTF / TC on STM32, TXCOMP / RXRDY on SAME70 TWIHS) signals the
`i2c_event::token<Peripheral>`. NACK and bus-error are reported as
`core::ErrorCode::Nack` / `BusError` from the await result.

### ADC

```cpp
#include "runtime/async_adc.hpp"

auto single = async::adc::read    <PeripheralId::ADC1>(adc);
auto scan   = async::adc::scan_dma<PeripheralId::ADC1>(adc, dma_ch, samples);
```

`read` waits on the EOC (end-of-conversion) interrupt of a single-channel
single conversion. `scan_dma` waits on the DMA transfer-complete
interrupt of a multi-channel scan, with each channel's sample landing in
the caller's buffer.

See [ADC.md](ADC.md) for the full per-vendor capability matrix, sequence
configuration, and the hardware trigger recipe.

### Timer

```cpp
#include "runtime/async_timer.hpp"

auto p = async::timer::wait_period<PeripheralId::TIM3>(tim);
auto d = async::timer::delay      <PeripheralId::TIM3>(
    tim, time::Duration::from_millis(10));
```

`wait_period` suspends until the next periodic update event from a
timer that the caller has pre-configured. `delay` reprograms the timer
to fire once after the requested duration. This is **distinct** from
`runtime::tasks::delay(Duration)` — that uses SysTick, which is shared
with the cooperative scheduler. Timer-driven async delay frees the
SysTick budget for the scheduler itself.

### GPIO edge

```cpp
#include "runtime/async_gpio.hpp"

auto e = async::gpio::wait_edge<PinId::PA0>(gpio, async::gpio::Edge::Falling);
```

Configures the pin's external-interrupt line for the requested edge
(`Rising`, `Falling`, `Both`), arms it, and returns an awaitable.
The ISR (EXTI on STM32, PIO on SAME70) signals
`gpio_event::token<Pin>::signal()`. Debounce — if needed — is
configured at the `hal::gpio` layer; the runtime wrapper itself does
not implement a re-arm gap.

### Watchdog (early-warning, crash-state capture)

```cpp
#include "runtime/async_watchdog.hpp"

auto op = async::watchdog::wait_for<
    hal::watchdog::InterruptKind::EarlyWarning>(wdg);
```

`wait_for<EarlyWarning>` is for **crash-state capture only** — a
coroutine running below the EWI handler's priority gets a bounded
window to dump state to a backup register before the watchdog resets
the system. Recovery from a watchdog timeout defeats the watchdog's
safety guarantee. See [WATCHDOG.md](WATCHDOG.md) for the per-vendor
capability matrix.

## Composing with the scheduler

```cpp
#include "async.hpp"
#include "runtime/tasks/scheduler.hpp"

tasks::Task sensor_loop() {
    while (true) {
        auto reading = co_await async::adc::read<PeripheralId::ADC1>(adc);
        // …process reading…
        co_await async::timer::delay<PeripheralId::TIM3>(
            tim, time::Duration::from_millis(50));
    }
}
```

Multiple tasks can each `co_await` distinct async operations
concurrently. The scheduler ticks all of them — whichever token is
signalled first resumes its task. There is no preemption, no priority
inversion, and no per-task stack: every awaiter is a coroutine frame
in the pool.

## Error handling

Every async function returns a `core::Result`. Two kinds of failure:

- **Start-time failure**: the HAL refused to begin the operation (e.g.
  invalid configuration, bus busy, descriptor not configured).
  Surfaces in the outer `core::Result<operation<…>, ErrorCode>`.
- **Completion-time failure**: the operation was launched but the
  hardware reported an error (NACK, bus error, parity, framing,
  timeout). Surfaces from the awaiter when the task resumes —
  `co_await` returns `Result<void, ErrorCode>` itself.

Always pattern-match both layers:

```cpp
auto op = async::i2c::write<…>(i2c, addr, payload);
if (op.is_err()) return op.err();             // start failed
auto done = co_await std::move(op).value();
if (done.is_err()) return done.err();          // completion failed
```

## Timeouts

The `operation<T>` type supports `wait_for<TimeSource>(Duration)`:

```cpp
auto op = async::uart::write_dma(uart, tx_dma, buf);
if (op.is_err()) return op.err();
auto r = op.value().template wait_for<MyTimeSource>(time::Duration::from_millis(50));
if (r.is_err()) {
    // Timeout — cancel the in-flight DMA at the HAL layer if needed.
}
```

The cooperative scheduler also supports `runtime::tasks::until(predicate)`
for compositional timeouts that combine a deadline with an arbitrary
condition.

## ISR wiring (vendor backend)

The runtime wrappers are deliberately HAL-handle-duck-typed. Each vendor
backend (STM32, SAME70, ESP32) implements the per-peripheral
`*_async()` methods that the wrappers dispatch into, and arms the
corresponding interrupt. From the ISR, the backend calls

```cpp
runtime::dma_event::token<Peripheral, Signal>::signal();    // DMA path
runtime::i2c_event::token<Peripheral>::signal();            // I2C
runtime::adc_event::token<Peripheral>::signal();            // ADC
runtime::timer_event::token<Peripheral>::signal();          // Timer
runtime::gpio_event::token<Pin>::signal();                  // GPIO edge
```

The token's static state is unique per (peripheral kind, peripheral id /
pin id) tuple, so multiple async waits on different instances do not
alias.

> **Status note:** The runtime headers + token types ship in this change.
> The per-vendor `*_async()` HAL method implementations and ISR signal
> hooks land in a focused follow-up (`add-async-hal-vendor-isr-hooks`)
> per board. Until then, `async_uart` is the only end-to-end-validated
> async path; SPI / I2C / ADC / Timer / GPIO compile clean against
> mock handles and are ready for the per-vendor wiring pass.

## Constraints

- Heap-free, RTOS-free, `noexcept` throughout.
- Each async header is opt-in: blocking-only builds link no async symbols.
- Token state is per (peripheral kind, peripheral id / pin id); no
  global aliasing.
- The `same70-zero-overhead` gate continues to forbid importing
  `async_*.hpp` from the gate build — verified by the gate's
  `#include` scanner.

## Follow-ups

- Per-vendor ISR signal hooks for STM32 and SAME70 SPI / I2C / ADC /
  Timer / GPIO → `add-async-hal-vendor-isr-hooks`.
- `examples/async_*_*` end-to-end demos → land with the per-vendor
  hooks; need real ISR wiring to demonstrate.
- Hardware spot-checks on representative boards.
- `async-bloat-check` CI job → lands with the examples (needs them
  to build to measure).
- Cookbook entries (concurrent SPI + I2C, ADC sampling loop,
  button-driven state machine, timer-accurate PWM, UART echo with
  timeout) → `docs/COOKBOOK.md` follow-up.
