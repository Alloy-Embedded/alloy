# runtime-tooling Spec Delta: ESP Toolchain Pins And ESP32 Catalog Coverage

## ADDED Requirements

### Requirement: Runtime Tooling Shall Cover Foundational ESP32 Cross-Toolchains

The runtime tooling layer SHALL ship pinned download URLs and checksum metadata for the
Espressif cross-toolchains required to build the ESP32-C3 (RISC-V) and ESP32-S3
(Xtensa) boards declared in the board manifest. Users SHALL be able to install these
toolchains through the same `alloy toolchain install` flow used for ARM toolchains,
without consulting Espressif's installer or `idf.py`.

#### Scenario: User installs the ESP32-S3 cross-toolchain
- **WHEN** a user runs `alloy toolchain install xtensa-esp32s3-elf-gcc` on a supported
  host
- **THEN** the tooling layer downloads the pinned Espressif archive into the
  per-version cache, verifies it against the recorded checksum, and exposes the bin
  directory through `alloy toolchain which`
- **AND** the user does not need to run any Espressif installer or set
  `IDF_TOOLS_PATH` by hand

#### Scenario: User installs the ESP32-C3 cross-toolchain
- **WHEN** a user runs `alloy toolchain install riscv32-esp-elf-gcc` on a supported host
- **THEN** the tooling layer installs the pinned RISC-V toolchain in the same cache
  layout used for other toolchains
- **AND** subsequent `alloy new --mcu ESP32-C3` scaffolds reference that toolchain in
  the generated `CMakePresets.json`

### Requirement: ESP32 Boards SHALL Be Catalogued For Scaffolding

The CLI's board catalog SHALL include the foundational ESP32 boards declared in
`cmake/board_manifest.cmake`, with the same vendor/family/device/arch/MCU values, so
that `alloy new --board <name>` and `alloy new --mcu <part>` resolve to those boards
through the catalog rather than falling through to the descriptor walk.

#### Scenario: Scaffolding an ESP32-S3 project from the catalog
- **WHEN** a user runs `alloy new ./proj --board esp32s3_devkitc` or
  `alloy new ./proj --mcu ESP32-S3`
- **THEN** the scaffolder copies the in-tree `boards/esp32s3_devkitc/` files into
  `<project>/board/` and emits a `CMakeLists.txt` that declares the
  `espressif/esp32s3/esp32s3` device tuple under `ALLOY_BOARD=custom`
- **AND** the preflight output names `xtensa-esp32s3-elf-gcc` as the required
  toolchain

#### Scenario: ESP32 boards do not silently change tier
- **WHEN** the catalog gains the ESP32 entries
- **THEN** the support matrix records them at a `compile-only` tier with the evidence
  they actually have (board files, descriptor coverage, build CI), and explicitly
  does not claim hardware validation
- **AND** any future tier change for these boards must update both the catalog and
  the support matrix in the same change

### Requirement: ESP Toolchain Coverage SHALL Not Imply ESP-IDF Framework Integration

Shipping pinned ESP toolchains SHALL NOT, by itself, integrate the ESP-IDF framework
(FreeRTOS, WiFi, BLE, NVS, partition tables, `idf.py`-driven build) into the runtime
or the scaffolded project shape. Projects scaffolded for ESP32 SHALL continue to
build through the same descriptor-driven CMake path used for other vendors, until a
separate proposal introduces an explicit IDF integration mode.

#### Scenario: Scaffolded ESP32 project does not depend on ESP-IDF
- **WHEN** a user scaffolds an ESP32 project with the ESP toolchain installed
- **THEN** the generated build does not require `idf.py`, `IDF_PATH`, or any
  ESP-IDF component
- **AND** the project consumes Alloy through `add_subdirectory(${ALLOY_ROOT})` with
  `ALLOY_BOARD=custom`, identical in shape to projects for ARM boards
