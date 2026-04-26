# Tasks: Extend SDMMC Coverage

## 1. Bus configuration + clock + command primitives

- [ ] 1.1 `enum class BusWidth { Bits1, Bits4, Bits8 }`,
      `set_bus_width(BusWidth)` — gated on `kBusWidthField.valid`.
      `Bits8` returns NotSupported on 4-bit-only controllers.
- [ ] 1.2 `set_clock_divider(std::uint16_t)`,
      `set_kernel_clock_source(KernelClockSource)`.
- [ ] 1.3 `struct CommandConfig { index, argument, response_type,
      wait_for_response }`,
      `enum class ResponseType { None, Short, Long, ShortBusy }`,
      `struct Response { type, raw[4] }`.
      `send_command(CommandConfig) -> Result<Response, ErrorCode>`.

## 2. Block transfer + DMA + timeouts

- [ ] 2.1 `set_block_size(std::uint16_t)`,
      `set_block_count(std::uint16_t)`.
- [ ] 2.2 `read_blocks(std::span<std::byte>) -> Result<void,
      ErrorCode>`,
      `write_blocks(std::span<const std::byte>) -> Result<void,
      ErrorCode>`.
- [ ] 2.3 `configure_dma(const DmaChannel&)`, `enable_dma(bool)`
      — gated on `kHasDma`.
- [ ] 2.4 `set_data_timeout(std::uint32_t cycles)`,
      `set_completion_timeout(std::uint32_t cycles)`.

## 3. Interrupts + IRQ vector + BlockDevice conformance

- [ ] 3.1 `enum class InterruptKind { CommandComplete,
      DataComplete, DataCrc, DataTimeout, CommandCrc,
      CommandTimeout, RxFifoFull, TxFifoEmpty, CardBusy,
      CardDetect }`.
      `enable_interrupt` / `disable_interrupt` — per-kind gated.
- [ ] 3.2 `irq_numbers() -> std::span<const std::uint32_t>`.
- [ ] 3.3 `port_handle<P>` satisfies `BlockDevice` from
      `add-filesystem-hal` (in main spec). `static_assert` in a
      new compile test.

## 4. Compile tests + async + example + HW

- [ ] 4.1 New `tests/compile_tests/test_sdmmc_api.cpp` against
      `same70_xplained` HSMCI.
- [ ] 4.2 `async::sdmmc::wait_for(InterruptKind)`,
      `async::sdmmc::read_blocks_dma(...)`,
      `async::sdmmc::write_blocks_dma(...)` runtime siblings.
- [ ] 4.3 `examples/sdmmc_probe_complete/`: targets
      `same70_xplained` HSMCI 4-bit at 25 MHz, FatFS mount, list
      root + write timestamped log.
- [ ] 4.4 SAME70 hardware spot-check: 1 MB write + read with
      MD5 verification.
- [ ] 4.5 Update `docs/SUPPORT_MATRIX.md` `sdmmc` row.

## 5. Documentation + follow-ups

- [ ] 5.1 `docs/SDMMC.md` — model, bus-width / clock recipe,
      command issuance, FatFS integration recipe.
- [ ] 5.2 Cross-link from `docs/FILESYSTEM.md` and
      `docs/ASYNC.md`.
- [ ] 5.3 File `add-emmc` follow-up.
- [ ] 5.4 File `add-sdmmc-uhs` follow-up.
