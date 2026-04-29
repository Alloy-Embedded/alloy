# Extend SDMMC Coverage To Match Published Descriptor Surface

## Why

`SdmmcSemanticTraits<P>` publishes argument / command / block /
config / control / data-timeout / completion-timeout registers,
full field set (block-count + block-length + bus-width +
clock-divider + clock-gate + command-index + command-ready +
DMA-enable + atacs), capability flag `kHasDma`, plus
`kKernelClockSelectorField` and `kIrqNumbers`.

Alloy doesn't have an SDMMC HAL today — only the SD-card-over-SPI
driver from `add-filesystem-hal`. This change ships the
descriptor-driven peripheral handle for the dedicated SDMMC
controller, which is faster than SPI mode and supports 4-bit /
8-bit bus widths plus high-speed clocks.

## What Changes

### `src/hal/sdmmc/sdmmc.hpp` — descriptor-driven SDMMC handle (new)

- **Bus configuration**:
  - `enum class BusWidth { Bits1, Bits4, Bits8 }`.
    `set_bus_width(BusWidth)` — gated on `kBusWidthField.valid`.
  - `set_clock_divider(std::uint16_t)` — direct write through
    `kClockDividerField`.
  - `set_kernel_clock_source(KernelClockSource)` — gated.
- **Command issuance**:
  - `struct CommandConfig { index, argument, response_type,
    wait_for_response }`.
  - `enum class ResponseType { None, Short, Long, ShortBusy }`.
  - `send_command(CommandConfig) -> Result<Response, ErrorCode>`.
  - `last_response() -> Response` (where `Response` carries up to
    128 bits for long responses).
- **Block transfer** (gated on `kBlockRegister.valid`):
  - `set_block_size(std::uint16_t bytes)`,
    `set_block_count(std::uint16_t)`.
  - `read_blocks(std::span<std::byte>) -> Result<void, ErrorCode>`,
    `write_blocks(std::span<const std::byte>) -> Result<void,
    ErrorCode>`.
- **DMA** (gated on `kHasDma`):
  - `configure_dma(const DmaChannel& channel)`,
    `enable_dma(bool)`.
- **Timeouts**:
  - `set_data_timeout(std::uint32_t cycles)`,
    `set_completion_timeout(std::uint32_t cycles)`.
- **Status / interrupts**:
  - `enum class InterruptKind { CommandComplete, DataComplete,
    DataCrc, DataTimeout, CommandCrc, CommandTimeout, RxFifoFull,
    TxFifoEmpty, CardBusy, CardDetect }`.
  - `enable_interrupt(InterruptKind)` /
    `disable_interrupt(InterruptKind)`.
- **NVIC vector lookup**: `irq_numbers() ->
  std::span<const std::uint32_t>`.
- **Async sibling**: `async::sdmmc::wait_for(InterruptKind)`,
  `async::sdmmc::read_blocks_dma`,
  `async::sdmmc::write_blocks_dma`.

### Concept conformance

`port_handle<P>` MUST satisfy `BlockDevice` from
`add-filesystem-hal` (already absorbed into main spec). This lets
the existing FatFS / littlefs backends consume the SDMMC handle
unchanged — same way they consume the SPI-mode SD card today.

### `examples/sdmmc_probe_complete/`

Targets `same70_xplained` HSMCI. Configures 4-bit bus at 25 MHz,
mounts FAT32 via `FatfsBackend<port_handle<HSMCI>>`, lists root
directory, writes a timestamped log entry.

### Docs

`docs/SDMMC.md` — model, bus-width / clock recipe, command
issuance, block transfer, DMA, FatFS integration recipe.

## What Does NOT Change

- The SPI-mode SD card driver from `add-filesystem-hal` is
  unchanged. Users who want SPI mode keep using it; users who
  want native SDMMC use this new handle.
- `BlockDevice` concept is unchanged.

## Out of Scope (Follow-Up Changes)

- eMMC support (boot partitions, RPMB). Tracked as `add-emmc`.
- UHS-I high-speed modes (DDR50, SDR104). Tracked as
  `add-sdmmc-uhs`.
- ESP32 / RP2040 SDMMC parity — gated on alloy-codegen.
- Hardware spot-checks → `validate-sdmmc-coverage-on-3-boards`.

## Alternatives Considered

Layering SDMMC on top of the SPI handle — rejected. SDMMC is a
completely different protocol (CMD lines + 4 / 8 data lines vs
1 + 1 data); the only thing they share is the SD-spec command
set, which is encoded at the BlockDevice driver layer.
