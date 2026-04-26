# Design: Extend SDMMC Coverage

## Context

Alloy doesn't have a dedicated SDMMC HAL — only SPI-mode SD card.
Descriptor publishes the full controller surface (command / data /
clock / bus-width / DMA). This change ships the descriptor-driven
handle and wires it into the existing `BlockDevice` concept.

## Goals

1. Ship a `port_handle<P>` SDMMC peripheral handle satisfying
   `BlockDevice` so FatFS / littlefs backends consume it unchanged.
2. Match modm-class SDMMC completeness (1 / 4 / 8-bit bus, full
   command set, DMA).
3. Compose with `complete-async-hal`'s pattern.

## Non-Goals

- eMMC, UHS-I high-speed modes.
- ESP32 / RP2040 SDMMC.
- A higher-level "filesystem-on-SD" reference (FatFS already does
  this once the BlockDevice plug is in).

## Key Decisions

### Decision 1: `BusWidth` typed enum tied to descriptor

`Bits1 / Bits4 / Bits8` — `Bits8` returns `NotSupported` on
controllers whose `kBusWidthField` width can't represent it (most
SDMMC peripherals cap at 4-bit; eMMC is the 8-bit user).

### Decision 2: `CommandConfig` + `Response` carry the SD-spec contract

```cpp
struct CommandConfig {
    std::uint8_t  index;        // CMD0..CMD63
    std::uint32_t argument;
    ResponseType  response_type;
    bool          wait_for_response;
};

struct Response {
    ResponseType type;
    std::array<std::uint32_t, 4> raw;  // up to 128 bits for long
};
```

The HAL doesn't know the SD-spec command set — that knowledge
lives in the BlockDevice driver layer. The HAL just issues the
command and returns the response.

### Decision 3: `BlockDevice` conformance

The HAL exposes `read_blocks` / `write_blocks` operating on
512-byte sectors (the universal SD/MMC sector size). The
BlockDevice adapter wraps these into FatFS-compatible
`disk_read` / `disk_write` callbacks.

## Risks

- **CMD line state machine.** Some commands require specific
  preconditions (e.g. APP_CMD before ACMD41). The HAL doesn't
  enforce; the driver does. Documented.
- **Card detection**. SDMMC peripherals usually have a card-detect
  pin separate from the data lines. The HAL exposes
  `enable_interrupt(CardDetect)` but the actual pin polling is
  GPIO-side; documented.

## Migration

First version of the HAL — no migration. The SPI-mode SD card
driver from `add-filesystem-hal` keeps working; users opt in to
SDMMC when they want speed.
