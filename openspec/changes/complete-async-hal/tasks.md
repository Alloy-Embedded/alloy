# Tasks: Complete Async HAL

Each peripheral is independently mergeable. Host-testable phases use the MMIO
simulation framework. Hardware validation uses the SAME70 Xplained and STM32G0
Nucleo as reference boards.

## 1. Event token generalization

- [ ] 1.1 Extract `dma_event::token` into a generic `peripheral_event::token<Tag>`
      template in `src/runtime/event.hpp`. `dma_event::token` becomes a thin alias.
      No API change for existing callers.
- [ ] 1.2 Add `i2c_event`, `adc_event`, `timer_event`, `gpio_event` token type aliases
      in their respective `src/runtime/async_*.hpp` headers.
- [ ] 1.3 `tests/unit/test_async.cpp`: extend with token generalization coverage —
      signal, reset, and double-signal guard for each new token type.

## 2. Async SPI

- [ ] 2.1 Create `src/runtime/async_spi.hpp`: `async::spi::write_dma`,
      `read_dma`, `transfer_dma` following the existing async UART pattern.
- [ ] 2.2 Implement ISR hooks for ST SPI DMA completion (DMA TX + RX complete
      interrupts signal the corresponding tokens).
- [ ] 2.3 Implement ISR hooks for Microchip SPI DMA completion (SAME70 XDMAC
      TC interrupt).
- [ ] 2.4 `tests/compile_tests/test_async_spi.cpp`: compile check — open SPI handle,
      call `async::spi::transfer_dma`, verify return type satisfies `operation<T>`.
- [ ] 2.5 `tests/host_mmio/`: add MMIO sim scenario for async SPI TX completion.
- [ ] 2.6 `examples/async_spi_transfer/`: two simulated SPI transfers composed with
      `co_await` in the task scheduler. Builds for `nucleo_g071rb`.
- [ ] 2.7 Hardware spot-check: run `async_spi_transfer` on SAME70 Xplained with SPI
      loopback. Verify no data corruption over 1000 transfers.

## 3. Async I2C

- [ ] 3.1 Create `src/runtime/async_i2c.hpp`: `async::i2c::write`, `read`,
      `write_read`. Interrupt-driven (not DMA) completion for all backends.
- [ ] 3.2 Implement ISR hooks for ST I2C: EVT interrupt (BTF/TC) signals the token.
      Handle NACK and bus error → `core::ErrorCode::Nack` / `BusError` in the result.
- [ ] 3.3 Implement ISR hooks for Microchip TWIHS: TXCOMP / RXRDY interrupt.
- [ ] 3.4 `tests/compile_tests/test_async_i2c.cpp`: compile check.
- [ ] 3.5 `tests/host_mmio/`: MMIO sim scenario for async I2C write completion
      including NACK simulation.
- [ ] 3.6 `examples/async_i2c_scan/`: async replacement of blocking `i2c_scan`.
      Task probes 0x00–0x7F with 50 ms timeout per address. Builds for `nucleo_g071rb`.
- [ ] 3.7 Hardware spot-check: run `async_i2c_scan` on SAME70 Xplained with AT24MAC402
      EEPROM. Verify address 0x58 detected, all others return `Nack`.

## 4. Async ADC

- [ ] 4.1 Create `src/runtime/async_adc.hpp`: `async::adc::read` (single conversion)
      and `async::adc::scan_dma` (multi-channel DMA scan).
- [ ] 4.2 Implement ISR hooks for ST ADC: EOC interrupt (single) and DMA TC interrupt
      (scan).
- [ ] 4.3 Implement ISR hooks for Microchip SAME70 AFEC: EOC interrupt.
- [ ] 4.4 `tests/compile_tests/test_async_adc.cpp`: compile check.
- [ ] 4.5 `tests/host_mmio/`: MMIO sim scenario for ADC single conversion completion.
- [ ] 4.6 `examples/async_adc_sample/`: multi-channel ADC scan running as a task,
      values posted to UART via `Channel<uint16_t, 8>`. Builds for `nucleo_g071rb`.
- [ ] 4.7 Hardware spot-check: `async_adc_sample` on NUCLEO-G071RB. Verify all
      channels read within expected voltage range (VCC/2 ± 10% on tied pins).

## 5. Async Timer

- [ ] 5.1 Create `src/runtime/async_timer.hpp`: `async::timer::wait_period` and
      `async::timer::delay`.
- [ ] 5.2 Implement ISR hooks for ST TIM: update interrupt signals the token.
- [ ] 5.3 Implement ISR hooks for Microchip TC: RC compare interrupt.
- [ ] 5.4 `tests/compile_tests/test_async_timer.cpp`: compile check.
- [ ] 5.5 `examples/async_timer_blink/`: LED blink driven by hardware timer period
      await, not SysTick delay. Demonstrates timer-accurate ~1 Hz blink.
      Builds for `nucleo_g071rb`.
- [ ] 5.6 Hardware spot-check: measure period accuracy on oscilloscope ± 1%.

## 6. Async GPIO interrupt

- [ ] 6.1 Create `src/runtime/async_gpio.hpp`: `async::gpio::wait_edge<Pin>(edge)`.
- [ ] 6.2 Implement ISR hooks for ST EXTI: line-matched ISR signals the token.
      Handle debounce: a re-arm gap of 10 ms before accepting next edge.
- [ ] 6.3 Implement ISR hooks for Microchip PIO: PIO ISR signals token on matching pin.
- [ ] 6.4 `tests/compile_tests/test_async_gpio.cpp`: compile check.
- [ ] 6.5 `examples/async_gpio_button/`: coroutine waits for button edge, toggles LED,
      loops. No polling. Builds for `nucleo_g071rb` (B1 button on PA0).
- [ ] 6.6 Hardware spot-check: button press triggers LED toggle reliably over 100
      presses with no spurious toggles.

## 7. Zero-overhead gate update

- [ ] 7.1 Verify `same70-zero-overhead` assembly gate still passes after all async
      headers are added. The gate build must not import any `async_*.hpp` — confirm
      via `#include` scanner in the gate script.
- [ ] 7.2 Add a new `async-bloat-check` CI job: builds each async example for
      `nucleo_g071rb`, asserts total `.text + .rodata + .data + .bss <= 16 KB`
      (coroutine scheduler + one peripheral async adapter budget).

## 8. Documentation

- [ ] 8.1 Create `docs/ASYNC.md`: async model overview, per-peripheral API reference,
      ISR wiring guide for each backend (STM32 + SAME70), `co_await` patterns,
      error handling, timeout recipes, and a "why not exceptions" note.
- [ ] 8.2 `docs/SUPPORT_MATRIX.md`: add `async` column per peripheral per board.
      Initial entries: UART async = validated; SPI/I2C/ADC/Timer/GPIO async =
      `compile-review` (upgraded to `hardware` after spot-checks).
- [ ] 8.3 `docs/COOKBOOK.md`: add async cookbook section with five recipes —
      concurrent SPI + I2C, ADC sampling loop, button-driven state machine,
      timer-accurate PWM, UART echo with timeout.
