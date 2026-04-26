# Design: ESP32 Classic Family With Dual-Core Bring-Up

## Context
Adding the original ESP32 (LX6, dual-core) is the first time alloy crosses three lines
at once: a new MCU family, a new architecture variant, and a true second-core bring-up.
Each of those is small in isolation; together they define how alloy will look when it
genuinely runs on multi-core targets. This design picks the smallest set of decisions
that lets the bring-up land without locking the project into choices we cannot revisit.

The two boards driving the work (WROVER-KIT v4.1, DevKitC v4) are testbed targets for
the maintainer. The runtime bar is a working `blink` plus a `time_probe` that uses both
cores -- enough to prove the architecture, not enough to claim foundational status.

## Goals / Non-Goals

Goals:
- One coherent runtime path for ESP32-classic that mirrors the ESP32-S3 path (descriptor
  → board → public HAL → application).
- Both Xtensa cores running, with a small, vendor-neutral surface for "what core am I
  on" so application code can pin work without learning ESP terminology.
- The architecture rename (`xtensa` → `xtensa-lx7` plus new `xtensa-lx6`) lands in the
  same change as its first user, so we never ship an ambiguous arch enum.
- Toolchain acquisition continues to flow through `alloy toolchain install` -- no
  Espressif installer, no `idf.py`.

Non-Goals:
- ESP-IDF compatibility. The boards must build through the same descriptor-driven CMake
  path used for every other vendor.
- WiFi, BLE, OTA, NVS, partition tables. These belong to a future IDF-integration
  proposal; including them here would force premature decisions about FreeRTOS.
- PSRAM, LCD, camera, microSD bring-up on WROVER-KIT. The board exposes them; v1
  declares them as TODO so the user can extend the board layer.
- Cross-core synchronisation primitives beyond what dual-core startup itself needs.
  Application-level multi-core scheduling is out of scope until there is a real user.

## Decisions

### Decision: Architecture enum migration is part of this change
The `xtensa` value in `_ALLOY_VALID_ARCHES` (introduced by `add-custom-board-bringup`)
is ambiguous between LX6 and LX7. We rename it to `xtensa-lx7` for ESP32-S3 and
introduce `xtensa-lx6` for ESP32-classic in the same change.

- Rationale: shipping an ambiguous enum and then breaking it later is worse than the
  one-time migration cost. Today there is exactly one in-tree consumer (`esp32s3_devkitc`)
  and zero downstream consumers (`add-custom-board-bringup` has not merged yet).
- Trade-off accepted: the custom-board contract documented in
  `docs/CUSTOM_BOARDS.md` lists accepted arch values; this change updates that table.

### Decision: Toolchain mapping is by arch, not by board
`scaffold.py::_toolchain_for_arch` already maps arch to toolchain. We extend it:
`xtensa-lx6 → xtensa-esp32-elf-gcc`, `xtensa-lx7 → xtensa-esp32s3-elf-gcc`,
`riscv32 → riscv32-esp-elf-gcc`. Boards declare the arch; the arch picks the
toolchain. The catalog still records the toolchain name explicitly so `alloy boards`
can show it, but the source of truth is arch.

### Decision: Dual-core bring-up sequence stays in the descriptor-driven runtime
The boot CPU (PRO_CPU) executes the existing alloy startup path: clocks come up via
descriptor data, system tick is configured, board init runs. Once startup finishes the
runtime brings up APP_CPU through a documented, descriptor-fed sequence. The data --
APP_CPU entry vector, sync register addresses, cache flush requirements -- comes from
`alloy-devices`; the algorithm lives in alloy.

- Rationale: matches the project's runtime/device boundary. We do not embed APP_CPU
  bring-up sequences in board headers, and we do not require a vendor framework to
  start the second core.
- Trade-off accepted: depends on `alloy-devices` publishing the LX6-specific dual-core
  start data. If those facts are not yet in the descriptor repo, this change blocks
  until they land. Alloy must not synthesize them.

### Decision: Public "current core" surface is a minimal enum
Application code that wants to pin work to a core uses a tiny vendor-neutral API:
```cpp
enum class Core { primary, secondary };
Core alloy::runtime::current_core();
void alloy::runtime::launch_on(Core, void(*)());
```
Implementation lives in arch-specific code; the public header does not expose
`PRO_CPU` / `APP_CPU` names. This keeps single-core targets free to leave `Core` as a
single-element enum or to ignore the API entirely.

- Rationale: the moment we expose `PRO_CPU` in a public header, every future multi-core
  vendor has to learn ESP terminology. A boring `primary/secondary` enum scales.
- Trade-off accepted: applications that need more than two cores will need to revisit
  the API. There is no two-core-only target on alloy's roadmap that justifies a
  different shape today.

### Decision: WROVER-KIT debug path uses the onboard FT2232HL via OpenOCD
Both boards' OpenOCD config files (`board/esp32-wrover-kit-3.3v.cfg` for WROVER-KIT;
none ships for DevKitC v4 because the latter does not have an onboard probe) come from
upstream OpenOCD's Espressif fork.

- Trade-off: DevKitC v4 users must use an external probe (a second WROVER-KIT works,
  for example). That is documented; we do not ship a fake config.

## Risks / Trade-offs

- **Bootloader on bare metal (the real risk).** The original ESP32 ROM loads a
  second-stage bootloader from flash; that bootloader is normally Espressif's
  IDF-blessed binary. Building "alloy without IDF" on this MCU therefore means either:
  - **(a)** writing a minimal alloy second-stage loader that reads the alloy app image
    and jumps to it, or
  - **(b)** treating Espressif's published bootloader as a vendored binary blob and
    flashing it alongside the alloy image (no IDF source dependency, but a binary
    dependency).

  Both are viable. **(a)** preserves the no-vendor-blob property; **(b)** is faster to
  ship. Option **(a)** is the preferred default but requires a small spike before this
  change can be implemented end-to-end. See the Open Questions section.

- **Dual-core synchronisation primitives.** Bringing up the second core safely needs a
  cross-core memory barrier and a way to flush the cache before the second core fetches
  its first instruction. These are arch-specific facts (LX6 has its own cache control
  registers); the descriptor must publish them. If alloy-devices does not yet expose
  these facts for esp32-classic, this change blocks.

- **Coexistence with the no-IDF promise.** Once the boards are on shelves and the user
  wants WiFi, the pull toward `add-esp-idf-integration` will be strong. We accept that
  pressure: the architectural conversation belongs in a separate proposal, and the
  current change does not paint us into a corner.

## Open Questions

- **Bootloader strategy.** Pick option (a) or (b) above before implementation begins.
  The decision changes the task list and the deliverable shape. **Lean: (a) -- write a
  minimal alloy second-stage loader -- with a one-week investigation budget; if that
  budget overruns, fall back to (b) and document the binary dependency in
  `docs/SUPPORT_MATRIX.md`.**
- **APP_CPU bring-up data location.** Are LX6 dual-core start facts already published
  in `alloy-devices`, or does that change need to land first? Decision: ask the
  `alloy-devices` maintainer before scheduling implementation; treat this OpenSpec as
  blocked until confirmed.
- **`launch_on` semantics for non-blocking work.** The first version of
  `alloy::runtime::launch_on` is fire-and-forget; we do not yet expose a join handle.
  Confirm that fire-and-forget is sufficient for the maintainer's testing scope. If a
  return-and-wait pattern is needed earlier, the API shape changes before the first
  implementation lands.
