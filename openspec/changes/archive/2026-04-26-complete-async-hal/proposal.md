# Complete Async HAL — SPI, I2C, ADC, Timer, and GPIO Interrupt

## Why

Alloy v0.1.0 introduced a three-tier concurrency model (blocking → event/completion →
coroutine scheduler) and shipped an async adapter for UART DMA. Every other peripheral
— SPI, I2C, ADC, Timer, GPIO interrupts — is blocking-only. This creates a hard wall:
the moment an application needs concurrent I2C and SPI operations, or needs to sample
ADC without stalling, it reaches for polling loops or busy-waits.

Embassy (Rust) covers every peripheral with native `async/await`. If Alloy's answer to
"can I read an SPI device without blocking the scheduler?" is "no", it loses to Embassy
and to any RTOS with DMA interrupt support.

The async model Alloy already has is the right one: a DMA transfer starts the operation,
an ISR signals a `completion<Tag>` event, and the awaitable returns `Result<T>`.
This change extends that model uniformly across the remaining peripherals.

## What Changes

### `src/hal/spi/` — async SPI

`src/runtime/async_spi.hpp`:
- `async::spi::write_dma<Connector>(handle, span) → operation<dma_event::token<...>>`
- `async::spi::read_dma<Connector>(handle, span) → operation<dma_event::token<...>>`
- `async::spi::transfer_dma<Connector>(handle, tx, rx) → operation<dma_event::token<...>>`

TX-complete and RX-complete DMA interrupts signal the corresponding `completion<Tag>`
events. The blocking SPI API is unchanged. The async adapter is a separate include and
adds zero cost when not used.

### `src/hal/i2c/` — async I2C

`src/runtime/async_i2c.hpp`:
- `async::i2c::write<Connector>(handle, addr, span) → operation<i2c_event::token<...>>`
- `async::i2c::read<Connector>(handle, addr, span) → operation<i2c_event::token<...>>`
- `async::i2c::write_read<Connector>(handle, addr, tx, rx) → operation<...>`

I2C on STM32 and SAME70 uses interrupt-driven (not DMA) completion — the peripheral
raises BTF/TC interrupt on transfer complete. The ISR signals the event token.
The `i2c_event::token<PeripheralId>` type mirrors `dma_event::token`.

### `src/hal/adc/` — async ADC

`src/runtime/async_adc.hpp`:
- `async::adc::read<PeripheralId>(handle) → operation<adc_event::token<...>>`

ADC end-of-conversion interrupt signals the token. The async adapter supports both
single-conversion and DMA-scan modes. In scan mode, the operation completes when all
channels have been sampled (DMA transfer-complete interrupt).

### `src/hal/timer/` — async Timer

`src/runtime/async_timer.hpp`:
- `async::timer::wait_period<PeripheralId>(handle) → operation<timer_event::token<...>>`
- `async::timer::delay<PeripheralId>(handle, duration) → operation<timer_event::token<...>>`

Timer update interrupt signals the token. Enables tasks to suspend for a hardware-timed
period without consuming the SysTick `delay(Duration)` slot.

### `src/hal/gpio/` — async GPIO interrupt

`src/runtime/async_gpio.hpp`:
- `async::gpio::wait_edge<Pin>(edge) → operation<gpio_event::token<Pin>>`
  where `edge` is `Rising`, `Falling`, or `Both`.

The external interrupt line (EXTI on STM32, PIO interrupt on SAME70) signals the token.
Enables tasks to wait for a button press or sensor data-ready signal without polling.

### `src/runtime/event.hpp` — token type generalization

The existing `completion<Tag>` and `dma_event::token<P, Signal>` pattern is extended
with `i2c_event::token`, `adc_event::token`, `timer_event::token`, and
`gpio_event::token` — all following the same interface so `operation<T>` composes them
identically.

### Examples

- `examples/async_spi_transfer/`: two SPI devices (flash + sensor) interleaved via the
  coroutine scheduler. Shows that concurrent SPI accesses compose correctly with
  `co_await`.
- `examples/async_i2c_scan/`: replaces the blocking `i2c_scan` with a coroutine that
  reads all 127 addresses using `async::i2c::write` without stalling other tasks.
- `examples/async_adc_sample/`: ADC multi-channel scan in DMA mode, result posted to a
  UART task via `Channel<uint16_t, N>`.
- `examples/async_gpio_button/`: coroutine wakes on GPIO edge (button press) and
  toggles LED. No polling loop.

### Documentation

`docs/ASYNC.md` — comprehensive guide: async model overview, per-peripheral API
reference, ISR wiring guide (how to hook the interrupt to the event token for each
backend), `co_await` patterns, error handling, timeout recipes.

## What Does NOT Change

- Blocking HAL APIs for all peripherals — zero regression.
- The zero-overhead gate (`same70-zero-overhead`) — async adapters are separate includes;
  the blocking path must not import them.
- `src/runtime/async_uart.hpp` — already exists; this change adds siblings only.

## Alternatives Considered

**DMA-only async for all peripherals:** DMA is not available on every peripheral on
every chip (e.g., I2C on some STM32G0 variants does not support DMA). Interrupt-driven
completion is the correct fallback and maps to the same event token model.

**Coroutine-first design (no blocking fallback):** Would break every existing example
and the foundational validation suite. The tier model (blocking → async) is intentional
and non-negotiable.
