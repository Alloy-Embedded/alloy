# Tasks: Close UART HAL Gaps

Host-testable: phases 1–3. Phase 4 requires hardware.

## 1. IR / codegen additions

- [ ] 1.1 Add UART IR fields for STM32G0 + STM32F4 families:
      `cts_enable_field`, `rts_enable_field`, `hdsel_field`,
      `dem_field`, `dep_field`, `linen_field`, `lbdl_field`, `sbkrq_field`,
      `orecf_field`, `fecf_field`, `necf_field`, `pecf_field`.
- [ ] 1.2 Add same fields for Microchip SAM (UART_MR, UART_IDR, US_CR) IR entries.
- [ ] 1.3 Extend `alloy-cpp-emit` UART template to include new fields in
      `UartSemanticTraits`.
- [ ] 1.4 Regen STM32G0 + SAME70; verify new fields appear in generated traits.
- [ ] 1.5 Update `cmake/hal-contracts/uart.json`:
      move `hardware_flow_ctl`, `half_duplex_field`, `rs485_de_field` to `optional`;
      add `lin_fields` group as optional.

## 2. HAL method additions

- [ ] 2.1 Add `enable_hardware_flow_control()` to `uart_handle.hpp`.
      Guard with `if constexpr (kCtseField.valid && kRtseField.valid)`.
- [ ] 2.2 Add `enable_half_duplex()`.
      Guard with `if constexpr (kHdselField.valid)`.
- [ ] 2.3 Add `enable_rs485_de(bool active_high)`.
      Guard with `if constexpr (kDemField.valid)`.
- [ ] 2.4 Add `enable_lin(bool long_break)` + `send_lin_break()`.
      Guard with `if constexpr (kLinenField.valid)`.
- [ ] 2.5 Add `read_and_clear_errors() -> UartErrors`.
      Define `UartErrors` struct in `src/hal/uart/uart_errors.hpp`.
      Implement ICR write path for STM32 (SR+DR read for older vendors).

## 3. Compile tests

- [ ] 3.1 Extend `tests/compile_tests/test_uart_api.cpp`:
      call `enable_hardware_flow_control()`, `enable_half_duplex()`,
      `enable_rs485_de(true)`, `enable_lin(false)`, `read_and_clear_errors()`.
- [ ] 3.2 Verify `NotSupported` path compiles for mock device with no flow-ctl fields.

## 4. Hardware validation

- [ ] 4.1 nucleo_g071rb: RS-485 loopback — enable DE, write 8 bytes, read back;
      verify data matches.
- [ ] 4.2 nucleo_g071rb: half-duplex UART loopback with single-wire jumper;
      verify TX→RX round-trip.
- [ ] 4.3 nucleo_g071rb: `read_and_clear_errors()` — inject overrun by filling
      RX FIFO; verify `overrun=true` returned; subsequent reads OK.

## 5. Documentation

- [ ] 5.1 Update `docs/UART_HAL.md`: document new methods, RS-485 wiring example,
      LIN break generation, error handling pattern.
- [ ] 5.2 Add `examples/modbus_rtu/` for nucleo_g071rb demonstrating RS-485 DE.
