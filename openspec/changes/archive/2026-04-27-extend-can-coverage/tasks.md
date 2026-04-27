# Tasks: Extend CAN Coverage

## 1. FD timing + test modes + frame primitives

- [x] 1.1 `set_data_timing(data_timing_config)` — gated on
      `kHasFlexibleDataRate && kDataPrescalerField.valid`. Uses
      kDataPrescalerField, kDataTimeSeg1Field, kDataTimeSeg2Field,
      kDataSyncJumpWidthField (all valid on SAME70 MCAN, all invalid on G071).
- [x] 1.2 `enum class TestMode { Normal, BusMonitor,
      LoopbackInternal, LoopbackExternal, Restricted }`.
      `set_test_mode(TestMode)` — gated on `kControlRegister.valid`;
      uses raw CCCR register RMW (individual MON/ASM/TEST bit fields
      not published in DB). TEST.LBCK set from kTestRegister for loopback.
- [x] 1.3 `struct CanFrame` (id, extended_id, remote_frame,
      fd_format, bit_rate_switch, dlc, data[64]).
      `transmit(CanFrame)`, `receive(CanFrame& out, FifoId fifo)` —
      NotSupported stubs (message RAM base address wiring deferred).

## 2. Filter banks + error counters + bus recovery

- [x] 2.1 `enum class FilterTarget { Rxfifo0, Rxfifo1,
      RejectMatching, AcceptMatching }`.
      `set_filter_standard`, `set_filter_extended` — NotSupported stubs
      (message RAM filter list configuration deferred).
- [x] 2.2 `tx_error_count()`, `rx_error_count()` — gated on
      `kErrorCounterRegister.valid`, raw ECR bits [7:0] and [15:8].
      `bus_off()`, `error_passive()`, `error_warning()` — gated on
      `kProtocolStatusRegister.valid`, raw PSR bits [6], [4], [5].
- [x] 2.3 `request_bus_off_recovery()` — delegates to `leave_init_mode()`.

## 3. Interrupts + IRQ vector

- [x] 3.1 `enum class InterruptKind { Tx, RxFifo0, RxFifo1, Error,
      BusOff, ErrorWarning, ErrorPassive, Wakeup }`.
      `enable_interrupt` / `disable_interrupt` — Tx→IE.TCE,
      RxFifo0→IE.RF0NE; others NotSupported (no IE field refs in DB).
- [x] 3.2 `irq_numbers() -> std::span<const std::uint32_t>`.

## 4. Compile tests + async + example + HW

- [x] 4.1 Extended `tests/compile_tests/test_can_api.cpp`.
      SAME70 (MCAN0) exercises all new methods. G071/F401 have no CAN.
- [x] 4.2 `async::can::wait_for<Kind>(handle)` — new files
      `src/runtime/can_event.hpp` + `src/runtime/async_can.hpp`.
      Added to `src/async.hpp`. Compile test: RxFifo0 + Tx.
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

## 10. Device-database follow-ups (deferred)

- [ ] 10.1 Publish CCCR.MON, CCCR.TEST, CCCR.ASM individual field refs.
- [ ] 10.2 Publish TEST.LBCK field ref.
- [ ] 10.3 Publish IE bits for BusOff, Error, ErrorWarning, ErrorPassive.
- [ ] 10.4 Message-RAM base address + TX/RX buffer layout for backend wiring.
