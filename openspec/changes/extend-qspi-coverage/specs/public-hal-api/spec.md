# public-hal-api Spec Delta: QSPI Coverage Extension

## ADDED Requirements

### Requirement: QSPI HAL SHALL Expose Per-Phase Frame Width Setters

The `alloy::hal::qspi::port_handle<P>` MUST expose
`enum class FrameWidth { Single, Dual, Quad, Octal }` plus three
independent setters:
`set_instruction_width(FrameWidth)`,
`set_address_width(FrameWidth)`,
`set_data_width(FrameWidth)`.

The HAL MUST also expose
`set_instruction(std::uint8_t opcode)`,
`set_address(std::uint32_t addr, std::uint8_t bits = 24)`, and
`set_dummy_cycles(std::uint8_t)`.

#### Scenario: STM32F4 QSPI runs 1-1-4 Quad I/O Read

- **WHEN** an application calls
  `qspi.set_instruction_width(FrameWidth::Single)`,
  `qspi.set_address_width(FrameWidth::Single)`,
  `qspi.set_data_width(FrameWidth::Quad)`,
  `qspi.set_instruction(0xEB)`,
  `qspi.set_address(0x000'0000, 24)`,
  `qspi.set_dummy_cycles(8)` on QSPI of `nucleo_f401re` with a
  W25Q128 wired
- **THEN** all calls succeed and the next read traverses the W25Q
  Quad I/O Read command path

### Requirement: QSPI HAL SHALL Expose Mode / CS / Continuous-Read / Scrambling Per Capability

The HAL MUST expose:

- `enum class QspiMode { Indirect, MemoryMapped, AutoPoll }`,
  `set_mode(QspiMode)`.
- `enum class CsMode { LowOnTransfer, HighOnTransfer,
  PerInstruction }`,
  `set_cs_mode(CsMode)` — gated on
  `kChipSelectModeField.valid`.
- `enable_continuous_read(bool)` — gated on
  `kContinuousReadModeField.valid`.
- Scrambling (`kHasScrambling`):
  `enable_scrambling(bool)`,
  `set_scrambling_key(std::uint32_t)`.
- `invalidate_cache_for_memory_map()` hook for post-write
  D-cache invalidation in MemoryMapped mode.

Backends without a capability MUST return
`core::ErrorCode::NotSupported`.

#### Scenario: MemoryMapped mode lets CPU read flash directly

- **WHEN** an application calls
  `qspi.set_mode(QspiMode::MemoryMapped)` on `nucleo_f401re` QSPI
  configured for W25Q128 1-1-4 reads
- **THEN** the call succeeds
- **AND** subsequent loads from `0x9000'0000` return W25Q
  contents at offset `addr - 0x9000'0000` without any further HAL
  calls

### Requirement: QSPI HAL SHALL Expose Transfer Primitives + Kernel Clock + DMA

The HAL MUST expose:

- `read(std::span<std::byte>) -> Result<void, ErrorCode>`,
  `write(std::span<const std::byte>) -> Result<void, ErrorCode>`,
  `last_transfer_done() -> bool`.
- `set_bits_per_transfer(std::uint8_t)` — gated on
  `kBitsPerTransferField.valid`.
- `set_kernel_clock_source(KernelClockSource)`.
- `configure_dma(const DmaChannel&)` — gated on `kHasDma`.

#### Scenario: Indirect read drains 1024 bytes without DMA

- **WHEN** an application configures Indirect mode and calls
  `qspi.read(buffer_1024)` after setting instruction + address
- **THEN** the call returns `Ok` and `buffer_1024` contains the
  flash data at the configured address

### Requirement: QSPI HAL SHALL Expose Typed Interrupt Setters And IRQ Number List

The HAL MUST expose
`enum class InterruptKind { TransferComplete, FifoThreshold,
StatusMatch, Timeout, Error }` plus
`enable_interrupt(InterruptKind)` /
`disable_interrupt(InterruptKind)` (each kind gated), and
`irq_numbers() -> std::span<const std::uint32_t>`.

#### Scenario: StatusMatch interrupt fires when AutoPoll matches

- **WHEN** an application configures AutoPoll mode reading the
  W25Q WIP bit and calls
  `qspi.enable_interrupt(InterruptKind::StatusMatch)`
- **THEN** the interrupt fires when the polled status matches the
  expected value

### Requirement: Async QSPI Adapter SHALL Add wait_for / read_dma / write_dma

The runtime `async::qspi` namespace MUST expose
`wait_for<P>(handle, InterruptKind kind)`,
`read_dma<P>(handle, dma_channel, std::span<std::byte>)`, and
`write_dma<P>(handle, dma_channel, std::span<const std::byte>)`
returning `Result<operation<…>, ErrorCode>`.

#### Scenario: Coroutine writes 4 KB via DMA

- **WHEN** a task awaits
  `async::qspi::write_dma<QSPI>(qspi, dma_ch, page_4k)`
- **THEN** the task resumes when the DMA TC interrupt fires and
  the awaiter returns `Ok`
