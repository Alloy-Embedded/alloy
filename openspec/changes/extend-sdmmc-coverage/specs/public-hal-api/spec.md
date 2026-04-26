# public-hal-api Spec Delta: SDMMC Coverage Extension

## ADDED Requirements

### Requirement: SDMMC HAL SHALL Expose Bus Width / Clock Setters

The `alloy::hal::sdmmc::port_handle<P>` MUST expose
`enum class BusWidth { Bits1, Bits4, Bits8 }` plus
`set_bus_width(BusWidth)` (gated on `kBusWidthField.valid`),
`set_clock_divider(std::uint16_t)`, and
`set_kernel_clock_source(KernelClockSource)`. `Bits8` MUST return
`core::ErrorCode::NotSupported` on controllers whose
`kBusWidthField` width can't represent it.

#### Scenario: SAM-E70 HSMCI runs 4-bit bus at 25 MHz

- **WHEN** an application calls
  `sd.set_bus_width(BusWidth::Bits4)` and
  `sd.set_clock_divider(div_for_25mhz)` on HSMCI of
  `same70_xplained`
- **THEN** both calls succeed and the SD bus runs at 25 MHz with
  4 data lines

### Requirement: SDMMC HAL SHALL Expose Command Issuance Primitives

The HAL MUST expose
`struct CommandConfig { index, argument, response_type,
wait_for_response }`,
`enum class ResponseType { None, Short, Long, ShortBusy }`,
`struct Response { type, raw[4] }`, and
`send_command(CommandConfig) -> Result<Response, ErrorCode>`. The
HAL MUST NOT hardcode the SD-spec command set; that knowledge
lives in the BlockDevice driver layer above.

#### Scenario: CMD0 (GO_IDLE_STATE) lands without response

- **WHEN** an application calls
  `sd.send_command({.index = 0, .argument = 0,
  .response_type = ResponseType::None, .wait_for_response = false})`
- **THEN** the command issues and the call returns `Ok` with an
  empty `Response`

#### Scenario: CMD2 (ALL_SEND_CID) returns long response

- **WHEN** an application calls
  `sd.send_command({.index = 2, .argument = 0,
  .response_type = ResponseType::Long, .wait_for_response = true})`
- **THEN** the call returns `Ok` with a `Response` whose `raw[]`
  carries the 128-bit CID

### Requirement: SDMMC HAL SHALL Expose Block Transfer Primitives And DMA

The HAL MUST expose
`set_block_size(std::uint16_t)`,
`set_block_count(std::uint16_t)`,
`read_blocks(std::span<std::byte>) -> Result<void, ErrorCode>`,
`write_blocks(std::span<const std::byte>) -> Result<void,
ErrorCode>`,
`configure_dma(const DmaChannel&)` (gated on `kHasDma`),
`enable_dma(bool)`,
`set_data_timeout(std::uint32_t)`, and
`set_completion_timeout(std::uint32_t)`.

#### Scenario: 4 KB write completes via DMA

- **WHEN** an application configures DMA, calls
  `sd.set_block_size(512)`, `sd.set_block_count(8)`, then
  `sd.write_blocks(buffer_4096)`
- **THEN** the call returns `Ok` once the DMA TC + DataComplete
  interrupts fire

### Requirement: SDMMC HAL SHALL Satisfy The BlockDevice Concept

`port_handle<P>` MUST satisfy
`alloy::hal::filesystem::BlockDevice` (defined by archived
`add-filesystem-hal`) so existing FatFS / littlefs backends
consume the SDMMC handle unchanged. A compile test MUST
`static_assert` conformance.

#### Scenario: FatFS backend mounts on SDMMC handle

- **WHEN** an application instantiates
  `Filesystem<FatfsBackend<port_handle<HSMCI>>>` and calls
  `mount()`
- **THEN** the call succeeds and FatFS reads / writes the SD
  card at speeds higher than the SPI-mode equivalent

### Requirement: SDMMC HAL SHALL Expose Typed Interrupt Setters And IRQ Number List

The HAL MUST expose
`enum class InterruptKind { CommandComplete, DataComplete,
DataCrc, DataTimeout, CommandCrc, CommandTimeout, RxFifoFull,
TxFifoEmpty, CardBusy, CardDetect }` plus
`enable_interrupt(InterruptKind)` /
`disable_interrupt(InterruptKind)` (each kind gated), and
`irq_numbers() -> std::span<const std::uint32_t>`.

#### Scenario: CardDetect IRQ fires on insertion

- **WHEN** an application calls
  `sd.enable_interrupt(InterruptKind::CardDetect)` and a card is
  inserted
- **THEN** the card-detect interrupt fires

### Requirement: Async SDMMC Adapter SHALL Add wait_for / read_blocks_dma / write_blocks_dma

The runtime `async::sdmmc` namespace MUST expose
`wait_for<P>(handle, InterruptKind kind)`,
`read_blocks_dma<P>(handle, dma_channel,
std::span<std::byte>)`, and
`write_blocks_dma<P>(handle, dma_channel,
std::span<const std::byte>)` returning
`Result<operation<…>, ErrorCode>`.

#### Scenario: Coroutine reads 64 KB via DMA

- **WHEN** a task awaits
  `async::sdmmc::read_blocks_dma<HSMCI>(sd, dma_ch, buffer_64k)`
- **THEN** the task resumes when both the DMA TC and SDMMC
  DataComplete interrupts fire and the awaiter returns `Ok`
