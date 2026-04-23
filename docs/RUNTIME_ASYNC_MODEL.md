# Runtime Async Model

`alloy` now exposes one layering for time, completion events, and optional async adaptation.

## Time

Public header: `src/time.hpp`.

- `Duration`: portable interval type
- `Instant`: monotonic timestamp
- `Deadline`: absolute timeout target
- `time::source<TickSource>`: selected clock source

Canonical example:

- [examples/time_probe/main.cpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/time_probe/main.cpp)

That example shows two common patterns on the same source:

- periodic scheduling with `sleep_until(next_tick)`
- timeout windows with `deadline_after(...)` and `expired(...)`

## Event-Driven Completion

Public header: `src/event.hpp`.

The public HAL does not switch to a second event-specific API. The operation stays on the HAL surface and completion is observed through a typed token.

Representative UART+DMA flow:

```cpp
auto uart = board::make_debug_uart();
auto tx_dma = board::make_debug_uart_tx_dma({
    .direction = alloy::hal::dma::Direction::memory_to_peripheral,
});

using TxCompletion = alloy::dma_event::token<
    board::DebugUartTxDma::peripheral_id,
    board::DebugUartTxDma::signal_id>;

TxCompletion::reset();
uart.write_dma(tx_dma, payload);
TxCompletion::wait_for<BoardTime>(alloy::time::Duration::from_millis(50));
```

Canonical example:

- [examples/dma_probe/main.cpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/dma_probe/main.cpp)

Generated interrupt stubs signal these tokens through the internal interrupt bridge.

## Optional Async Adapters

The async layer is optional and executor-agnostic.

- public header: `src/async.hpp`

It wraps existing HAL operations instead of replacing them.

```cpp
const auto operation = alloy::async::uart::write_dma(uart, tx_dma, payload);
if (operation.is_ok() && operation.unwrap().poll() == alloy::async::poll_status::pending) {
    // integrate with a scheduler, callback loop, or custom executor
}
```

Blocking-only builds do not need these headers and do not pay for them in the hot blocking path.

## Low Power And Wakeup

Public header: `src/low_power.hpp`.

- `enter(mode)` applies the selected low-power mode and restores the previous CPU sleep-depth state on wakeup
- `set_hooks(...)` installs runtime callbacks around low-power entry and wakeup
- `time_valid_in<T>(mode)` and `completion_valid_in<T>(mode)` express whether a time source or completion path remains valid in that mode
- `wakeup_source_valid_in(mode, source)` checks published wake-capable sources from the selected device contract
- `wakeup_token<source>` exposes a typed completion token for a published wake-capable source

Current default policy is intentionally conservative:

- non-`SLEEPDEEP` modes keep runtime time/event paths valid
- `SLEEPDEEP` modes require an explicit wake-capable source or a future specialization for the relevant time/event source
- published wakeup sources are treated as deep-sleep-valid completion tokens through `wakeup_token<...>`

Representative validation:

- compile smoke in `tests/compile_tests/test_runtime_low_power_api.cpp`
- host-MMIO wakeup-capable path in `tests/host_mmio/scenarios/test_same70_bringup.cpp`

## Recommended Use

- use blocking HAL calls by default
- use typed completion tokens when the transfer already has a natural interrupt or DMA completion path
- add `src/async.hpp` only when the application needs polling or executor integration on top of the same HAL operation
