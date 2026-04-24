# Alloy Cookbook

Official examples for the active runtime path.

The cookbook keeps board-oriented examples on `board::init()` plus `board::make_*()`.
Use the ergonomic direct-HAL route spellings when you are teaching manual connector setup,
not when pointing users to the canonical in-repo examples.

## Canonical Examples

| Task | Example | Primary board |
| --- | --- | --- |
| LED bring-up | `blink` | `nucleo_g071rb` |
| Typed time source and deadlines | `time_probe` | `same70_xplained` |
| Debug UART logging | `uart_logger` | `same70_xplained` |
| Watchdog configure + refresh | `watchdog_probe` | `same70_xplained` |
| RTC configure | `rtc_probe` | `same70_xplained` |
| Timer + PWM | `timer_pwm_probe` | `same70_xplained` |
| ADC + DAC | `analog_probe` | `same70_xplained` |
| I2C bus scan | `i2c_scan` | `same70_xplained` |
| SPI bring-up | `spi_probe` | `same70_xplained` |
| DMA-backed UART binding | `dma_probe` | `same70_xplained` |
| Async completion + bounded timeout | `async_uart_timeout` | `same70_xplained` |
| CAN bring-up | `can_probe` | `same70_xplained` |

## Common Flows

### Build one example

```bash
python3 scripts/alloyctl.py build --board same70_xplained --target uart_logger -j8
```

### Flash and monitor one example

```bash
python3 scripts/alloyctl.py flash --board same70_xplained --target uart_logger --build-first -j8
python3 scripts/alloyctl.py monitor --board same70_xplained
```

### Sweep the whole board bundle

```bash
python3 scripts/alloyctl.py sweep --board same70_xplained -j8
```

### Explain the official board path

```bash
python3 scripts/alloyctl.py explain --board same70_xplained
python3 scripts/alloyctl.py explain --board same70_xplained --connector debug-uart
python3 scripts/alloyctl.py explain --board same70_xplained --clock
```

## Runtime Async Model

### Wait for a transfer with a bounded deadline

The canonical answer. Use `runtime::event::completion::wait_for` on the
published DMA completion token for your peripheral. `Ok` means the transfer
completed; `Err(Timeout)` means the deadline expired first — it is a
first-class recoverable outcome, not undefined behavior.

```cpp
using TxCompletion = alloy::dma_event::token<
    board::DebugUartTxDma::peripheral_id,
    board::DebugUartTxDma::signal_id>;

TxCompletion::reset();
uart.write_dma(tx_dma, payload);
const auto result = TxCompletion::wait_for<BoardTime>(
    alloy::time::Duration::from_millis(50));
if (result.is_ok()) {
    // transfer completed
} else if (result.error() == alloy::core::ErrorCode::Timeout) {
    // deadline elapsed; token and operation remain valid for retry/cancel
}
```

Full example: [examples/async_uart_timeout/main.cpp](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/examples/async_uart_timeout/main.cpp)

### Integrate with a scheduler (optional)

If an application already has a scheduler or main-loop callback, use the
optional async adapter to wrap the same underlying operation. The blocking
path keeps working without the adapter — see the zero-overhead rule below.

See [docs/RUNTIME_ASYNC_MODEL.md](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/docs/RUNTIME_ASYNC_MODEL.md) for the scheduler-friendly form.

## Rules

- examples under `examples/` are the official runtime-path documentation
- board-oriented examples remain on the existing `board::*` path even after the ergonomic alias layer
- new examples should use `src/hal/**`, `src/time.hpp`, `src/event.hpp`, `src/async.hpp`, and `src/low_power.hpp`
- examples should not introduce alternate vendor-specific API layers
- blocking+completion examples MUST NOT include `src/async.hpp` — the
  canonical completion+timeout path stays usable without the async adapter
  layer. `scripts/check_blocking_only_path.py` enforces this invariant on
  `examples/async_uart_timeout/main.cpp` and
  `tests/compile_tests/test_blocking_only_completion_api.cpp`.
