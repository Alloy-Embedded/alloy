# Tasks: Extend SPI Coverage

Phases ordered. Phases 1-4 are host-testable (compile + concept).
Phase 5 needs the existing 3-board hardware matrix
(SAME70 / STM32G0 / STM32F4).

## 1. Variable data size, clock speed, kernel-clock source

- [x] 1.1 `set_data_size(std::uint8_t bits)` — gated on
      `kDsField.valid || kDffField.valid`. Validates the requested
      width against the published field's encoding range and returns
      `core::ErrorCode::InvalidArgument` for unrealisable widths
      (e.g. `Bits12` on an `kDffField`-only peripheral).
- [x] 1.2 `set_clock_speed(std::uint32_t hz)` — resolves the BR
      divider from the peripheral kernel clock. Returns
      `InvalidArgument` if the realised rate falls outside ±5 % of
      the requested rate.
- [x] 1.3 `realised_clock_speed() -> std::uint32_t` — returns the
      rate the current BR encoding produces; lets callers verify
      what they actually got after `set_clock_speed`.
- [ ] 1.4 `set_kernel_clock_source(KernelClockSource)` — gated on
      `kKernelClockSelectorField.valid`; runtime-validates against
      the published option list.
      Deferred: depends on `extend-uart-coverage` exporting the
      shared `KernelClockSource` enum into `alloy::device`. None of
      the foundational SPI peripherals (G0 / F4 / SAME70) publish a
      kernel-clock-selector field, so the implementation would
      currently always return NotSupported.
- [x] 1.5 `kernel_clock_hz() -> std::uint32_t` — same shape as the
      UART variant.

## 2. Frame format, CRC, bidirectional, NSS management

- [x] 2.1 `enum class FrameFormat { Motorola, TI }`.
      `set_frame_format(FrameFormat)` — gated independently on
      `kSupportsMotorolaFrame` / `kSupportsTiFrame`.
- [x] 2.2 CRC (gated on `kSupportsCrc`):
      `enable_crc(bool)`,
      `set_crc_polynomial(std::uint16_t)`,
      `read_crc() -> std::uint16_t`,
      `crc_error() -> bool`,
      `clear_crc_error() -> Result<void, ErrorCode>`.
- [x] 2.3 Bidirectional 3-wire (gated on `kSupportsBidirectional3Wire`):
      `set_bidirectional(bool)`,
      `enum class BiDir { Receive, Transmit }`,
      `set_bidirectional_direction(BiDir)`.
- [x] 2.4 Hardware NSS management (gated on `kSupportsNssHwManagement`):
      `enum class NssManagement { Software, HardwareInput, HardwareOutput }`.
      `set_nss_management(NssManagement)`,
      `set_nss_pulse_per_transfer(bool)`.

## 3. SAM-style per-CS timing

- [x] 3.1 `set_cs_decode_mode(bool)` — gated on `kPcsdecField.valid`
      (PCS bits decoded as 1-of-N vs 4-of-15 direct).
- [x] 3.2 `set_cs_delay_between_consecutive(std::uint16_t cycles)` —
      wired through `kDlybctField`.
- [x] 3.3 `set_cs_delay_clock_to_active(std::uint16_t cycles)` —
      wired through `kDlybsField`.
- [x] 3.4 `set_cs_delay_active_to_clock(std::uint16_t cycles)` —
      wired through `kDlybcsField`.

## 4. Status flags, interrupts, IRQ vector lookup

- [x] 4.1 Status flag accessors (gated per-field):
      `tx_register_empty()`, `rx_register_not_empty()`, `busy()`,
      `mode_fault()`, `frame_format_error()`. Each has its
      `clear_*` mirror where the descriptor publishes a clear-side
      field (currently `clear_mode_fault` for STM32; CRC error clear
      lives in 2.2).
- [x] 4.2 `enum class InterruptKind { Txe, Rxne, Error, ModeFault,
      CrcError, FrameError }`.
      `enable_interrupt(InterruptKind)` /
      `disable_interrupt(InterruptKind)` — each gated on the
      corresponding control-side IE field.
- [x] 4.3 `irq_numbers() -> std::span<const std::uint32_t>` —
      `static constexpr` returning the descriptor's `kIrqNumbers`.

## 5. Compile tests

- [x] 5.1 Extend `tests/compile_tests/test_spi_api.cpp` to
      instantiate every new method against `nucleo_g071rb` SPI1.
      `set_data_size_static<Bits>` enforces compile-time
      `static_assert(Bits >= 4 && Bits <= 16)` for `Bits3` / `Bits17`.
- [x] 5.2 SAME70 path covered by the same shared body of
      `exercise_spi_backend()` — the test exercises every per-CS
      timing setter on `same70_xplained` SPI0 (DLYBCS / DLYBCT /
      DLYBS).
- [x] 5.3 STM32F4 path covered by the same shared test body
      against `nucleo_f401re` SPI1 (CRC + frame-format + NSS
      hardware output).

## 6. Async integration

- [x] 6.1 `async::spi::wait_for<Kind>(port)` — sibling of the
      existing DMA wrappers; signals on the requested
      interrupt-kind token.
- [x] 6.2 `tests/compile_tests/test_async_peripherals.cpp` extended
      with a `wait_for<CrcError>` and `wait_for<ModeFault>`
      instantiation against the `MockSpiPort`.

## 7. Example

- [x] 7.1 `examples/spi_probe_complete/`: targets all foundational
      boards. Configures the bus, then exercises every Phase 1-4
      lever and prints OK / NotSupported per feature.
- [x] 7.2 Builds clean on `nucleo_f401re` SPI1.
- [x] 7.3 Builds clean on `same70_xplained` SPI0.

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

- [x] 9.1 `docs/SPI.md` — comprehensive guide: model,
      variable-data-size recipe, CRC recipe, frame-format selection,
      NSS hardware-management recipe, per-CS timing on SAM,
      kernel-clock source, async wiring, modm migration table.
- [x] 9.2 Reference `docs/SPI.md` from `docs/ASYNC.md` (under the
      SPI section).
- [ ] 9.3 Cross-link from `docs/SUPPORT_MATRIX.md` `spi` row.
      Deferred to the same follow-up that records the hardware
      spot-check (task 8.4).

## 10. Out-of-scope follow-ups (filed but not done in this change)

- [ ] 10.1 File alloy-codegen `add-spi-cs-typed-enum` (mirror of
      `add-adc-channel-typed-enum`) so the HAL drops the
      `std::uint8_t` CS shim in favour of `SpiChipSelectOf<P>::type`.
- [ ] 10.2 File alloy `add-i2s-coverage` once the descriptor
      publishes the I²S-side fields for STM32 SPI peripherals that
      double as I²S audio.
- [ ] 10.3 File alloy `add-spi-circular-dma` — canonical
      ring-buffer-and-flag-when-half recipe.
