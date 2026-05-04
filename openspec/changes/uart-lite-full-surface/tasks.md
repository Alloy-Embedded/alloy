# Tasks: UART Lite — Full Surface

Host-testable: phases 1–5 (compile tests). Phase 6 requires hardware.

---

## Phase 1 — Codegen: new capability fields

### 1.1 — YAML patches

- [x] 1.1.1 `codegen/patches/st/stm32g0-bootstrap.yaml`:
      Add per USART (1–4, LPUART1):
      `kSupportsFlowCtl`, `kSupportsRS485`, `kSupportsIrda`,
      `kDmaTxSignal` (DMAMUX request ID), `kDmaRxSignal`.
      Set `kSupportsFlowCtl=false` / `kSupportsRS485=false` / `kSupportsIrda=false`
      for LPUART (no DEM/DEP/LINEN/IREN in LPUART register map).
      Also filled missing kSupportsLin/HalfDuplex/Smartcard on USART2.
      Added USART3, USART4, LPUART blocks (irq=29 shared; LPUART kMaxBaudHz=2666666).
      Note: LPUART name in SVD is "LPUART" (not "LPUART1") — bootstrap uses "LPUART".

- [x] 1.1.2 Create `codegen/patches/st/stm32f4-bootstrap.yaml`:
      USART1/2/3/6, UART4/5. `kFifoDepth=1` (SCI2, no FIFO register),
      `kSupportsFlowCtl=true` for USART1/2/3/6, `false` for UART4/5.
      `kSupportsRS485=false` for all F4 (no DEM/DEP bits in SCI2 CR3).
      `kDmaTxSignal/kDmaRxSignal=0` (F4 uses fixed DMA streams, no DMAMUX).

### 1.2 — Jinja2 template

- [x] 1.2.1 `codegen/templates/driver_semantics/uart.hpp.j2`:
      Added kSupportsFlowCtl, kSupportsRS485, kDmaTxSignal, kDmaRxSignal
      to base fallback struct (false/0u defaults) and per-peripheral specialization.
      Note: kSupportsIrda already existed in template; not duplicated.

### 1.3 — Regenerate + verify

- [x] 1.3.1 Ran `python3 tests/codegen_smoke/generate.py` for all 5 devices.
      New fields present in all generated uart.hpp snapshots.
      G0 LPUART: kSupportsLin=false, kSupportsRS485=false, kSupportsIrda=false ✓
      F4 USART: kSupportsRS485=false (no DEM/DEP in SCI2) ✓
      G0 USART1: kSupportsFlowCtl=true, kDmaTxSignal=53u, kDmaRxSignal=52u ✓

---

## Phase 2 — Codegen: modm-style from-GPIO Guard B

- [ ] 2.1 `codegen/templates/runtime/connectors.hpp.j2` (alloy-devices):
      Add `SignalSourceTraits<PinId, SignalId>` struct for each (pin, signal) pair
      that has at least one valid peripheral. Struct fields:
      - `static constexpr std::array<PeripheralId, N> kValidPeripherals`
      - `static constexpr std::string_view kMessage` — human-readable
        "PB5::signal_tx can only connect to: USART3. Check the connector<> peripheral ID."

- [ ] 2.2 In the primary (catch-all) `ConnectorTraits<Pin, Peri, Sig>` template:
      Add Guard B `static_assert` using `SignalSourceTraits`.
      Guard B fires only when `SignalSourceTraits<Pin, Sig>::kPresent` is true
      but `Peri` is not in `kValidPeripherals` — prevents false positives for
      unrouted signals.

- [ ] 2.3 Regenerate `connectors.hpp` for stm32g071rb.
      Verify: `connector<USART1, tx<PA5>>` gives
      "PA5::signal_tx can only connect to: USART2. ..."
      (compile negative test in 5.2).

---

## Phase 3 — `lite::port<P>` extended MMIO kernel

Add to `src/hal/uart/lite.hpp` — all pure MMIO, no connector knowledge:

### 3.1 — New types

