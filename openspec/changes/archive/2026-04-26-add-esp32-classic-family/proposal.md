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

### Dual-core startup (board-level scope only — runtime surface is follow-up)
- Bootloader strategy: direct-boot, no second-stage loader for blink-class apps.
  Boards apply this directly via `boards/<name>/startup.{S,cpp}` (no Espressif vendor
  blob, manual WDT disable + clock + IO_MUX). PSRAM and second-stage are explicitly
  out of scope.
- APP_CPU bring-up at the **board level** ships via the `add-smp-multicore` archive:
  `boards/esp32_devkit/board.cpp` exposes `board::start_app_cpu(void(*)())` with the
  DPORT.APPCPU_CTRL_* sequence as board-private constants. Validated by
  `examples/esp32_dual_core` (CrossCoreChannel between PRO_CPU and APP_CPU).
- The vendor-neutral runtime surface (`alloy::runtime::Core`,
  `alloy::runtime::current_core`, `alloy::runtime::launch_on`) and the
  descriptor-driven version of the bring-up sequence are **deferred** (see Out of
  Scope below). Shipping them today without descriptor-published facts would either
  hardcode vendor knowledge in alloy or deliver a stub that misleads users.

### Boards
- `boards/esp_wrover_kit/`: WROVER-KIT v4.1, ESP32-WROVER-B module. RGB LED on
  GPIO0/2/4, JTAG via onboard FT2232HL, microSD on SPI2, ILI9341 LCD on SPI3, debug
  UART through the FT2232HL B channel. PSRAM is **out of scope** at v1 (documented).
- `boards/esp32_devkit/`: ESP32-DevKitC v4, ESP-WROOM-32 module. Single user LED on
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
  - `board-bringup` (extended: ESP32 classic boards declared with the same manifest
    shape; secondary-core launch primitive `board::start_app_cpu` exposed opt-in)
  - `runtime-tooling` (NOT touched here — `xtensa-esp-elf-gcc` toolchain pin and
    ESP32 catalog requirements were already absorbed by the sibling
    `add-esp-toolchain-pins` archive)
  - `startup-runtime` (NOT touched — descriptor-driven dual-core bring-up +
    `alloy::runtime::Core` typed surface deferred to follow-up; see Out of Scope)

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

- Out of scope (deferred to dedicated follow-up changes):
  - **Vendor-neutral runtime core surface.** `alloy::runtime::Core`,
    `alloy::runtime::current_core`, `alloy::runtime::launch_on`, and the descriptor-
    driven version of the APP_CPU bring-up sequence. Tracked under the future change
    `add-runtime-multicore-surface` (alloy repo), which depends on the alloy-codegen
    change `expose-xtensa-dual-core-facts` (drafted) being implemented first to
    publish the typed `AppCpuControlPlane` data the runtime would consume.
  - **`signal_cts` / `signal_rts` descriptor enumerator gap.** `src/device/dev.hpp`
    references these unconditionally; the same gap exists on `esp32c3` and `esp32s3`
    so it is not LX6-specific. Resolves uniformly via either a runtime-side
    conditional or a descriptor-side addition; either way is a separate change.
  - **Hardware validation runbook.** Promotion from `compile-only` to
    `representative` (and `foundational` later) lands in follow-ups when boards are
    in the maintainer's hands.
  - ESP-IDF framework integration (FreeRTOS, WiFi/BLE, NVS, partition tables,
    `idf.py` builds). Tracked under a future `add-esp-idf-integration` proposal —
    actively scoped via the WiFi/BLE OpenSpec tree under discussion.
  - PSRAM bring-up on WROVER-KIT; Camera and SD-card driver bring-up on WROVER-KIT.
