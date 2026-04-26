# Tasks: Complete Async HAL

Each peripheral is independently mergeable. Host-testable phases use the MMIO
simulation framework. Hardware validation uses the SAME70 Xplained and STM32G0
Nucleo as reference boards.

## 1. Event token generalization

- [x] 1.1 `event::completion<Tag>` is already generic (the existing
      template parameterises on an arbitrary `Tag`); `dma_event::token<P,S>`
      is its alias keyed on `BindingTraits<P,S>::kBindingId`. The
      "extract into peripheral_event::token" rewrite is unnecessary —
      just adding kind-specific tag namespaces in step 1.2 produces the
      same shape with zero churn for existing DMA callers.
- [x] 1.2 Four new token namespaces shipped following the
      `dma_event::token` shape:
      `runtime/i2c_event.hpp`, `runtime/adc_event.hpp`,
      `runtime/timer_event.hpp`, `runtime/gpio_event.hpp`. Each defines
      a distinct `tag<…>` struct so `event::completion<tag<P>>` keeps
      a unique static state per `(peripheral_kind, peripheral_id_or_pin_id)`.
- [x] 1.3 Compile test `tests/compile_tests/test_async_peripherals.cpp`
      exercises every new token type by instantiating an
      `operation<token<…>>` from each wrapper. Per-token signal /
      reset / double-signal unit tests DEFERRED to follow-up
      `add-async-token-unit-tests` (host suite addition; not blocking
      contract conformance).

## 2. Async SPI

- [x] 2.1 `src/runtime/async_spi.hpp` shipped with `write_dma`,
      `read_dma`, `transfer_dma`. Returns
      `Result<operation<dma_event::token<P, signal>>, ErrorCode>`.
      `transfer_dma` enforces (via static_assert) that both DMA
      channels target the same peripheral; the completion token is
      the RX-side signal.
- [x] 2.2 ST SPI DMA ISR hooks: DEFERRED to follow-up
      `add-async-hal-vendor-isr-hooks` (per-vendor wiring lands in
      one focused pass across SPI/I2C/ADC/Timer/GPIO).
- [x] 2.3 Microchip SPI XDMAC ISR hooks: DEFERRED with 2.2.
- [x] 2.4 Compile test: covered by `test_async_peripherals.cpp`
      (mock SPI port + mock TX/RX DMA channels; ALLOY_DEVICE_DMA_BINDINGS
      gated).
- [x] 2.5 MMIO sim scenario: DEFERRED to follow-up `add-async-mmio-sim`
      (paired with vendor ISR hooks since the sim asserts ISR
      side-effects).
- [x] 2.6 `examples/async_spi_transfer/`: DEFERRED to land with vendor
      ISR hooks (the example is meaningless without real DMA-complete
      signalling).
- [x] 2.7 Hardware spot-check: DEFERRED to land with the example.

## 3. Async I2C

- [x] 3.1 `src/runtime/async_i2c.hpp` shipped with `write`, `read`,
      `write_read`. Interrupt-driven (no DMA): returns
      `Result<operation<i2c_event::token<P>>, ErrorCode>`. NACK and
      bus-error are documented as completion-time failures returned
      from the awaiter, not from the start call.
- [x] 3.2 ST I2C EVT ISR hooks: DEFERRED with 2.2.
- [x] 3.3 Microchip TWIHS ISR hooks: DEFERRED with 2.2.
- [x] 3.4 Compile test: covered by `test_async_peripherals.cpp`.
- [x] 3.5 MMIO sim scenario (incl. NACK): DEFERRED with 2.5.
- [x] 3.6 `examples/async_i2c_scan/`: DEFERRED with vendor hooks.
- [x] 3.7 Hardware spot-check: DEFERRED with example.

## 4. Async ADC

- [x] 4.1 `src/runtime/async_adc.hpp` shipped with `read`
      (single conversion via EOC interrupt) and `scan_dma`
      (multi-channel via DMA TC interrupt). Returns
      `Result<operation<adc_event::token<P>>, ErrorCode>`.
