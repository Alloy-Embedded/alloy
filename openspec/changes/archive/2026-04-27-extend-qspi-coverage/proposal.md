# Extend QSPI Coverage To Match Published Descriptor Surface

## Why

`QspiSemanticTraits<P>` publishes control / mode / instruction-
frame / instruction-address / instruction-code registers, full
field set (instruction / address / data enable, dummy cycles,
bits-per-transfer, chip-select-mode, continuous-read-mode,
last-transfer, address-field, instruction-field, clock-gate,
enable / disable + status), capability flags (`kHasDma`,
`kHasScrambling`), `kKernelClockSelectorField`, and `kIrqNumbers`.

Alloy doesn't have a QSPI HAL today. This change ships the
descriptor-driven peripheral handle from scratch — the same
typed-method, capability-gated pattern the rest of the
extend-*-coverage series uses, but starting from zero.

## What Changes

### `src/hal/qspi/qspi.hpp` — descriptor-driven QSPI handle (new)

- **Frame configuration** (instruction + address + data phases):
  - `enum class FrameWidth { Single, Dual, Quad, Octal }`.
  - `set_instruction_width(FrameWidth)`,
    `set_address_width(FrameWidth)`,
    `set_data_width(FrameWidth)`.
  - `set_instruction(std::uint8_t opcode)`,
    `set_address(std::uint32_t addr, std::uint8_t bits = 24)`,
    `set_dummy_cycles(std::uint8_t)`.
- **Bits per transfer** (gated on
  `kBitsPerTransferField.valid`):
  `set_bits_per_transfer(std::uint8_t bits)`.
- **Chip-select mode** (gated on `kChipSelectModeField.valid`):
  `enum class CsMode { LowOnTransfer, HighOnTransfer,
  PerInstruction }`.
  `set_cs_mode(CsMode)`.
- **Continuous read** (gated on `kContinuousReadModeField.valid`):
  `enable_continuous_read(bool)`.
- **Memory-mapped mode** (descriptor publishes `kModeRegister`):
  `set_mode(QspiMode)` where `QspiMode` is `Indirect`,
  `MemoryMapped`, `AutoPoll`.
- **Transfer primitives**:
  `read(std::span<std::byte>) -> Result<void, ErrorCode>`,
  `write(std::span<const std::byte>) -> Result<void, ErrorCode>`,
  `last_transfer_done() -> bool`.
- **Scrambling** (gated on `kHasScrambling`):
  `enable_scrambling(bool)`,
  `set_scrambling_key(std::uint32_t)`.
- **Kernel clock + DMA**:
  `set_kernel_clock_source(KernelClockSource)`,
  `configure_dma(const DmaChannel&)` — gated on `kHasDma`.
- **Status / interrupts**:
  `enum class InterruptKind { TransferComplete, FifoThreshold,
  StatusMatch, Timeout, Error }`.
  `enable_interrupt(InterruptKind)` /
  `disable_interrupt(InterruptKind)`.
- **NVIC vector lookup**: `irq_numbers() ->
  std::span<const std::uint32_t>`.
- **Async sibling**: `async::qspi::wait_for(InterruptKind)` plus
  `async::qspi::read_dma` / `async::qspi::write_dma`.

### Driver layering

QSPI flash drivers (W25Q, MX25 NOR / NAND families) consume the
QSPI handle via the `BlockDevice` concept defined by
`add-filesystem-hal`. The handle provides quad-mode I/O; the
driver knows the W25Q command set.

### `examples/qspi_probe_complete/`

Targets `nucleo_f401re` (QSPI peripheral on F4 only of the
foundational set today). Configures Quad I/O Read at 4-line data
phase, 24-bit addressing, dummy cycles per W25Q datasheet,
memory-mapped mode reading from address 0x9000'0000.

### Docs

`docs/QSPI.md` — model, instruction / address / data phase
configuration recipe, memory-mapped vs indirect modes, dummy
cycles tuning, async wiring, modm migration table.

## What Does NOT Change

- This is the first version of the QSPI HAL. No migration
  concern.
- `BlockDevice` concept from `add-filesystem-hal` is unchanged;
  this change layers the QSPI handle below it.
- QSPI tier in `docs/SUPPORT_MATRIX.md` lands as
  `compile-review` until hardware spot-check matrix lands.

## Out of Scope (Follow-Up Changes)

- W25Q quad-mode flash driver. Tracked as `add-w25q-quad-driver`;
  consumes this handle.
- OctoSPI (HSPI on STM32H7). Tracked as `add-octospi-coverage`
  once an OctoSPI-capable family lands.
- ESP32 / RP2040 QSPI parity — gated on alloy-codegen.
- Hardware spot-checks → `validate-qspi-coverage-on-3-boards`.

## Alternatives Considered

Implementing QSPI as an extension of the existing SPI HAL —
rejected. Quad I/O is fundamentally different (instruction +
address + data phases with independent widths); shoehorning into
the SPI handle would either lose precision or bloat the SPI API.
