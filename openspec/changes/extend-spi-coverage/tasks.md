# Tasks: Extend SPI Coverage

Phases ordered. Phases 1-4 are host-testable (compile + concept).
Phase 5 needs the existing 3-board hardware matrix
(SAME70 / STM32G0 / STM32F4).

## 1. Variable data size, clock speed, kernel-clock source

- [ ] 1.1 `set_data_size(std::uint8_t bits)` — gated on
      `kDsField.valid || kDffField.valid`. Validates the requested
      width against the published field's encoding range and returns
      `core::ErrorCode::InvalidArgument` for unrealisable widths
      (e.g. `Bits12` on an `kDffField`-only peripheral).
- [ ] 1.2 `set_clock_speed(std::uint32_t hz)` — resolves the BR
      divider from the peripheral kernel clock. Returns
      `InvalidArgument` if the realised rate falls outside ±5 % of
      the requested rate.
- [ ] 1.3 `realised_clock_speed() -> std::uint32_t` — returns the
      rate the current BR encoding produces; lets callers verify
      what they actually got after `set_clock_speed`.
- [ ] 1.4 `set_kernel_clock_source(KernelClockSource)` — gated on
      `kKernelClockSelectorField.valid`; runtime-validates against
      the published option list.
- [ ] 1.5 `kernel_clock_hz() -> std::uint32_t` — same shape as the
      UART variant.

## 2. Frame format, CRC, bidirectional, NSS management

- [ ] 2.1 `enum class FrameFormat { Motorola, TI }`.
      `set_frame_format(FrameFormat)` — gated independently on
      `kSupportsMotorolaFrame` / `kSupportsTiFrame`.
- [ ] 2.2 CRC (gated on `kSupportsCrc`):
      `enable_crc(bool)`,
      `set_crc_polynomial(std::uint16_t)`,
      `read_crc() -> std::uint16_t`,
      `crc_error() -> bool`,
      `clear_crc_error() -> Result<void, ErrorCode>`.
- [ ] 2.3 Bidirectional 3-wire (gated on `kSupportsBidirectional3Wire`):
      `set_bidirectional(bool)`,
      `enum class BiDir { Receive, Transmit }`,
      `set_bidirectional_direction(BiDir)`.
- [ ] 2.4 Hardware NSS management (gated on `kSupportsNssHwManagement`):
      `enum class NssManagement { Software, HardwareInput, HardwareOutput }`.
      `set_nss_management(NssManagement)`,
      `set_nss_pulse_per_transfer(bool)`.

## 3. SAM-style per-CS timing

- [ ] 3.1 `set_cs_decode_mode(bool)` — gated on `kPcsdecField.valid`
      (PCS bits decoded as 1-of-N vs 4-of-15 direct).
- [ ] 3.2 `set_cs_delay_between_consecutive(std::uint16_t cycles)` —
      wired through `kDlybctField`.
- [ ] 3.3 `set_cs_delay_clock_to_active(std::uint16_t cycles)` —
      wired through `kDlybsField`.
- [ ] 3.4 `set_cs_delay_active_to_clock(std::uint16_t cycles)` —
      wired through `kDlybcsField`.

## 4. Status flags, interrupts, IRQ vector lookup

- [ ] 4.1 Status flag accessors (gated per-field):
      `tx_register_empty()`, `rx_register_not_empty()`, `busy()`,
      `mode_fault()`, `frame_format_error()`. Each has its
      `clear_*` mirror where the descriptor publishes a clear-side
      field.
- [ ] 4.2 `enum class InterruptKind { Txe, Rxne, Error, ModeFault,
      CrcError, FrameError }`.
      `enable_interrupt(InterruptKind)` /
      `disable_interrupt(InterruptKind)` — each gated on the
      corresponding control-side IE field.
- [ ] 4.3 `irq_numbers() -> std::span<const std::uint32_t>` —
      `static constexpr` returning the descriptor's `kIrqNumbers`.

## 5. Compile tests

- [ ] 5.1 Extend `tests/compile_tests/test_spi_api.cpp` to
      instantiate every new method against `nucleo_g071rb` SPI1.
      Verify return types, `static_assert` fires on `Bits3` /
      `Bits17` data-size requests at compile time.
- [ ] 5.2 Add a `same70_xplained`-targeted compile test exercising
      the per-CS timing setters (DLYBCS / DLYBCT / DLYBS — SAM-only).
- [ ] 5.3 Add a `nucleo_f401re`-targeted compile test exercising
      CRC + frame-format + NSS hardware output (full STM32 SPI
      surface).

## 6. Async integration

- [ ] 6.1 `async::spi::wait_for(InterruptKind)` — sibling of the
      existing DMA wrappers; signals on the requested
      interrupt-kind token.
- [ ] 6.2 `tests/compile_tests/test_async_peripherals.cpp` extended
      with a `wait_for(CrcError)` instantiation.

## 7. Example

- [ ] 7.1 `examples/spi_probe_complete/`: targets `nucleo_g071rb`
      SPI1. Configures 16-bit data, 16 MHz clock, Mode 3, MSB-first,
      CRC with CCITT polynomial, Hardware NSS output with NSSP.
      Demonstrates every new lever in one demo.
- [ ] 7.2 Mirror configuration on `nucleo_f401re` SPI1.
- [ ] 7.3 Mirror on `same70_xplained` SPI0 (drop the STM32-only
      NSSP path; add the SAM per-CS-delay levers).

## 8. Hardware spot-check (3-board matrix)

- [ ] 8.1 SAME70 Xplained: run `spi_probe_complete`. Verify SPI
      loopback at 16 MHz with 0 CRC errors over 60 s.
- [ ] 8.2 STM32G0 Nucleo: same matrix.
- [ ] 8.3 STM32F4 Nucleo: same matrix.
- [ ] 8.4 Update `docs/SUPPORT_MATRIX.md` `spi` row to record the
      extended-coverage validation; consider promoting from
      `representative` to `foundational` if the 3-board matrix
      stays green.

## 9. Documentation

- [ ] 9.1 `docs/SPI.md` — comprehensive guide: model,
      variable-data-size recipe, CRC recipe, frame-format selection,
      NSS hardware-management recipe, per-CS timing on SAM,
      kernel-clock source, async wiring, modm migration table.
- [ ] 9.2 Reference `docs/SPI.md` from `docs/ASYNC.md` (under the
      SPI section) and `docs/COOKBOOK.md` (where applicable).
- [ ] 9.3 Cross-link from `docs/SUPPORT_MATRIX.md` `spi` row.

## 10. Out-of-scope follow-ups (filed but not done in this change)

- [ ] 10.1 File alloy-codegen `add-spi-cs-typed-enum` (mirror of
      `add-adc-channel-typed-enum`) so the HAL drops the
      `std::uint8_t` CS shim in favour of `SpiChipSelectOf<P>::type`.
- [ ] 10.2 File alloy `add-i2s-coverage` once the descriptor
      publishes the I²S-side fields for STM32 SPI peripherals that
      double as I²S audio.
- [ ] 10.3 File alloy `add-spi-circular-dma` — canonical
      ring-buffer-and-flag-when-half recipe.
