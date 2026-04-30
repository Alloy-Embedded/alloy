# Tasks: Close SDMMC HAL Gaps

Host-testable: phases 1–3. Phase 4 requires hardware (microSD card + board).

## 1. IR additions

- [ ] 1.1 Add SDMMC response registers to STM32G0 IR:
      `kResp1Reg`, `kResp2Reg`, `kResp3Reg`, `kResp4Reg`
      (SDMMC_RESP1 = 0x14, RESP2=0x18, RESP3=0x1C, RESP4=0x20 offsets from SDMMC base).
- [ ] 1.2 Add data control fields to STM32G0 IR:
      `kDtenField`, `kDtdirField`, `kDblocksizeField`, `kDmaenField`.
      Registers: `kFifoReg` (SDMMC_FIFO), `kDtimerReg`, `kDlenReg`.
- [ ] 1.3 Add same fields for STM32F4 and STM32H7 IR (different base addresses,
      same field layout).
- [ ] 1.4 Add SDMMC IR entries for SAME70 HSMCI peripheral:
      HSMCI_RDR (receive data), HSMCI_RSPR0–RSPR3, HSMCI_DMA.
- [ ] 1.5 Extend `alloy-cpp-emit` SDMMC template; regen all three families;
      verify `kResp1Reg.valid = true` in generated traits.
- [ ] 1.6 Update `cmake/hal-contracts/sdmmc.json`:
      add `resp1_reg`, `resp2_reg`, `data_transfer_fields` to `required` list.

## 2. HAL response methods

- [ ] 2.1 Implement `read_response_r1()` in `sdmmc_handle.hpp`:
      reads `kResp1Reg`; returns `ErrorCode::NotSupported` if not valid.
- [ ] 2.2 Implement `read_response_r2()`: reads all four response registers
      into `std::array<uint32_t, 4>`.
- [ ] 2.3 Implement `send_command_r1(cmd, arg)`: send CMD, wait `CMDREND` flag,
      call `read_response_r1()`. Return combined status + response.
- [ ] 2.4 Implement `send_command_r2(cmd, arg)`.
- [ ] 2.5 Implement data transfer helpers:
      `start_data_transfer(dir, block_size, length)`,
      `read_fifo_word() -> uint32_t`,
      `write_fifo_word(uint32_t)`.
- [ ] 2.6 Add compile test `tests/compile_tests/test_sdmmc_api.cpp`:
      call `read_response_r1()`, `read_response_r2()`, `send_command_r1()`,
      `start_data_transfer()`.

## 3. SdCard high-level driver

- [ ] 3.1 Create `src/hal/sdmmc/sd_card.hpp`:
      `SdCard<SdmmcHandle, CdPin = NoCdPin>` template class.
- [ ] 3.2 Implement `SdCard::init()`:
      full SD v2.0 initialization sequence (CMD0, CMD8, ACMD41, CMD2, CMD3, CMD7, CMD16).
      Handle SDHC/SDXC flag from OCR response.
- [ ] 3.3 Implement `SdCard::read_block(block, buf)`:
      CMD17 (READ_SINGLE_BLOCK), start transfer, read 512 bytes from FIFO.
- [ ] 3.4 Implement `SdCard::write_block(block, buf)`:
      CMD24 (WRITE_BLOCK), start transfer, write 512 bytes to FIFO, wait DBCKEND.
- [ ] 3.5 Implement `SdCard::card_size_blocks()`:
      read CSD register (CMD9 → R2 response); parse C_SIZE field.
- [ ] 3.6 Add `static_assert(BlockStorage<SdCard<MockSdmmc>>)` (uses driver registry concept).
- [ ] 3.7 Add compile test: instantiate `SdCard<MockSdmmc>`, call all methods.

## 4. Hardware validation

- [ ] 4.1 same70_xplained + microSD: `SdCard::init()` succeeds; verify
      `card_size_blocks()` returns expected value for known test card.
- [ ] 4.2 same70_xplained: write 512-byte block at block 0x1000; read back;
      verify data matches.
- [ ] 4.3 same70_xplained + FATFS: mount FAT32 filesystem;
      `f_open` / `f_write` / `f_close` creates file; verify on host PC.

## 5. Documentation

- [ ] 5.1 Update `docs/SDMMC_HAL.md`: document response API, SdCard class,
      FATFS integration snippet.
- [ ] 5.2 Add `examples/sdmmc_fatfs/` for same70_xplained.