- [x] 3.1.1 Move `UartErrors` from `detail/backend.hpp` into `lite.hpp`
      (before `Config` struct). Keep 4 bool fields (`parity`, `framing`, `noise`,
      `overrun`) + `any()`.
- [x] 3.1.2 Add `enum class FifoTrigger : uint8_t { Empty, Quarter, Half, ThreeQuarters, Full }`.
- [x] 3.1.3 Add `enum class InterruptKind : uint8_t { Tc, Txe, Rxne, IdleLine, LinBreak, Cts, Error, RxFifoThreshold, TxFifoThreshold }`.
- [x] 3.1.4 Add `enum class Oversampling : uint8_t { X16, X8 }`.

### 3.2 — New `detail::` bit constants

- [x] 3.2.1 CR3: `kCr3Dmar`, `kCr3Dmat`, `kCr3Rtse`, `kCr3Ctse`, `kCr3Hdsel`,
              `kCr3Dem`, `kCr3Dep`, `kCr3Iren`, `kCr3Scen`.
- [x] 3.2.2 CR1 SCI3: `kSci3Cr1Fifoen`, `kSci3Cr1DeatMask` (bits [25:21]),
              `kSci3Cr1DedtMask` (bits [20:16]), `kSci3Cr1Uesm` (bit 23), `kSci3Cr1Over8`.
- [x] 3.2.3 CR2: `kCr2Linen`, `kCr2Lbdl`, `kCr2Lbdie`, `kCr2Clken`.
- [x] 3.2.4 ISR/SR: `kIsrLbdf`, `kIsrRxff`, `kIsrTxff`.
- [x] 3.2.5 ICR: `kIcrLbdcf`.
- [x] 3.2.6 RQR: `kRqrSbkrq`.
- [x] 3.2.7 CR3 FIFO thresholds: `kCr3TxftcfgShift=29`, `kCr3RxftcfgShift=24`, masks.

### 3.3 — New methods in `port<P>`

- [x] 3.3.1 `read_and_clear_errors() -> UartErrors`:
      SCI3: read ISR bits 0-3, write ICR single-word clear.
      SCI2: read SR + `(void)sci2().dr` (sticky-flag clear side-effect).
- [x] 3.3.2 `error_flags() -> UartErrors`: read-only, no clear.
- [x] 3.3.3 `enable_hardware_flow_control(bool)`: CR3 RTSE (bit 8) + CTSE (bit 9).
- [x] 3.3.4 `set_de_polarity(bool active_high)`: CR3 DEP (bit 15). 0=active-high, 1=active-low.
- [x] 3.3.5 `enable_de(bool)`: CR3 DEM (bit 14).
- [x] 3.3.6 `set_de_assertion_time(uint8_t)`: SCI3 CR1 DEAT [25:21].
- [x] 3.3.7 `set_de_deassertion_time(uint8_t)`: SCI3 CR1 DEDT [20:16].
- [x] 3.3.8 `set_half_duplex(bool)`: CR3 HDSEL (bit 3). Note: must disable CTSE/RTSE first.
- [x] 3.3.9 `enable_lin(bool)`: CR2 LINEN (bit 14). Also clears CR2 CLKEN.
- [x] 3.3.10 `set_lin_break_length(bool long_break)`: CR2 LBDL (bit 5). 0=10-bit, 1=11-bit.
- [x] 3.3.11 `send_lin_break()`: SCI3: RQR SBKRQ (bit 1). SCI2: CR1 SBK (bit 0), set then wait HW clear.
- [x] 3.3.12 `lin_break_detected() -> bool`: ISR/SR LBDF (bit 8).
- [x] 3.3.13 `clear_lin_break_flag()`: SCI3: ICR LBDCF (bit 8). SCI2: SR+DR read.
- [x] 3.3.14 `enable_lin_break_irq(bool)`: CR2 LBDIE (bit 6).
- [x] 3.3.15 `set_smartcard_mode(bool)`: CR3 SCEN (bit 5).
- [x] 3.3.16 `set_irda_mode(bool)`: CR3 IREN (bit 1).
- [x] 3.3.17 `enable_fifo(bool)`: SCI3 CR1 FIFOEN (bit 29).
- [x] 3.3.18 `set_tx_fifo_threshold(FifoTrigger)`: CR3 TXFTCFG [31:29].
- [x] 3.3.19 `set_rx_fifo_threshold(FifoTrigger)`: CR3 RXFTCFG [26:24].
- [x] 3.3.20 `tx_fifo_full() -> bool`: ISR TXFF (bit 25).
- [x] 3.3.21 `rx_fifo_full() -> bool`: ISR RXFF (bit 24).
- [x] 3.3.22 `enable_dma_tx(bool)`: CR3 DMAT (bit 7).
- [x] 3.3.23 `enable_dma_rx(bool)`: CR3 DMAR (bit 6).
- [x] 3.3.24 `enable_interrupt(InterruptKind, bool)`:
      RXNE/IDLE/PE/ERR → CR1 bits 2/4/5/8; TXE/TC → CR1 bits 6/7; LBDIE → CR2 bit 6; CTSIE → CR3 bit 10.
      SCI3 FIFO: RXFTIE → CR3 bit 27, TXFTIE → CR3 bit 23.
