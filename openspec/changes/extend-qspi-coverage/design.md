# Design: Extend QSPI Coverage

## Context

Alloy doesn't have a QSPI HAL today. The descriptor publishes the
full instruction-frame / address / data / chip-select / dummy-
cycles / bits-per-transfer / continuous-read / scrambling
surface. This change ships the descriptor-driven handle from
scratch.

## Goals

1. Ship a `port_handle<P>` QSPI peripheral handle that lifts every
   published `QspiSemanticTraits<P>` field into typed methods.
2. Compose with `BlockDevice` concept from `add-filesystem-hal`
   so QSPI flash drivers (W25Q et al) layer cleanly.
3. Compose with `complete-async-hal`'s pattern.

## Non-Goals

- Specific flash driver (W25Q / MX25). Lives in `drivers/memory/`
  consuming this handle.
- OctoSPI (8-line). Tracked separately.
- ESP32 / RP2040 QSPI.

## Key Decisions

### Decision 1: Three independent `FrameWidth` setters

QSPI's instruction / address / data phases each have independent
line counts. The HAL exposes three separate setters
(`set_instruction_width`, `set_address_width`, `set_data_width`)
rather than one `set_frame_format(InstructionWidth, AddressWidth,
DataWidth)` because real-world commands vary across phases (e.g.
1-1-4 reads: 1-line instruction, 1-line address, 4-line data).

### Decision 2: `QspiMode { Indirect, MemoryMapped, AutoPoll }`

STM32 QUADSPI has three operating modes. The HAL consolidates
into a typed enum:

- `Indirect` — application drives transfers via `read` / `write`.
- `MemoryMapped` — flash appears at a fixed CPU address; reads
  are CPU loads.
- `AutoPoll` — polls a status register until a match condition.

Backends without a mode return `NotSupported`.

### Decision 3: `BlockDevice` concept conformance

The QSPI handle isn't itself a `BlockDevice` — that role belongs
to the flash driver. But the HAL's `read` / `write` / `set_address`
primitives are exactly what a `BlockDevice` adapter needs. The
existing `drivers/memory/w25q/w25q_block_device.hpp` adapter from
`add-filesystem-hal` keeps working; a quad-mode variant
`drivers/memory/w25q/w25q_quad_block_device.hpp` (separate
follow-up) consumes this QSPI handle.

## Risks

- **Dummy-cycle tuning is flash-specific.** The HAL exposes
  `set_dummy_cycles(u8)`; computing the correct value is the
  driver's job (W25Q datasheet specifies it as a function of
  clock rate).
- **Memory-mapped mode interaction with caches.** STM32 D-cache
  must be invalidated after writes for memory-mapped reads to
  see fresh data. Documented; the HAL exposes
  `invalidate_cache_for_memory_map()` as a hook.

## Migration

First version of the HAL — no migration.
