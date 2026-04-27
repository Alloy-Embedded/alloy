# Tasks: Extend QSPI Coverage

## 1. Frame configuration + transfer primitives

- [x] 1.1 `enum class FrameWidth { Single, Dual, Quad, Octal }`.
      `set_instruction_width(FrameWidth)`,
      `set_address_width(FrameWidth)`,
      `set_data_width(FrameWidth)`.
- [x] 1.2 `set_instruction(std::uint8_t opcode)`,
      `set_address(std::uint32_t addr, std::uint8_t bits = 24)`,
      `set_dummy_cycles(std::uint8_t)`.
- [x] 1.3 `set_bits_per_transfer(std::uint8_t bits)` — gated on
      `kBitsPerTransferField.valid`.
- [x] 1.4 `read(std::span<std::byte>)`,
      `write(std::span<const std::byte>)`,
      `last_transfer_done() -> bool`.

## 2. Mode + chip-select + continuous-read + scrambling

- [x] 2.1 `enum class QspiMode { Indirect, MemoryMapped, AutoPoll }`.
      `set_mode(QspiMode)`.
- [x] 2.2 `enum class CsMode { LowOnTransfer, HighOnTransfer,
      PerInstruction }`.
      `set_cs_mode(CsMode)` — gated on
      `kChipSelectModeField.valid`.
- [x] 2.3 `enable_continuous_read(bool)` — gated on
      `kContinuousReadModeField.valid`.
- [x] 2.4 Scrambling (gated on `kHasScrambling`):
      `enable_scrambling(bool)`,
      `set_scrambling_key(std::uint32_t)`.
- [x] 2.5 `invalidate_cache_for_memory_map()` — hook for D-cache
      invalidation after writes when in MemoryMapped mode.

## 3. Kernel clock + DMA + interrupts + IRQ vector

- [x] 3.1 `set_kernel_clock_source(KernelClockSource)`.
- [x] 3.2 `configure_dma(const DmaChannel&)` — gated on `kHasDma`.
- [x] 3.3 `enum class InterruptKind { TransferComplete,
      FifoThreshold, StatusMatch, Timeout, Error }`.
      `enable_interrupt` / `disable_interrupt` — per-kind gated.
- [x] 3.4 `irq_numbers() -> std::span<const std::uint32_t>`.

## 4. Compile tests + async + example + HW

- [x] 4.1 New `tests/compile_tests/test_qspi_api.cpp` against
      `nucleo_f401re` QSPI.
- [ ] 4.2 `async::qspi::wait_for(InterruptKind)`,
      `async::qspi::read_dma(...)`,
      `async::qspi::write_dma(...)` runtime siblings.
- [ ] 4.3 `examples/qspi_probe_complete/`: targets `nucleo_f401re`,
      Quad I/O Read at 1-1-4, 24-bit addressing, MemoryMapped mode.
- [ ] 4.4 STM32F4 hardware spot-check with W25Q128: 1 MB
      sequential read with CRC verification.
- [ ] 4.5 Update `docs/SUPPORT_MATRIX.md` `qspi` row.

## 5. Documentation + follow-ups

- [ ] 5.1 `docs/QSPI.md` — model, frame-phase configuration,
      memory-mapped vs indirect modes, dummy-cycle tuning.
- [ ] 5.2 Cross-link from `docs/ASYNC.md` and `docs/FILESYSTEM.md`.
- [ ] 5.3 File `add-w25q-quad-driver` follow-up.
- [ ] 5.4 File `add-octospi-coverage` follow-up.
