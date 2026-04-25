# Add ESP32 Classic (LX6) Family With Dual-Core Bring-Up

## Why
The two boards the maintainer wants to validate Alloy on -- the **ESP32-WROVER-KIT v4.1**
and the **ESP32-DevKitC v4** -- both use the original ESP32 module (Xtensa LX6,
**dual-core**). Alloy today only supports the newer ESP32-C3 (RISC-V) and ESP32-S3
(Xtensa LX7) variants; the LX6 family does not exist anywhere in the runtime, the build
system, the descriptor pipeline, or the CLI catalog.

Adding these boards is therefore not a "scaffold + done" task. It requires:

- a new MCU family (`esp32`) in the build manifest, platform module, and toolchain file,
- a renamed architecture enum (current `xtensa` is ambiguous between LX6 and LX7) which
  is itself a contract change in `add-custom-board-bringup`,
- a dual-core startup model -- mandatory per the maintainer's requirement; alloy has not
  yet booted a second core on any vendor,
- pinned `xtensa-esp32-elf-gcc` toolchain,
- board files for two real boards, including the WROVER-KIT's onboard JTAG probe and
  rich peripheral surface (LCD, microSD, RGB LED, camera header).

There is also a non-trivial open question -- how to second-stage-boot the original ESP32
without ESP-IDF -- which the design surfaces explicitly so it is decided up front rather
than discovered mid-implementation.

## What Changes

### Build and runtime infrastructure
- Add `cmake/platforms/esp32.cmake`, `cmake/toolchains/xtensa-esp32-elf.cmake`, and the
  `esp32` family branch in `cmake/board_manifest.cmake`.
- Migrate the `arch` enum: rename current `xtensa` to `xtensa-lx7` (used by ESP32-S3) and
  introduce `xtensa-lx6` (used by ESP32-classic). Update every consumer:
  - `_ALLOY_VALID_ARCHES` in `cmake/board_manifest.cmake` (added by
    `add-custom-board-bringup`)
  - the in-tree `esp32s3_devkitc` board manifest entry
  - `tools/alloy-cli/src/alloy_cli/_boards.toml`
  - `tools/alloy-cli/src/alloy_cli/scaffold.py` (`VALID_ARCHES` plus
    `_toolchain_for_arch` mapping)
  - `tools/alloy-cli/tests/custom_board/CMakeLists.txt` if it pins a value
  - `docs/CUSTOM_BOARDS.md`
- Pin `xtensa-esp32-elf-gcc` in `_toolchain_pins.toml` (alongside the entries scoped by
  `add-esp-toolchain-pins`).

### Dual-core startup
- Extend the descriptor-driven startup runtime to bring up the second Xtensa core
  (APP_CPU) after PRO_CPU clock bring-up, in a way that stays inside alloy's
  runtime/device boundary -- startup *algorithms* in the runtime, startup *data* in
  `alloy-devices`.
- Define a small public surface for "what core am I running on" so application code
  can pin work to a core without leaking ESP-specific names.
- Decide and document the second-stage-bootloader strategy (see design.md open
  question). Two viable paths under evaluation: (a) write a minimal alloy second-stage
  loader, (b) reuse Espressif's open-source second-stage bootloader as a vendored
  binary blob. **Option (a) is preferred** to keep the no-IDF promise intact, but it is
  research work that must complete before this change can be implemented.

### Boards
- `boards/esp_wrover_kit/`: WROVER-KIT v4.1, ESP32-WROVER-B module. RGB LED on
  GPIO0/2/4, JTAG via onboard FT2232HL, microSD on SPI2, ILI9341 LCD on SPI3, debug
  UART through the FT2232HL B channel. PSRAM is **out of scope** at v1 (documented).
- `boards/esp32_devkitc/`: ESP32-DevKitC v4, ESP-WROOM-32 module. Single user LED on
  GPIO2, debug UART through the CP210x USB bridge.
- Both boards expose the same descriptor-driven HAL path used by the existing ESP32-S3
  board.

### CLI and docs
- Add the two boards to `_boards.toml` with display name, MCU (`ESP32-WROVER-B`,
  `ESP32-WROOM-32`), toolchain (`xtensa-esp32-elf-gcc`), and OpenOCD config files
  (`board/esp32-wrover-kit-3.3v.cfg` for WROVER-KIT, none for DevKitC v4).
- Update `docs/SUPPORT_MATRIX.md` with the new boards at a `compile-only` tier
  initially, moving to `representative` once hardware spot-checks land.
- Update `docs/BOARD_TOOLING.md` and `docs/CLI.md` with the new toolchain and boards.

## Impact

- Affected specs:
  - `board-bringup` (extended: ESP32 classic boards must follow the same declarative
    contract; dual-core bring-up extends the existing `board::init()` requirement)
  - `startup-runtime` (extended: dual-core start sequence becomes a runtime concern;
    explicit non-leak of vendor names through the public "current core" surface)
  - `runtime-tooling` (extended: new toolchain pin; ESP32 classic boards in catalog;
    again, explicit non-implication of ESP-IDF integration)

- Affected code:
  - `cmake/{platforms,toolchains}/`, `cmake/board_manifest.cmake`
  - `boards/esp_wrover_kit/`, `boards/esp32_devkitc/` (new)
  - `boards/esp32s3_devkitc/` (arch rename only)
  - `src/runtime/startup/` (dual-core sequencing -- exact paths to be confirmed during
    investigation)
  - `tools/alloy-cli/src/alloy_cli/_boards.toml`,
    `tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml`,
    `tools/alloy-cli/src/alloy_cli/scaffold.py`
  - `tools/alloy-cli/tests/custom_board/` (arch rename)
  - `docs/{SUPPORT_MATRIX,BOARD_TOOLING,CLI,CUSTOM_BOARDS}.md`

- Hard dependencies:
  - `alloy-devices` must publish `espressif/esp32/esp32/` runtime descriptors before
    this change can be implemented. If the descriptors do not exist, the proposal
    blocks until they do; alloy must not synthesize device facts.
  - `add-custom-board-bringup` must merge first (or be amended in this change) so the
    arch enum migration has a concrete origin.
  - `add-esp-toolchain-pins` is a sibling change for the C3/S3 toolchain pins; this
    change extends the same `_toolchain_pins.toml` with the LX6 entry.

- Out of scope:
  - ESP-IDF framework integration (FreeRTOS, WiFi/BLE, NVS, partition tables, `idf.py`
    builds). Tracked separately under a future `add-esp-idf-integration` proposal.
  - PSRAM bring-up on WROVER-KIT.
  - Camera and SD-card driver bring-up on WROVER-KIT (the boards expose them; alloy
    drivers come later).
  - Hardware validation gates for either board. v1 lands at a `compile-only` tier;
    `representative` and `foundational` claims arrive in follow-up changes once a
    runbook and CI integration exist.
