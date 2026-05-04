# Tasks: Refactor Lite HAL Surface

Host-testable: phases 1–2 (compile tests). Phase 2 FDCAN + phase 3 require hardware.

---

## Phase 1 — Tier taxonomy + device-data bridge

### 1.1 — Documentation

- [x] 1.1.1 Create `src/hal/README.md` with tier taxonomy (Tier 0/1/2), extension guide,
      and a table of every driver mapped to its tier.

### 1.2 — Tier 2: `irq_number()` / `irq_count()` on all PeripheralSpec-gated drivers

Add the following two methods to each driver.  Use `if constexpr (requires { P::kIrqLines[0]; })`
so the call compiles against both v2.1 flat-struct and any earlier artifact that lacks the field.

- [x] 1.2.1 `src/hal/uart/lite.hpp`  — add `irq_number(idx=0)`, `irq_count()`
- [x] 1.2.2 `src/hal/spi/lite.hpp`   — add `irq_number(idx=0)`, `irq_count()`
- [x] 1.2.3 `src/hal/i2c/lite.hpp`   — add `irq_number(idx=0)`, `irq_count()`
- [x] 1.2.4 `src/hal/gpio/lite.hpp`  — add `irq_number(idx=0)` (EXTI line = pin, device-specific)
- [x] 1.2.5 `src/hal/timer/lite.hpp` — add `irq_number(idx=0)`, `irq_count()`
- [x] 1.2.6 `src/hal/adc/lite.hpp`   — add `irq_number(idx=0)`, `irq_count()`
- [x] 1.2.7 `src/hal/dac/lite.hpp`   — add `irq_number(idx=0)`, `irq_count()`
- [x] 1.2.8 `src/hal/rtc/lite.hpp`   — add `irq_number(idx=0)`, `irq_count()`
- [x] 1.2.9 `src/hal/watchdog/lite.hpp` — add `irq_number(idx=0)`, `irq_count()` (both iwdg_port + wwdg_port)
- [x] 1.2.10 `src/hal/lptim/lite.hpp` — add `irq_number(idx=0)`, `irq_count()`

### 1.3 — Tier 1: vendor-hint documentation + optional compile-time guard

Add the `ALLOY_ASSERT_VENDOR_STM32` guard block and vendor-scope comment to each
address-template driver class body (see design.md §Tier 1 Vendor-Hint Pattern).
Note: `dmamux` is part of `dma/lite.hpp` — no separate file exists.

- [x] 1.3.1 `src/hal/dma/lite.hpp`    — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block
- [x] 1.3.2 `src/hal/dma/lite.hpp`    — dmamux<> class (inside same file) — vendor guard
- [x] 1.3.3 `src/hal/exti/lite.hpp`   — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block
- [x] 1.3.4 `src/hal/flash/lite.hpp`  — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block
- [x] 1.3.5 `src/hal/crc/lite.hpp`    — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block
- [x] 1.3.6 `src/hal/rng/lite.hpp`    — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block
- [x] 1.3.7 `src/hal/pwr/lite.hpp`    — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block
- [x] 1.3.8 `src/hal/syscfg/lite.hpp` — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block
- [x] 1.3.9 `src/hal/opamp/lite.hpp`  — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block
- [x] 1.3.10 `src/hal/comp/lite.hpp`  — vendor-scope comment + `ALLOY_ASSERT_VENDOR_STM32` block

### 1.4 — Compile tests

- [x] 1.4.1 Add `tests/compile_tests/test_lite_irq_bridge.cpp`:
      mock peripheral structs for all 10 Tier 2 drivers; static_assert on return
      type (uint32_t / size_t), value matches kIrqLines/kIrqCount, noexcept.
- [x] 1.4.2 Add `tests/compile_tests/test_lite_tier1_guard.cpp`:
      `#define ALLOY_ASSERT_VENDOR_STM32 + ALLOY_DEVICE_VENDOR_STM32=1`, includes
      all 9 Tier 1 headers; static_assert sizeof == 1 for all instantiations.

---

## Phase 2 — FDCAN lite driver + SAME70 UART dispatch

### 2.1 — FDCAN lite driver (STM32 G4/H7 polling)

- [x] 2.1.1 Create `src/hal/fdcan/lite.hpp` — `StFdcan` concept, `controller<P, MsgRamBase>`,
      `configure()`, `start()`, `write_tx()`, `read_rx()`, `tx_pending()`, `rx_available()`,
      `irq_number()`, `irq_count()`, `clock_on()`, `clock_off()` (Tier 2, targets STM32G4).
- [x] 2.1.2 Create `src/hal/fdcan.hpp` umbrella header.
- [x] 2.1.3 Add compile test `tests/compile_tests/test_fdcan_lite.cpp`.

### 2.2 — SAME70 UART dispatch in uart/lite.hpp

