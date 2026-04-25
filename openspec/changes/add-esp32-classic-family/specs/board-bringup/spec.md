# board-bringup Spec Delta: ESP32 Classic Boards And Dual-Core Bring-Up

## ADDED Requirements

### Requirement: Boards SHALL Support Dual-Core Bring-Up Through The Runtime

When a board declares an MCU with more than one core, `board::init()` SHALL bring up
every core the descriptor declares, using the descriptor-driven runtime path. Boards
SHALL NOT contain handwritten cross-core start sequences in board headers.

#### Scenario: ESP32 classic dual-core bring-up
- **WHEN** an ESP32 classic board (`esp_wrover_kit` or `esp32_devkitc`) initialises
- **THEN** the runtime brings up the secondary Xtensa core after the primary core
  finishes startup, using sync facts published by `alloy-devices`
- **AND** the board header does not embed APP_CPU bring-up registers or sequences

### Requirement: ESP32 Classic Boards SHALL Follow The Same Declarative Contract As Other Boards

The `esp_wrover_kit` and `esp32_devkitc` boards SHALL be declared with the same
manifest shape (vendor, family, device, arch, MCU, flash size, board header path,
optional linker script) used by every other in-tree board.

#### Scenario: ESP32 classic board declared in manifest
- **WHEN** the board manifest is queried for `esp_wrover_kit` or `esp32_devkitc`
- **THEN** the manifest returns vendor `espressif`, family `esp32`, device `esp32`,
  arch `xtensa-lx6`, and the board header path under `boards/<name>/board.hpp`
- **AND** the entry follows the declarative shape; no custom CMake logic is required

## MODIFIED Requirements

### Requirement: Boards Shall Be Declarative

Boards SHALL declare local hardware choices and bring-up policy on top of shared runtime
primitives. Multi-core targets MAY add a `core` selection field to local hardware
choices (e.g. "this peripheral is initialised on the primary core") through the public
runtime surface; they SHALL NOT introduce a parallel cross-core init path inside the
board header.

#### Scenario: Board LED declaration
- **WHEN** a board exposes an LED resource
- **THEN** it declares the selected pin and polarity
- **AND** it does not embed raw register initialization logic in the board header

#### Scenario: Multi-core LED declaration
- **WHEN** a board on a multi-core MCU declares an LED resource
- **THEN** the declaration optionally specifies the core that owns the toggle, using
  the public `alloy::runtime::Core` enum
- **AND** the runtime executes that ownership through the public `launch_on` surface
  rather than through board-private cross-core glue
