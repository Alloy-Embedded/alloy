# Tasks: Extend CAN Coverage

## 1. FD timing + test modes + frame primitives

- [ ] 1.1 `set_data_timing(prescaler, sjw, tseg1, tseg2)` — gated
      on `kHasFlexibleDataRate`.
- [ ] 1.2 `enum class TestMode { Normal, BusMonitor,
      LoopbackInternal, LoopbackExternal, Restricted }`.
      `set_test_mode(TestMode)` — gated on the descriptor's TEST/
      bus-monitor field.
- [ ] 1.3 `struct CanFrame` (id, extended_id, remote_frame,
      fd_format, bit_rate_switch, dlc, data).
      `transmit(CanFrame)`, `receive(CanFrame& out, FifoId fifo)`.

## 2. Filter banks + error counters + bus recovery

- [ ] 2.1 `enum class FilterTarget { Rxfifo0, Rxfifo1,
      RejectMatching, AcceptMatching }`.
      `set_filter_standard(bank, id, mask, target)`,
      `set_filter_extended(bank, id, mask, target)` — gated on
      `kGlobalFilterRegister.valid` /
      `kExtendedFilterConfigRegister.valid`. Out-of-range bank
      returns `InvalidArgument`.
- [ ] 2.2 `tx_error_count()`, `rx_error_count()`, `bus_off()`,
      `error_passive()`, `error_warning()` — gated on
      `kErrorCounterRegister.valid`.
- [ ] 2.3 `request_bus_off_recovery() -> Result<void, ErrorCode>`.

## 3. Interrupts + IRQ vector

- [ ] 3.1 `enum class InterruptKind { Tx, RxFifo0, RxFifo1, Error,
      BusOff, ErrorWarning, ErrorPassive, Wakeup }`.
      `enable_interrupt` / `disable_interrupt` — per-kind gated.
- [ ] 3.2 `irq_numbers() -> std::span<const std::uint32_t>`.

## 4. Compile tests + async + example + HW

- [ ] 4.1 Extend `tests/compile_tests/test_can_api.cpp`.
- [ ] 4.2 `async::can::wait_for(InterruptKind)` and
      `async::can::receive_frame(FifoId)` runtime siblings.
- [ ] 4.3 `examples/can_probe_complete/`: targets `same70_xplained`
      MCAN0 with classic 500k + FD 2M, two filter banks, async
      receive task, bus-off recovery handler.
- [ ] 4.4 SAME70 hardware spot-check: 60 s of loopback at 500k +
      2M with 0 errors.
- [ ] 4.5 Update `docs/SUPPORT_MATRIX.md` `can` row.

## 5. Documentation + follow-ups

- [ ] 5.1 `docs/CAN.md` — comprehensive guide.
- [ ] 5.2 Cross-link from `docs/ASYNC.md`.
- [ ] 5.3 File `add-can-filter-bank-typed-enum` (alloy-codegen).
- [ ] 5.4 File `add-can-bittime-helper` for baud/sample-point math.
