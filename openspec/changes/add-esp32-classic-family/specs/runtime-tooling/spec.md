# runtime-tooling Spec Delta: ESP32 Classic Toolchain And Catalog

## ADDED Requirements

### Requirement: Runtime Tooling SHALL Cover The ESP32 Classic Cross-Toolchain

The runtime tooling layer SHALL ship a pinned download URL and checksum metadata for
the Espressif `xtensa-esp32-elf-gcc` toolchain. Users SHALL be able to install it
through `alloy toolchain install xtensa-esp32-elf-gcc` without consulting Espressif's
installer or `idf.py`.

#### Scenario: User installs the ESP32 classic cross-toolchain
- **WHEN** a user runs `alloy toolchain install xtensa-esp32-elf-gcc` on a supported
  host
- **THEN** the tooling layer downloads the pinned archive into the per-version cache,
  verifies it against the recorded checksum, and exposes the bin directory through
  `alloy toolchain which`

### Requirement: ESP32 Classic Boards SHALL Be Catalogued For Scaffolding

The CLI's board catalog SHALL include `esp_wrover_kit` and `esp32_devkitc` with the
same vendor/family/device/arch/MCU values declared in `cmake/board_manifest.cmake`,
so that `alloy new --board <name>` and `alloy new --mcu ESP32-WROVER-B` /
`alloy new --mcu ESP32-WROOM-32` resolve through the catalog rather than the
descriptor walk.

#### Scenario: Scaffolding an ESP-WROVER-KIT project
- **WHEN** a user runs `alloy new ./proj --board esp_wrover_kit`
- **THEN** the scaffolder copies the in-tree `boards/esp_wrover_kit/` files into
  `<project>/board/` and emits a `CMakeLists.txt` that declares the
  `espressif/esp32/esp32` device tuple under `ALLOY_BOARD=custom`
- **AND** the preflight output names `xtensa-esp32-elf-gcc` as the required toolchain

#### Scenario: ESP32 classic boards do not silently change tier
- **WHEN** the catalog gains the ESP32-classic entries
- **THEN** the support matrix records both at a `compile-only` tier and explicitly
  does not claim hardware validation
- **AND** any future tier change for these boards must update both the catalog and
  the support matrix in the same change

## MODIFIED Requirements

### Requirement: Runtime Tooling Shall Manage Pinned Cross-Toolchains

The runtime tooling layer SHALL provide commands to install, list, and select pinned
cross-toolchains required by supported boards, and SHALL surface those toolchain paths
to generated CMake presets without requiring the user to edit toolchain files by hand.
The set of pinned toolchains SHALL include at least one cross-toolchain for every
in-tree foundational board's architecture, including the LX6 (ESP32-classic), LX7
(ESP32-S3), and RISC-V (ESP32-C3) Espressif variants.

#### Scenario: User installs the ARM toolchain through the tooling layer
- **WHEN** a user installs the pinned ARM cross-toolchain through the tooling layer
- **THEN** the toolchain is placed in a versioned cache and is verified against a
  published checksum before use
- **AND** generated project presets reference that toolchain path automatically

#### Scenario: Toolchain coverage spans every supported board's architecture
- **WHEN** the in-tree manifest declares an architecture
- **THEN** the toolchain pin file ships at least one toolchain that produces code for
  that architecture
- **AND** the catalog's `toolchain` field on every board points at one of those pins

#### Scenario: Doctor offers to fix missing toolchains
- **WHEN** the diagnostic command detects a missing or mismatched toolchain
- **THEN** it reports which toolchain is missing and offers to install the pinned
  version after explicit user consent
- **AND** it never silently mutates the user's environment without consent
