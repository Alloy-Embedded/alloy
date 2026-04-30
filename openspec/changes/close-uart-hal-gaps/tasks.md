# Tasks: Close UART HAL Gaps

Host-testable: phases 1–3. Phase 4 requires hardware.

## 1. IR / codegen additions

- [ ] 1.1 Add UART IR fields for STM32G0 + STM32F4 families:
      `cts_enable_field`, `rts_enable_field`, `hdsel_field`,
      `dem_field`, `dep_field`, `linen_field`, `lbdl_field`, `sbkrq_field`,
      `orecf_field`, `fecf_field`, `necf_field`, `pecf_field`.
      (fields discovered via `find_runtime_field_ref_by_register_and_offset`
       in `st_uart_register_bank` — no IR patch needed for existing registers;
       CTSE/RTSE/DEP added to register_bank and base fallback)
- [ ] 1.2 Add same fields for Microchip SAM (UART_MR, UART_IDR, US_CR) IR entries.
- [ ] 1.3 Extend `alloy-cpp-emit` UART template to include new fields in
      `UartSemanticTraits`.
- [ ] 1.4 Regen STM32G0 + SAME70; verify new fields appear in generated traits.
- [ ] 1.5 Update `cmake/hal-contracts/uart.json`:
      move `hardware_flow_ctl`, `half_duplex_field`, `rs485_de_field` to `optional`;
      add `lin_fields` group as optional.

## 2. HAL method additions

- [x] 2.1 Add `enable_hardware_flow_control(bool)` to `uart.hpp`.
      Uses `cr3_rtse_field` (CR3 bit 8) + `cr3_ctse_field` (CR3 bit 9).
      Returns NotSupported when either field is invalid.
- [x] 2.2 `set_half_duplex(bool)` already existed via `cr3_hdsel_field`.
- [x] 2.3 Add `set_de_polarity(bool active_high)`.
      Uses `cr3_dep_field` (CR3 bit 15); 0=active-high, 1=active-low.
      `enable_de(bool)` already existed.
- [x] 2.4 `enable_lin(bool)` + `send_lin_break()` already existed.
- [x] 2.5 Add `read_and_clear_errors() -> UartErrors`.
      `UartErrors` struct defined in `detail/backend.hpp`.
      ST modern: read ISR(PE/FE/NE/ORE), write ICR clear bits atomically.
      ST legacy: read SR then DR (clears sticky flags).

## 3. Compile tests

- [x] 3.1 Extended `tests/compile_tests/test_uart_api.cpp`:
      `enable_hardware_flow_control(true)`, `set_de_polarity(true)`,
      `read_and_clear_errors()`, `UartErrors::any()`.
      F4 compile test clean; G0 pre-existing IR regen issues unrelated to UART.
- [ ] 3.2 Verify `NotSupported` path compiles for mock device with no flow-ctl fields.
      (implicit — base class returns kInvalidFieldRef, if constexpr guard returns NotSupported)

## 4. Hardware validation

- [ ] 4.1 nucleo_g071rb: RS-485 loopback — enable DE, write 8 bytes, read back;
      verify data matches.
- [ ] 4.2 nucleo_g071rb: half-duplex UART loopback with single-wire jumper;
      verify TX→RX round-trip.
- [ ] 4.3 nucleo_g071rb: `read_and_clear_errors()` — inject overrun by filling
      RX FIFO; verify `overrun=true` returned; subsequent reads OK.

## 5. Documentation

- [x] 5.1 Create `docs/UART_HAL.md`: hardware flow control, RS-485 DE + polarity,
      half-duplex, LIN, `read_and_clear_errors` + recovery pattern, vendor extension points.
- [ ] 5.2 Add `examples/modbus_rtu/` for nucleo_g071rb demonstrating RS-485 DE.
