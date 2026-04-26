# Add ESP Toolchain Pins And ESP32 Catalog Entries

## Why
The ESP32-C3 and ESP32-S3 boards are already wired through the runtime: real
`board.cpp` implementations live under [`boards/esp32c3_devkitm/`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/esp32c3_devkitm/) and
[`boards/esp32s3_devkitc/`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/boards/esp32s3_devkitc/), the board manifest declares
the right vendor/family/arch tuples, the descriptor pipeline accepts the `espressif`
vendor, and the toolchain CMake files even know how to find an existing
`xtensa-esp32s3-elf-gcc` / `riscv32-esp-elf-gcc` install. The only things missing for
"`alloy new --mcu ESP32-S3` works out of the box on a fresh machine" are:

1. an entry in the CLI's board catalog so `alloy new` recognises the boards and the
   MCU strings (`ESP32-C3`, `ESP32-S3`),
2. pinned download URLs and sha256s for Espressif's prebuilt cross-toolchains so
   `alloy toolchain install` can fetch them.

Without those, every ESP32 user hits the same friction wall: install Python, install
ESP-IDF, install the toolchain by hand, then come back to alloy. That defeats the goal
of a one-command bring-up across vendors.

This change keeps strictly to the bare-metal compile path. Auto-fetching the full
ESP-IDF framework (FreeRTOS, WiFi/BLE drivers, partition tables, `idf.py`-driven build)
is a separate, larger architectural decision that will be proposed as
`add-esp-idf-integration` once the toolchain piece lands and we have data on user demand.

## What Changes
- Extend [`tools/alloy-cli/src/alloy_cli/_boards.toml`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tools/alloy-cli/src/alloy_cli/_boards.toml) with
  `esp32c3_devkitm` and `esp32s3_devkitc` entries (vendor, family, device, arch, MCU,
  toolchain). Match the existing `cmake/board_manifest.cmake` declarations exactly so
  `--board <name>` and `--mcu ESP32-C3 / ESP32-S3` both resolve correctly.
- Extend [`tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml`](/Users/lgili/Documents/01%20-%20Codes/01%20-%20Github/alloy/tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml) with
  `xtensa-esp32s3-elf-gcc` and `riscv32-esp-elf-gcc` entries pointing at Espressif's
  published `dl.espressif.com` archive URLs. Sha256 fields ship as `TODO` until the
  release engineer validates them, matching the policy already used for the existing
  ARM and OpenOCD pins.
- Add a `_toolchain_for_arch` mapping in `scaffold.py` so `arch=xtensa` resolves to
  `xtensa-esp32s3-elf-gcc` and `arch=riscv32` resolves to `riscv32-esp-elf-gcc`.
- Update `docs/SUPPORT_MATRIX.md` to declare the two ESP32 boards at a `compile-only`
  tier with the evidence they actually have (board files, descriptors, CMake presets,
  build CI). They explicitly do not claim hardware validation.
- Update `docs/CLI.md` with the two new toolchain names so `alloy toolchain install`
  documentation is complete.
- Add a focused unit test that exercises the ESP path end-to-end against a synthetic
  pin file and a synthetic descriptor, mirroring the existing `--mcu` raw-MCU test.

## Impact
- Affected specs:
  - `runtime-tooling` (extended with the ESP toolchain coverage requirement)
- Affected code:
  - `tools/alloy-cli/src/alloy_cli/_boards.toml`
  - `tools/alloy-cli/src/alloy_cli/_toolchain_pins.toml`
  - `tools/alloy-cli/src/alloy_cli/scaffold.py` (small toolchain mapping change)
  - `tools/alloy-cli/tests/test_scaffold.py` (new ESP scenarios)
  - `docs/SUPPORT_MATRIX.md`, `docs/CLI.md`
- Out of scope for this change:
  - fetching ESP-IDF (the framework, not the toolchain) -- separate proposal
  - layering FreeRTOS, WiFi, or BLE under the alloy public API -- separate proposal
  - hardware-validation gates for ESP32 -- a follow-up once the bring-up runbook exists
  - Windows toolchain pins -- Windows is best-effort across the CLI for now