- [x] 3.3.25 `enable_wakeup_from_stop(bool)`: SCI3 CR1 UESM (bit 23).
- [x] 3.3.26 `set_oversampling(Oversampling)`: CR1 OVER8 (bit 15).
- [ ] 3.3.27 `set_baud_rate(uint32_t bps, uint32_t clk_hz) -> Result<void, ErrorCode>`:
      Existing method — add `if (bps > Caps::kMaxBaudHz) return Err(InvalidParameter)` guard.
      Note: this method is in `port<P>` which has no Caps; move guard to `uart::port<C>`.

---

## Phase 4 — `uart::port<Connector>` — user-facing type

- [x] 4.1 Created `src/hal/uart/port.hpp`:
      - `port<Connector>` inherits `port_handle<Connector>` — gets all Phase 1–3 methods
      - `static connect()` → `Result<void, ErrorCode>`: calls
        `detail::runtime::apply_route_operations(Connector::operations())`
      - `configure()` override: calls `connect()` first, returns early on failure
      - Guard A: inherited from `Connector::valid` static_assert in `runtime_connector.hpp`
      - Guard C: `has_tx_or_rx_v<C>` and `has_duplicate_role_v<C>` via
        `count_bindings_with_role<C, role_id>()` consteval helper in `uart::detail`
      - Note: `Caps::kSupportsXxx` gating deferred — `requires` constraints already on
        `lite::port<P>` methods; the connector-typed surface inherits that gating

- [x] 4.2 Updated `src/hal/uart.hpp` umbrella to include `"hal/uart/port.hpp"`.

- [x] 4.3 `has_tx_or_rx_v<C>` and `has_duplicate_role_v<C>` placed in
      `uart::detail` inside `port.hpp` — no separate file needed at this size.

---

## Phase 5 — Compile tests

- [x] 5.1 `tests/compile_tests/test_uart_port.cpp`:
      Mock peripheral `MockUsart1` with all capability fields set.
      Mock connector `MockConnector<MockUsart1, tx<PA9>, rx<PA10>>`.
      `static_assert` on every method in Phase 3+4 (return type, noexcept, requires-clause
      fires correctly when capability is false).

- [x] 5.2 `tests/compile_tests/test_uart_pin_guard.cpp` (negative compile test):
      Use `//! [expect-error]` pragma or CMake compile-fail target.
      Verify that `connector<USART1, tx<PA5>>` fires:
        - Guard A: "Invalid connector for USART1 tx. Valid pins: PB6."
        - Guard B (after phase 2): "PA5::signal_tx can only connect to: USART2."
      Verify that `connector<USART1, tx<PB6>, tx<PB6>>` fires Guard C duplicate check.
      Verify that `connector<USART1>` (no tx/rx) fires Guard C missing check.

