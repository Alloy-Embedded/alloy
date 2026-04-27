# Tasks: Extend I2C Coverage

Phases ordered. Phases 1-4 are host-testable. Phase 5 needs the
3-board hardware matrix.

## 1. Speed / addressing / clock stretching

- [x] 1.1 `set_clock_speed(std::uint32_t hz)` — NotSupported stub
      (TIMINGR / CCR register refs not published in current device DB;
      use configure(Config{speed,...}) as workaround).
- [x] 1.2 `enum class SpeedMode { Standard100kHz, Fast400kHz,
      FastPlus1MHz }`. `set_speed_mode(SpeedMode)` — NotSupported stub.
- [x] 1.3 `enum class DutyCycle { Duty2, Duty169 }`.
      `set_duty_cycle(DutyCycle)` — implemented via `kDutyField` (F401
      CCR.DUTY); NotSupported on G071 (v2) and SAME70.
- [x] 1.4 `enum class AddressingMode { Bits7, Bits10 }`.
      `set_addressing_mode(AddressingMode)` — NotSupported stub
      (no mode-configuration field in DB).
- [x] 1.5 `set_own_address(std::uint16_t addr, AddressingMode mode)`,
      `set_dual_address(std::uint16_t addr2)` — NotSupported stubs
      (no OAR1/OAR2 field in DB).
- [x] 1.6 `set_clock_stretching(bool enabled)` — NotSupported stub
      (no NOSTRETCH field in DB).

## 2. Status / interrupts / IRQ vector

- [x] 2.1 Status accessors: `nack_received()`, `arbitration_lost()`,
      `bus_error()`. `clear_*` mirrors per descriptor.
      Implemented: STM32 v2 uses NACKF/ARLOF/BERRF (ISR), STM32 v1 uses
      AF/ARLO/BERR (SR1), SAME70 uses NACK/ARBLST.
- [x] 2.2 `enum class InterruptKind { Tx, Rx, Stop, Tc, AddrMatch,
      Nack, BusError, ArbitrationLoss, Overrun, PecError, Timeout,
      SmbAlert }`. `enable_interrupt` / `disable_interrupt` —
      NotSupported stubs (no individual IE field refs in DB).
- [x] 2.3 `irq_numbers() -> std::span<const std::uint32_t>`.
- [x] 2.4 `set_kernel_clock_source(KernelClockSource)` — gated on
      `kKernelClockSelectorField.valid` (invalid on all current devices).

## 3. SMBus / PEC

- [x] 3.1 SMBus (`kSupportsSmbus`): `enable_smbus(bool)`,
      `set_smbus_role(SmbusRole)` — NotSupported stubs (SMBHEN/SMBDEN
      bits not published in DB).
- [x] 3.2 PEC (`kSupportsPec`): `enable_pec(bool)`,
      `last_pec() -> std::uint8_t`, `pec_error() -> bool`,
      `clear_pec_error() -> Result<void, ErrorCode>` — NotSupported stubs.

## 4. Compile tests

- [x] 4.1 Extend `tests/compile_tests/test_i2c_api.cpp`:
      G071 has no I2C signals in SignalId — no board-specific section.
      F401 + SAME70 exercise all new methods.
- [x] 4.2 Add a `nucleo_f401re`-targeted compile test for legacy
      CCR-style timing. PB8/SCL + PB9/SDA I2C1. Also fixed:
      - `schema_alloy_i2c_st_i2c1_v1_5_cube` mapping in `to_i2c_schema`
      - `read_microchip_twihs` 2-arg call in backend → 3-arg (empty slice).
- [x] 4.3 Add a `same70_xplained`-targeted compile test for
      CWGR-style + SMBus.

## 5. Async integration

- [ ] 5.1 `async::i2c::wait_for(InterruptKind)` — sibling of
      existing `async::i2c::write` / `read` / `write_read`.
- [ ] 5.2 Compile test extends `test_async_peripherals.cpp`.

## 6. Example

- [ ] 6.1 `examples/i2c_probe_complete/`: targets `nucleo_g071rb`
      I2C1, configures Fast Plus 1 MHz with Duty2, 10-bit
      addressing, clock-stretching enabled, NACK + bus-error
      interrupts, async write_read against AT24MAC402.
- [ ] 6.2 Mirror on `nucleo_f401re` (drop FastPlus, use Fast 400k).
- [ ] 6.3 Mirror on `same70_xplained` (TWIHS path).

## 7. Hardware spot-check (3-board matrix)

- [ ] 7.1 SAME70 Xplained: AT24MAC402 read/write end-to-end at
      400 kHz, 0 errors over 60 s.
- [ ] 7.2 STM32G0 Nucleo: same.
- [ ] 7.3 STM32F4 Nucleo: same.
- [ ] 7.4 Update `docs/SUPPORT_MATRIX.md` `i2c` row.

## 8. Documentation

- [ ] 8.1 `docs/I2C.md` — model, speed-mode recipe, 10-bit
      addressing, clock-stretching, SMBus, error handling, async
      wiring, modm migration table, per-vendor capability matrix.
- [ ] 8.2 Cross-link from `docs/ASYNC.md` and `docs/COOKBOOK.md`.

## 9. Out-of-scope follow-ups (filed)

- [ ] 9.1 alloy-codegen `add-i2c-clock-source-typed-enum`.
- [ ] 9.2 alloy `add-i2c-slave-mode` — full slave-side HAL.
- [ ] 9.3 alloy `add-i2c-circular-rx-dma` example.

## 10. Device-database follow-ups (deferred)

- [ ] 10.7 Publish TIMINGR register ref for STM32 I2C v2 backends.
- [ ] 10.8 Publish OAR1/OAR2 field refs for own/dual address.
- [ ] 10.9 Publish NOSTRETCH, individual IE bits, SMBus/PEC fields.
- [ ] 10.10 Add I2C signals to G071 SignalId enum (signal_scl, signal_sda).
