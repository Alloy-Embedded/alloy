# runtime-tooling Specification

## Purpose
Runtime tooling defines the stable user-facing product layer for board-oriented configure, build,
flash, monitor, validation, diagnostics, and downstream consumption.

The goal is that users can discover what is supported, how to run it, and why a configuration
fails without reading internal CMake files or runtime implementation details first.
## Requirements
### Requirement: Runtime Tooling Shall Expose Stable Board-Oriented Entry Points

The repo SHALL expose stable and documented board-oriented entry points for configure, build,
validation, and supported flash/debug flows.

#### Scenario: User builds a foundational board
- **WHEN** a user selects a foundational board
- **THEN** the repo documents and exposes one supported path to configure and build it
- **AND** the user does not need to reverse-engineer internal CMake files first

### Requirement: Runtime Tooling Shall Surface Actionable Configuration Diagnostics

The user-facing tooling layer SHALL surface actionable diagnostics for invalid connector, clock,
or ownership configurations.

#### Scenario: Invalid connector choice
- **WHEN** a user selects an invalid connector or conflicting resource combination
- **THEN** the reported error explains the conflict in runtime terms
- **AND** it points toward valid alternatives when the generated contract provides them

### Requirement: Runtime Tooling Shall Support Explainable Diagnostics

When the generated contract publishes enough data, the runtime tooling layer SHALL be able to
explain why a connector, clock, or capability fact exists and how it differs across targets.

#### Scenario: User asks why a pin choice failed
- **WHEN** a user inspects a failed or unavailable connector/resource choice
- **THEN** the tooling can explain the relevant fact in runtime terms
- **AND** it may surface provenance or target-to-target differences when those inputs are available

### Requirement: Runtime Tooling Shall Support Downstream CMake Consumption

If `alloy` claims package-style downstream consumption, the runtime tooling layer SHALL provide a
documented and validated CMake integration path.

#### Scenario: Downstream application consumes Alloy through CMake package integration
- **WHEN** a downstream CMake project follows the documented package-style integration path
- **THEN** it can configure and link against `alloy` without reverse-engineering internal targets
- **AND** the documented integration path is covered by validation

### Requirement: Official Examples Shall Cover The Public Runtime Surface

The official example set SHALL cover the primary public runtime surface.

#### Scenario: User looks for a CAN or RTC example
- **WHEN** a primary public HAL class is documented as supported
- **THEN** the repo provides at least one official example or explicit cookbook path for that class

### Requirement: Tooling Diagnostics Shall Prefer Ergonomic API Spellings

User-facing diagnostics and explain/diff tooling SHALL prefer the ergonomic public API spellings
when presenting connector, route, or capability information.

#### Scenario: User inspects a failed UART route choice

- **WHEN** a user asks the tooling to explain a failed or invalid route/configuration choice
- **THEN** the first rendered spelling uses the ergonomic public names such as `dev::USART2`,
  `dev::PA2`, and `tx<dev::PA2>`
- **AND** canonical generated names may still appear as secondary detail for expert debugging

### Requirement: Runtime Tooling Shall Export An IDE-Ready Compile Commands Path

The runtime tooling layer SHALL provide a single entry point that makes `compile_commands.json`
from a configured build directory discoverable by a clangd/LSP-backed IDE without manual copying.

#### Scenario: User enables IDE LSP after configuring a board

- **WHEN** a user has configured any supported board build directory and runs the tooling
  entry point for IDE-ready compile commands
- **THEN** a `compile_commands.json` path is exposed at the repo root (symlink or copy)
- **AND** the exposed path points at the most recently configured supported build directory

### Requirement: Runtime Tooling Shall Emit A Machine-Readable Environment Report

The runtime tooling layer SHALL provide a subcommand that emits a structured, machine-readable
environment report covering alloy version, pinned device-contract ref, board tier membership,
required release gates per board, detected tool versions, and current repo git sha.

#### Scenario: User captures environment state for a bug report or release audit

- **WHEN** a user invokes the environment report entry point
- **THEN** the tool prints a structured document (JSON) capturing the fields above
- **AND** the document is stable enough to diff across runs on the same repo state

### Requirement: Runtime Tooling Shall Provide Preflight Environment Diagnostics

The runtime tooling layer SHALL provide a preflight diagnostics entry point that verifies the
toolchain, probe tooling, python dependencies, and the device-contract ref alignment expected by
the current repo state.

#### Scenario: User runs preflight before first flash

- **WHEN** a user invokes the preflight diagnostics entry point
- **THEN** the tool verifies cmake, the ARM bare-metal toolchain, openocd, python deps, and the
  selected device-contract ref against `docs/RELEASE_MANIFEST.json`
- **AND** any failed check reports a human-actionable hint and causes a non-zero exit

### Requirement: Runtime Tooling Shall Scaffold A Downstream Starter For One Foundational Board

The runtime tooling layer SHALL provide an entry point that scaffolds a minimal downstream
firmware starter tree targeting one chosen foundational board.

#### Scenario: User bootstraps a new firmware project

- **WHEN** a user invokes the scaffold entry point targeting a foundational board
- **THEN** the tool produces a starter tree that configures, builds, and mirrors the blink
  example surface using the public HAL
- **AND** the generated tree points at the documented CMake package consumption path

### Requirement: Public Docs Site Shall Be Built And Publishable From The Repo

The repo SHALL carry a committed, reproducible configuration for a public docs site that renders
the existing user-facing markdown documentation without requiring external tooling outside the
declared toolchain.

#### Scenario: Contributor builds the docs site locally

- **WHEN** a contributor runs the documented docs-site build command
- **THEN** a static site is produced from the committed `mkdocs.yml` configuration and the
  `docs/**.md` sources, with no external fetches beyond the declared python requirements
- **AND** the build fails loudly on broken internal links or missing nav entries

### Requirement: Docs Site Navigation Shall Separate User Guide From Internals

The published docs site SHALL expose a navigation structure that clearly separates the
user-facing guide from internal design notes and release discipline material.

#### Scenario: New adopter browses the docs site

- **WHEN** a first-time visitor lands on the docs site
- **THEN** the navigation surfaces the quickstart, board tooling, cookbook, and support matrix
  as the primary user guide entries
- **AND** internal material (architecture, release discipline, runtime device boundary, runtime
  cleanup audit) is reachable under a clearly labeled secondary section, not on the landing
  surface

### Requirement: Docs Site Shall Be Covered By A Release Gate

The docs site SHALL be covered by a release gate that fails loudly when the site cannot be
built from the current repo state.

#### Scenario: Release discipline validates the docs site

- **WHEN** the release discipline gate set is exercised for a candidate release
- **THEN** the docs-site gate runs the documented build command with strict link checking
- **AND** any broken link, missing nav entry, or build failure fails the gate and the release

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

