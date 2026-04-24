## ADDED Requirements

### Requirement: Driver Seed Library Shall Live Under A Dedicated Top-Level Directory

The repo SHALL host the seed driver library under a dedicated `drivers/` top-level directory
that is separate from the HAL, the board helper layer, and the examples tree.

#### Scenario: Contributor adds a new driver

- **WHEN** a contributor adds a new device driver
- **THEN** the driver lives under `drivers/<class>/<device>/` (e.g. `drivers/display/ssd1306/`)
- **AND** the driver does not modify files under `src/hal/**`, `src/device/**`,
  `boards/**`, or `examples/**`

### Requirement: Seed Drivers Shall Be Header-Only Templated Over The Public HAL Handle

Each seed driver SHALL be header-only and parameterised over the public HAL handle type
returned by the board helper layer (e.g. `board::make_i2c()`, `board::make_spi()`), so the
driver compiles against any board that exposes the relevant bus through the documented public
HAL surface.

#### Scenario: Driver is instantiated against a supported board

- **WHEN** an application instantiates a seed driver with a handle produced by a supported
  board's public HAL helper
- **THEN** the driver compiles without modification to the HAL or the board helper
- **AND** the driver calls only the documented public HAL methods on the handle (e.g.
  `read`, `write`, `write_read` for I2C; `transfer`, `write`, `read` for SPI)

### Requirement: Seed Drivers Shall Return `core::Result` Without Dynamic Allocation

Every fallible operation on a seed driver SHALL return `core::Result<T, core::ErrorCode>`
and SHALL NOT rely on dynamic memory allocation, exceptions, or static mutable state that
outlives a single driver instance.

#### Scenario: Application invokes a seed-driver operation

- **WHEN** an application invokes any fallible driver operation
- **THEN** the operation returns `core::Result<T, core::ErrorCode>`
- **AND** the driver performs no heap allocation, throws no exceptions, and does not rely on
  process-global mutable state

### Requirement: Seed Library Shall Ship Three Canonical Drivers

The seed library SHALL ship canonical drivers for three device classes that together
exercise the I2C and SPI public HAL surfaces: an SSD1306-class OLED display, a BME280-class
environmental sensor, and a W25Q-class NOR flash.

#### Scenario: Adopter browses the seed library

- **WHEN** a new adopter browses `drivers/`
- **THEN** there is at least one working driver for an SSD1306 OLED display over I2C
- **AND** there is at least one working driver for a BME280 temperature/pressure/humidity
  sensor over I2C with compensation math
- **AND** there is at least one working driver for a W25Q NOR flash device over SPI with
  JEDEC-ID check, page read/program, and sector erase

### Requirement: Seed Drivers Shall Be Covered By Compile Tests

Each seed driver SHALL be covered by at least one compile test that instantiates the driver
against the public HAL handle surface, so API drift breaks the build instead of silently
regressing.

#### Scenario: HAL surface drifts

- **WHEN** the public HAL handle surface changes in a way the seed drivers depend on
- **THEN** the driver compile tests fail to build
- **AND** the failure surfaces in the descriptor-contract-smoke build gate
