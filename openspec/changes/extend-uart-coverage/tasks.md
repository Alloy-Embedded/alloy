# Tasks: Extend UART Coverage

Phases ordered. Phases 1-4 are host-testable (compile + concept).
Phase 5 needs the existing 3-board hardware matrix
(SAME70 / STM32G0 / STM32F4) — same boards that already validate
`uart` at the foundational tier today.

## 1. Baudrate / oversampling / kernel clock

- [x] 1.1 `set_baudrate(std::uint32_t bps)` — resolves BRR from the
      peripheral's current kernel clock + chosen oversampling.
      Returns `core::ErrorCode::InvalidArgument` when the resulting
      BRR overflows 16 bits or the realised baud rate falls outside
      ±2 % of the requested rate.
- [x] 1.2 `enum class Oversampling { X16, X8 }` +
      `set_oversampling(Oversampling)` — gated on
      `kBaudOversamplingOptions.size() > 1`.
- [x] 1.3 `kernel_clock_hz() -> std::uint32_t` — reads the resolved
      kernel-clock value through the
      `KernelClockSourceOption.frequency_hz` lookup.
- [ ] 1.4 `set_kernel_clock_source(KernelClockSource)` — gated on
      non-empty `kKernelClockSourceOptions`. Validates the choice
      against the published options list at call time and returns
      `NotSupported` when the requested source isn't in the set.

## 2. FIFO + status flags + interrupts

- [x] 2.1 `enum class FifoTrigger { Empty, Quarter, Half,
      ThreeQuarters, Full }`. `set_tx_threshold(FifoTrigger)` /
      `set_rx_threshold(FifoTrigger)` — gated on `kFifoDepth > 0`
      and `kFifoTriggerFractionsQ8` membership; the HAL clamps to
      the published fraction set.
- [x] 2.2 `enable_fifo(bool)`, `tx_fifo_full() -> bool`,
      `rx_fifo_empty() -> bool`, `rx_fifo_threshold_reached() -> bool`.
- [x] 2.3 Status flag accessors (gated per-field):
      `tx_complete()`, `tx_register_empty()`,
      `rx_register_not_empty()`, `parity_error()`,
      `framing_error()`, `noise_error()`, `overrun_error()`.
      Each has its `clear_*` mirror where the descriptor publishes
      a clear-side field.
- [x] 2.4 `enum class InterruptKind { Tc, Txe, Rxne, IdleLine,
      LinBreak, Cts, Error, RxFifoThreshold, TxFifoThreshold }`.
      `enable_interrupt(InterruptKind)` /
      `disable_interrupt(InterruptKind)` — each kind gated on the
      corresponding control-side field.
- [ ] 2.5 `irq_numbers() -> std::span<const std::uint32_t>` —
      static accessor returning the descriptor's `kIrqNumbers`.

## 3. LIN / RS-485 DE / half-duplex / smartcard / IrDA / multiprocessor / wakeup

- [x] 3.1 LIN: `enable_lin(bool)`, `send_lin_break()`,
      `lin_break_detected() -> bool`, `clear_lin_break_flag()`.
- [x] 3.2 RS-485 DE: `enable_de(bool)`,
      `set_de_assertion_time(std::uint8_t sample_times)`,
      `set_de_deassertion_time(std::uint8_t)` — gated on the DE
      field group.
- [x] 3.3 `set_half_duplex(bool)` — gated on the HDSEL field.
- [x] 3.4 `set_smartcard_mode(bool)` /
      `set_irda_mode(bool)` — independent gates.
- [ ] 3.5 Multiprocessor: `set_address(std::uint8_t addr,
      AddressLength len)`, `mute_until_address(bool)` —
      `AddressLength` is `Bits4` or `Bits7`.
- [ ] 3.6 `enum class WakeupTrigger { AddressMatch, RxneNonEmpty,
      StartBit }` + `enable_wakeup_from_stop(WakeupTrigger)` —
      gated on the UESM / WUS fields.

## 4. Compile tests

- [x] 4.1 Extend `tests/compile_tests/test_uart_api.cpp` to
      instantiate every new method against the existing
      `nucleo_g071rb` device contract; confirm `static_assert`
      fires on out-of-range `Oversampling` selection.
- [x] 4.2 Add a `nucleo_f401re`-targeted compile test exercising
      the LIN enabled path (USART2 LIN supported on F4).
- [ ] 4.3 Add a `same70_xplained`-targeted compile test exercising
      the per-peripheral `irq_numbers()` accessor (SAME70 publishes
      multiple IRQ lines per peripheral).

## 5. Async integration

- [x] 5.1 `async::uart::wait_for<Kind>(port)` — new uart_event.hpp token
      + async_uart.hpp wrapper; resets token, arms IRQ, returns operation.
- [x] 5.2 `tests/compile_tests/test_async_peripherals.cpp` extended
      with `wait_for<IdleLine>` and `wait_for<LinBreak>` mock compilation.

## 6. Example

- [x] 6.1 `examples/uart_probe_complete/`: targets `nucleo_g071rb`.
      Exercises all Phase 1-3 levers on USART2 (debug port): baudrate,
      oversampling, FIFO, status flags, interrupts, LIN, DE, half-duplex,
      smartcard, IrDA, async::uart::wait_for<IdleLine>. Each feature
      prints OK or NotSupported(N) — no loopback jumper required.
- [ ] 6.2 Mirror configuration on `nucleo_f401re` for USART1+USART2
      (drop the LPUART1-specific bits absent on F4).
- [ ] 6.3 Mirror on `same70_xplained` for FLEXCOM0 USART
      (drop the M1-bit-specific paths absent on SAM).

## 7. Hardware spot-check (3-board matrix)

- [ ] 7.1 SAME70 Xplained: run `uart_probe_complete`. Verify TX/RX
      at 921600 with 0 framing errors over 60 s loopback.
- [ ] 7.2 STM32G0 Nucleo: same matrix.
- [ ] 7.3 STM32F4 Nucleo: same matrix.
- [ ] 7.4 Update `docs/SUPPORT_MATRIX.md` `uart` row to record the
      extended-coverage validation (single-line note + link to
      `docs/UART.md`).

## 8. Documentation

- [ ] 8.1 `docs/UART.md` — comprehensive guide: model, baudrate /
      FIFO / kernel-clock recipes, LIN / RS-485 / smartcard / IrDA
      recipes, multiprocessor / wakeup recipe, error handling,
      async wiring, modm migration table, per-vendor capability
      matrix.
- [ ] 8.2 Reference `docs/UART.md` from `docs/ASYNC.md` (under the
      UART section) and `docs/COOKBOOK.md` (where applicable).
- [ ] 8.3 Cross-link from `docs/SUPPORT_MATRIX.md` `uart` row.

## 9. Out-of-scope follow-ups (filed but not done in this change)

- [ ] 9.1 File alloy-codegen `add-uart-clock-source-typed-enum`
      (mirror of `add-adc-channel-typed-enum`) so the HAL drops
      runtime `KernelClockSource` validation in favour of compile-
      time typed `UartClockSourceOf<P>::type`.
- [ ] 9.2 File alloy `add-uart-idle-line-rx-dma` example —
      canonical "receive unknown-length frame" recipe combining
      `wait_for(IdleLine)` with circular RX DMA.
- [ ] 9.3 File alloy `add-uart-validate-frame-config` — typed
      negative-matrix that rejects nonsensical 9-bit + parity
      combinations at compile time.
