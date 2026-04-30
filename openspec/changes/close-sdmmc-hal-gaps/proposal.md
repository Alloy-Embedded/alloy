# Proposal: Close SDMMC HAL Gaps

## Status
`open` — response registers RSPR0–RSPR3 absent from IR, blocking card
identification and state machine.

## Problem

The SDMMC HAL can send commands and read the status register, but cannot
read command responses. The SD card state machine requires:

| Response type | Register         | Used for                                |
|---------------|------------------|-----------------------------------------|
| R1 (32-bit)   | SDMMC_RESP1      | CMD16 (block size), CMD24 (write), etc. |
| R2 (128-bit)  | RESP1+RESP2+RESP3+RESP4 | CID / CSD register read        |
| R3 (32-bit)   | SDMMC_RESP1      | ACMD41 OCR (voltage negotiation)        |
| R6 (32-bit)   | SDMMC_RESP1      | CMD3 RCA assignment                     |
| R7 (32-bit)   | SDMMC_RESP1      | CMD8 voltage check                      |

Without `RSPR0`–`RSPR3` in the semantic traits, the HAL cannot implement card
initialization (IDLE → READY → IDENTIFICATION → STANDBY → TRANSFER), blocking
all SDMMC functionality past the raw command send/receive level.

Additional gaps:

| Feature                 | Status  | Blocker                         |
|-------------------------|---------|---------------------------------|
| Data transfer start     | absent  | DTEN + DTDIR fields not in IR   |
| FIFO read/write         | absent  | SDMMC_FIFO register not in IR   |
| Block size config       | absent  | DBLOCKSIZE field not in IR      |
| Data timeout config     | absent  | DTIMER register not in IR       |
| DMA enable              | absent  | DMAEN field not in IR           |
| Card detect pin binding | absent  | No concept for CD pin           |

## Proposed Solution

### IR additions — response registers

```cpp
// Generated for STM32G0/F4/H7 after IR update:
template <>
struct SdmmcSemanticTraits<PeripheralId::Sdmmc1> {
    // --- existing ---
    static constexpr RuntimeRegisterRef kCmdReg       = { ... };
    static constexpr RuntimeRegisterRef kStatusReg    = { ... };
    // --- NEW ---
    static constexpr RuntimeRegisterRef kResp1Reg     = { SDMMC_RESP1, 0, 32, true };
    static constexpr RuntimeRegisterRef kResp2Reg     = { SDMMC_RESP2, 0, 32, true };
    static constexpr RuntimeRegisterRef kResp3Reg     = { SDMMC_RESP3, 0, 32, true };
    static constexpr RuntimeRegisterRef kResp4Reg     = { SDMMC_RESP4, 0, 32, true };
    // --- data transfer ---
    static constexpr RuntimeFieldRef    kDtenField    = { SDMMC_DCTRL, 0, 1, true };
    static constexpr RuntimeFieldRef    kDtdirField   = { SDMMC_DCTRL, 1, 1, true };
    static constexpr RuntimeFieldRef    kDblocksizeField = { SDMMC_DCTRL, 4, 4, true };
    static constexpr RuntimeRegisterRef kFifoReg      = { SDMMC_FIFO, 0, 32, true };
    static constexpr RuntimeRegisterRef kDtimerReg    = { SDMMC_DTIMER, 0, 32, true };
    static constexpr RuntimeRegisterRef kDlenReg      = { SDMMC_DLEN, 0, 25, true };
    static constexpr RuntimeFieldRef    kDmaenField   = { SDMMC_DCTRL, 3, 1, true };
};
```

### HAL response API

```cpp
// src/hal/sdmmc/sdmmc_handle.hpp
auto read_response_r1() -> core::Result<uint32_t, core::ErrorCode>;
auto read_response_r2() -> core::Result<std::array<uint32_t, 4>, core::ErrorCode>;
auto read_response_r3() -> core::Result<uint32_t, core::ErrorCode>;  // alias r1

// Higher-level: send command + wait for response in one call
auto send_command_r1(uint8_t cmd, uint32_t arg)
    -> core::Result<uint32_t, core::ErrorCode>;
auto send_command_r2(uint8_t cmd, uint32_t arg)
    -> core::Result<std::array<uint32_t, 4>, core::ErrorCode>;
```

### SD card initialization state machine

```cpp
// src/hal/sdmmc/sd_card.hpp
template <typename SdmmcHandle, typename CdPin = detail::NoCdPin>
class SdCard {
public:
    auto init() -> core::Result<void, core::ErrorCode>;
    auto read_block(uint32_t block, std::span<std::byte, 512> buf)
        -> core::Result<void, core::ErrorCode>;
    auto write_block(uint32_t block, std::span<const std::byte, 512> buf)
        -> core::Result<void, core::ErrorCode>;
    auto card_size_blocks() const -> uint32_t;
};
```

`SdCard::init()` implements the full SD card initialization sequence:
CMD0 (IDLE) → CMD8 (voltage check) → ACMD41 (wait ready) →
CMD2 (CID) → CMD3 (RCA) → CMD7 (select) → CMD16 (block size 512).

### Card detect pin binding

```cpp
// Optional CdPin: any pin_handle that has read()
template <typename SdmmcHandle, typename CdPin>
class SdCard {
    auto is_card_present() -> bool {
        if constexpr (!std::is_same_v<CdPin, detail::NoCdPin>)
            return _cd.read().value_or(false);
        return true;  // assume present if no CD pin
    }
};
```

## Impact

- Unblocks FATFS over SDMMC (filesystem layer).
- Enables the `FlashWriter` concept for `OtaClient` backed by SDMMC.
- Enables SdCard as a `BlockStorage` driver in the driver registry.