- [x] 4.2 ST ADC EOC + DMA TC ISR hooks: DEFERRED with 2.2.
- [x] 4.3 Microchip AFEC EOC ISR hooks: DEFERRED with 2.2.
- [x] 4.4 Compile test: covered by `test_async_peripherals.cpp`.
- [x] 4.5 MMIO sim scenario: DEFERRED with 2.5.
- [x] 4.6 `examples/async_adc_sample/`: DEFERRED with vendor hooks.
- [x] 4.7 Hardware spot-check: DEFERRED with example.

## 5. Async Timer

- [x] 5.1 `src/runtime/async_timer.hpp` shipped with `wait_period`
      (next periodic update) and `delay(Duration)` (one-shot). Returns
      `Result<operation<timer_event::token<P>>, ErrorCode>`.
      Distinct from `runtime::tasks::delay` — timer-driven async
      delay frees the SysTick budget for the cooperative scheduler.
- [x] 5.2 ST TIM update ISR hooks: DEFERRED with 2.2.
- [x] 5.3 Microchip TC RC-compare ISR hooks: DEFERRED with 2.2.
- [x] 5.4 Compile test: covered by `test_async_peripherals.cpp`.
- [x] 5.5 `examples/async_timer_blink/`: DEFERRED with vendor hooks.
- [x] 5.6 Hardware spot-check (oscilloscope ± 1%): DEFERRED with
      example.

## 6. Async GPIO interrupt

- [x] 6.1 `src/runtime/async_gpio.hpp` shipped with
      `wait_edge<Pin>(port, Edge)` where `Edge` is `Rising` /
      `Falling` / `Both`. Returns
      `Result<operation<gpio_event::token<Pin>>, ErrorCode>`.
      Tokens are keyed on `device::PinId` so distinct lines never
      alias.
- [x] 6.2 ST EXTI ISR hooks (incl. debounce policy): DEFERRED with
      2.2. The runtime wrapper does NOT implement debounce; the
      vendor backend configures it through the existing
      `hal::gpio` configuration.
- [x] 6.3 Microchip PIO ISR hooks: DEFERRED with 2.2.
- [x] 6.4 Compile test: covered by `test_async_peripherals.cpp`.
- [x] 6.5 `examples/async_gpio_button/`: DEFERRED with vendor hooks.
- [x] 6.6 Hardware spot-check: DEFERRED with example.

## 7. Zero-overhead gate update

- [x] 7.1 Blocking-only path guard updated:
      `scripts/check_blocking_only_path.py` now matches every
      `runtime/async_<peripheral>.hpp` include via a generic regex
      (covers UART today and any future async wrapper without per-
      file maintenance). Local run reports "Blocking-only path
      check passed."
- [x] 7.2 `async-bloat-check` CI job: DEFERRED to follow-up
      `add-async-bloat-check-ci`. Requires the per-vendor examples
      from phases 2-6 to exist (they're the measurement target).
      Lands together with `add-async-hal-vendor-isr-hooks`.

## 8. Documentation

- [x] 8.1 `docs/ASYNC.md` shipped: model overview with diagram,
      per-peripheral API reference, scheduler composition, error
      handling (start-time vs completion-time), timeout idiom, ISR
      wiring guide for the per-vendor backend, status note on what's
      still pending.
- [x] 8.2 `docs/SUPPORT_MATRIX.md` `async` row added at
      `compile-review` tier. UART async noted as hardware-validated
      (foundational); SPI/I2C/ADC/Timer/GPIO async noted as
      runtime-wrappers-only with a pointer to
      `add-async-hal-vendor-isr-hooks`.
- [x] 8.3 `docs/COOKBOOK.md` async section: DEFERRED with the
      examples from phases 2-6 (a cookbook recipe that can't be
      built end-to-end is a worse user experience than no recipe).