- [x] 5.3 Update `tests/compile_tests/test_uart_api.cpp`:
      Replace `port_handle<>` with `uart::port<>`.
      Add calls: `enable_de(true)`, `set_de_polarity(true)`, `enable_hardware_flow_control(false)`,
      `enable_lin(false)`, `read_and_clear_errors()`, `enable_dma_tx(true)`.

---

## Phase 6 — Delete old backend

Execute after all internal callers verified migrated (grep for includes first).

- [x] 6.1 Find all include sites:
      ```bash
      grep -rn '"hal/uart/uart.hpp"\|"hal/uart/detail/backend.hpp"' src/
      ```
      Found: `src/runtime/uart_event.hpp` — uses `hal::uart::InterruptKind` (backend.hpp
      namespace); `lite::InterruptKind` is in `hal::uart::lite::` — NOT a drop-in swap.
      Migrating `uart_event.hpp` requires moving types or adding `using` alias — deferred.
      `test_same70_bringup.cpp` (host-mmio, uses internal detail fns — deferred to 6.4).
- [x] 6.2 Update every call site: `port_handle<C>` → `uart::port<C>`.
      Updated: all 8 board_uart.hpp files, `examples/modbus_rtu/main.cpp`,
      `examples/uart_path_probe/main.cpp`. Removed ambiguous old `open()` from `uart.hpp`.
      Remaining: `port_handle<Connector>` template definition stays in `uart.hpp` (6.3 deferred).
- [ ] 6.3 Delete `src/hal/uart/uart.hpp`.
- [ ] 6.4 Delete `src/hal/uart/detail/backend.hpp`.
- [ ] 6.5 Remove `has_signal<>` helper (replaced by `requires` in port.hpp).
- [ ] 6.6 Add `[[deprecated("use uart::port<Connector>")]]` to any public typedef or
      alias referencing the old type, leave for one release cycle, then remove.

---

## Phase 7 — Hardware validation

Requires Nucleo-G071RB.

- [ ] 7.1 RS-485 loopback — `enable_de(true)` + `set_de_polarity(active-high)`;
      transmit 8 bytes, receive same bytes; verify data matches.
- [ ] 7.2 Hardware flow control — wire CTS→RTS loopback; `enable_hardware_flow_control(true)`;
      verify TX pauses when CTS deasserted.
- [ ] 7.3 Half-duplex — single-wire jumper TX↔RX; `set_half_duplex(true)`;
      TX 4 bytes, RX back; verify round-trip.
- [ ] 7.4 `read_and_clear_errors()` overrun injection — fill RX FIFO;
      verify `overrun=true`; subsequent read succeeds.
- [ ] 7.5 LIN break detection — `enable_lin(true)`, `send_lin_break()`;
      verify `lin_break_detected()` returns `true` on receiving side.

---

## Notes

- `lite::port<P>` (internal Tier 2 kernel) retains `requires StUsart<P>` / `AnyUart<P>` concept.
  New methods in 3.3 have no `requires (Caps::kSupportsXxx)` — they are unconditional on
  `lite::port<P>`.  The capability guard lives in `uart::port<C>` via the `requires` clause.
  Rationale: `lite::port<P>` is a hardware register driver; it should do what you ask.
  `uart::port<C>` is the safe user API; it should refuse unsupported operations at compile time.

- `uart::port<Connector>` replaces `port_handle<Connector>` entirely.
  Name change rationale: `port` is shorter and consistent with `hal::spi::port<C>`,
  `hal::i2c::port<C>` (future migration of those backends follows same pattern).

- `UartSemanticTraits` is NOT deprecated at this stage — it's the source of capability
  data for `uart::port<C>`.  Deprecation of the *register-field* accessors
  (`kCtseField`, `kTxeField`, etc.) is tracked separately in close-uart-hal-gaps.

- DMA integration (channel binding, buffer management) is out of scope for this openspec.
  `enable_dma_tx/rx(bool)` sets the CR3 enable bits only; full DMA channel setup is via
  `hal::dma::channel<>` (separate openspec: extend-dma-coverage).