- [x] 2.2.1 Add `detail::is_sam_uart(const char* tmpl) → bool` consteval helper.
- [x] 2.2.2 Add `SamUart<P>` concept (PeripheralSpec + kTemplate == "uart").
- [x] 2.2.3 Change `port<P>` constraint from `StUsart<P>` to `AnyUart<P>`
      where `AnyUart = StUsart<P> || SamUart<P>`.
- [x] 2.2.4 Add SAME70 register offset constants in `detail` namespace:
      `kSamCrOfs`, `kSamMrOfs`, `kSamBrgrOfs`, `kSamRhrOfs`, `kSamThrOfs`, `kSamSrOfs`,
      `kSamIerOfs`, `kSamIdrOfs` + CR/MR/SR/IER bit constants.
- [x] 2.2.5 Add `if constexpr (SamUart<P>)` dispatch in `configure()`,
      `write_byte()`, `read_byte()`, `try_read_byte()`, `try_write_byte()`,
      `ready_to_send()`, `data_available()`, `flush()`, `enabled()`, `disable()`,
      `frame_error()`, `overrun()`, `parity_error()`, `noise_error()`, `has_errors()`,
      `clear_errors()`, `enable_rx_irq()`, `enable_tx_irq()`, `enable_tc_irq()`,
      `enable_error_irq()`, `disable_irqs()`.
- [x] 2.2.6 Add compile test `tests/compile_tests/test_sam_uart_lite.cpp` with a
      mock SAME70 UART PeripheralSpec (kTemplate="uart", kBaseAddress=0xDEAD0000).

### 2.3 — Codegen: `kDmaRxRequest` + `kDmaTxRequest` fields

- [x] 2.3.1 Extend `alloy-cpp-emit` (or equivalent codegen) to emit
      `kDmaRxRequest` and `kDmaTxRequest` for UART, SPI, I2C, ADC, DAC in
      `peripheral_traits.h`.  Vendor sources: DMA request line tables in RM.
- [ ] 2.3.2 Regen STM32G0 (`nucleo_g071rb`) and verify new fields appear.
- [ ] 2.3.3 Regen SAME70 (`same70_xplained`) and verify new fields appear (XDMAC PIDs).

---

## Phase 3 — `kRccEnable` compile-time clock gate

### 3.1 — RCC gate table (codegen)

- [x] 3.1.1 Create `src/device/rcc_gate_table.hpp` with `RccGate` struct and
      `find_rcc_gate()` declaration + conditional include of the generated
      per-device table (via `ALLOY_DEVICE_RCC_TABLE_AVAILABLE` + `ALLOY_DEVICE_RCC_TABLE_INCLUDE`).
- [x] 3.1.2 Extend codegen to emit the lookup table as part of the device artifact
      build step (CMake target `alloy_device_rcc_table`).

### 3.2 — `clock_on()` / `clock_off()` in Tier 2 drivers

Add to each Tier 2 `port<P>` / `controller<P>` class, guarded by
`requires (requires { P::kRccEnable; })`:

- [x] 3.2.1 `uart/lite.hpp`     — `clock_on()`, `clock_off()`
- [x] 3.2.2 `spi/lite.hpp`      — `clock_on()`, `clock_off()`
- [x] 3.2.3 `i2c/lite.hpp`      — `clock_on()`, `clock_off()`
- [x] 3.2.4 `timer/lite.hpp`    — `clock_on()`, `clock_off()`
- [x] 3.2.5 `adc/lite.hpp`      — `clock_on()`, `clock_off()`
- [x] 3.2.6 `dac/lite.hpp`      — `clock_on()`, `clock_off()`
- [x] 3.2.7 `rtc/lite.hpp`      — `clock_on()`, `clock_off()`
- [x] 3.2.8 `watchdog/lite.hpp` — `clock_on()`, `clock_off()` (both iwdg_port + wwdg_port)
- [x] 3.2.9 `lptim/lite.hpp`    — `clock_on()`, `clock_off()`
      Also added to `fdcan/lite.hpp` (controller<P,MsgRamBase>).

### 3.3 — Compile tests

- [x] 3.3.1 Add `tests/compile_tests/test_lite_clock_gate.cpp`:
      mock specs for all 11 Tier 2 driver instantiations; static_assert return
      type void; static_assert methods absent when kRccEnable missing;
      compiles without `dev::peripheral_on<>` or any device artifact.

---

## Hardware Validation

- [ ] HW.1 `nucleo_g071rb` — `irq_number()` integration: enable USART1 IRQ via
      `Nvic::enable_irq(Uart1::irq_number())`, send 8 bytes, confirm ISR fires.
- [ ] HW.2 `nucleo_g071rb` — `clock_on()` path: call `Uart1::clock_on()` instead of
      `dev::peripheral_on<dev::usart1>()`, confirm USART1 operates normally.
- [ ] HW.3 `same70_xplained` — SAME70 UART dispatch (Phase 2): configure UART0 at
      115200 baud via `port<dev::uart0>`, loopback 8 bytes.
- [ ] HW.4 `nucleo_g4` (or `nucleo_h743`) — FDCAN lite (Phase 2): polling TX→RX
      self-reception with 500 kbps nominal bit rate.
