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
if (const auto result = TxCompletion::wait_for<BoardTime>(alloy::time::Duration::from_millis(50));
    result.is_err() && result.error() == alloy::core::ErrorCode::Timeout) {
    // bounded: timeout is a first-class recoverable outcome — the token and
    // the underlying operation remain valid, so the caller may retry,
    // escalate, or cancel without undefined behavior.
}
```

Canonical example:

- [examples/async_uart_timeout/main.cpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/async_uart_timeout/main.cpp)

That example exercises the recommended completion+timeout path on the debug UART and
reports three distinguishable outcomes over the VCOM: `success=N`, `timeout=N`, and
`recovered=N`. It demonstrates that timeout does not leave the runtime in an undefined
state — the same token recovers on the next generous wait.

Historical bring-up reference (non-timeout path):

- [examples/dma_probe/main.cpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/dma_probe/main.cpp)

Generated interrupt stubs signal these tokens through the internal interrupt bridge.

### Timeout Is A First-Class Outcome

`runtime::event::completion::wait_for` returns `core::Result<void, core::ErrorCode>`.
The timeout outcome is `core::ErrorCode::Timeout`, which is type-level distinguishable
from a completed transfer. Callers MUST NOT treat any non-`Ok` result as a completion.

Host coverage:

- [tests/unit/test_async_completion_timeout.cpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tests/unit/test_async_completion_timeout.cpp)

That test proves both directions on a deterministic mock time source:

- signal fires before the deadline → `Ok`
- signal withheld past the deadline → `Err(Timeout)`
- same token is reusable after a timeout and completes cleanly when signalled

## Optional Async Adapters

The async layer is optional and executor-agnostic.

- public header: `src/async.hpp`

It wraps existing HAL operations instead of replacing them — the same
underlying UART+DMA operation the blocking+completion path uses.

```cpp
// Scheduler-friendly form of the canonical completion+timeout path.
auto operation = alloy::async::uart::write_dma(uart, tx_dma, payload);
if (operation.is_err()) { /* kickoff failed — handle as in the blocking path */ }

auto op = operation.unwrap();
const auto deadline = BoardTime::deadline_after(alloy::time::Duration::from_millis(50));

while (op.poll() == alloy::async::poll_status::pending) {
    if (BoardTime::expired(deadline)) {
        // same semantics as TxCompletion::wait_for returning Err(Timeout):
        // bounded, recoverable, type-level distinct from success
        break;
    }
    // yield to a scheduler, callback loop, or custom executor here
}
```

Blocking-only builds do not need these headers and do not pay for them in the hot blocking path.
The async adapter uses the same underlying `runtime::event::completion` primitive that
backs the canonical completion+timeout example, so the two forms are interchangeable
from the user's perspective.

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
- observable `enter → wait → wake` coverage on SAME70 in
  `tests/host_mmio/scenarios/test_same70_bringup.cpp`
  ("host mmio exposes SAME70 low-power coordination hooks and wake-capable sources")
  — the test asserts `before_entry`, `wait_hook`, `after_wakeup` all fire for
  `deep_sleep`, that the first published wake source is valid in that mode,
  and that the `wakeup_token` completes when signalled

Supported vs. representative today:

- supported: SAME70 `deep_sleep` with the first published wake source, validated
  by the host-MMIO test above
- representative (API present, evidence not yet expanded): any other mode or
  wake source not yet listed as a required low-power gate in
  `docs/RELEASE_MANIFEST.json`

## Recommended Use

- use blocking HAL calls by default
- use typed completion tokens when the transfer already has a natural interrupt or DMA completion path
- add `src/async.hpp` only when the application needs polling or executor integration on top of the same HAL operation
