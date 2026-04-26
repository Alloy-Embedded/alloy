# board-bringup Spec Delta: ESP32 Classic Boards

## ADDED Requirements

### Requirement: ESP32 Classic Boards SHALL Follow The Same Declarative Contract As Other Boards

The `esp_wrover_kit` and `esp32_devkit` boards SHALL be declared with the same
manifest shape (vendor, family, device, arch, MCU, flash size, board header path,
optional linker script) used by every other in-tree board. The arch MUST be
`xtensa-lx6` (distinct from `xtensa-lx7` used by ESP32-S2/S3) so consumers can
branch on the variant without parsing MCU strings.

#### Scenario: ESP32 classic board declared in manifest

- **WHEN** the board manifest is queried for `esp_wrover_kit` or `esp32_devkit`
- **THEN** the manifest returns vendor `espressif`, family `esp32`, device `esp32`
  (or `esp32-wroom32` for the WROVER variant), arch `xtensa-lx6`, and the board
  header path under `boards/<name>/board.hpp`
- **AND** the entry follows the declarative shape; no custom CMake logic is required

#### Scenario: ESP32 classic boards do not silently change tier

- **WHEN** the catalog gains the ESP32-classic entries
- **THEN** `docs/SUPPORT_MATRIX.md` records both at a `compile-only` tier and
  explicitly does not claim hardware validation
- **AND** any future tier change for these boards must update both the catalog
  and the support matrix in the same change

### Requirement: ESP32 Classic Boards SHALL Expose The Secondary-Core Launch Primitive Opt-In

ESP32 classic boards SHALL expose `board::start_app_cpu(void(*fn)())` as the
opt-in primitive for releasing APP_CPU. The primitive SHALL NOT be invoked
automatically from `Reset_Handler`; applications that ignore it boot normally
on PRO_CPU. The data-driven version of this primitive (sourced from descriptor
`AppCpuControlPlane` typed register ids) is a follow-up tracked by the
alloy-codegen change `expose-xtensa-dual-core-facts`.

#### Scenario: Secondary-core launch primitive is declared per-board

- **WHEN** an application targets `boards/esp32_devkit/` or
  `boards/esp_wrover_kit/`
- **THEN** the board declares `void board::start_app_cpu(void (*fn)())`
- **AND** the primitive's body is currently sourced from board-private register
  constants (DPORT.APPCPU_CTRL_*); migrating those constants behind a typed
  descriptor surface is the explicit out-of-scope follow-up
