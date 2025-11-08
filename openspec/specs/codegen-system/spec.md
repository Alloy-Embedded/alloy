# codegen-system Specification

## Purpose
TBD - created by archiving change add-codegen-multiarch. Update Purpose after archive.
## Requirements
### Requirement: Multi-Architecture Support

The code generator SHALL support multiple MCU architectures with different register layouts and startup code.

#### Scenario: Architecture detection
- **WHEN** generating code for a board
- **THEN** the generator SHALL detect the architecture from the database
- **AND** it SHALL select appropriate templates for that architecture

#### Scenario: Supported architectures
- **WHEN** generating code
- **THEN** the generator SHALL support at least: arm-cortex-m0, arm-cortex-m3, arm-cortex-m4, rl78, xtensa
- **AND** each architecture SHALL have dedicated startup and linker templates

### Requirement: SVD File Support

The code generator SHALL parse ARM SVD files to generate register definitions.

#### Scenario: SVD parsing
- **WHEN** provided with an SVD file path
- **THEN** the generator SHALL extract peripherals, registers, and interrupts
- **AND** it SHALL convert to internal JSON database format

#### Scenario: STM32F103 SVD
- **WHEN** parsing STM32F103xB.svd
- **THEN** it SHALL extract GPIOA-GPIOE definitions
- **AND** it SHALL extract USART1-3 definitions
- **AND** it SHALL generate interrupt vector table

### Requirement: Custom Database Format

The code generator SHALL support custom JSON database for MCUs without SVD files (e.g., RL78, ESP32).

#### Scenario: Manual database creation
- **WHEN** no SVD file exists
- **THEN** developers SHALL be able to create database/families/<family>.json manually
- **AND** it SHALL follow the documented schema

#### Scenario: RL78 database
- **WHEN** loading rl78.json database
- **THEN** it SHALL include port register definitions
- **AND** it SHALL include SAU peripheral definitions
- **AND** it SHALL define RL78 interrupt vectors

### Requirement: Architecture-Specific Templates

The code generator SHALL use architecture-specific templates for startup code and linker scripts.

#### Scenario: ARM Cortex-M startup
- **WHEN** generating for ARM Cortex-M architecture
- **THEN** it SHALL use cortex_m_startup.c.j2 template
- **AND** it SHALL include reset handler with .data/.bss initialization
- **AND** it SHALL include vector table with NVIC interrupts

#### Scenario: RL78 startup
- **WHEN** generating for RL78 architecture
- **THEN** it SHALL use rl78_startup.c.j2 template
- **AND** it SHALL place interrupt vectors at address 0x0000
- **AND** it SHALL initialize RL78-specific registers

#### Scenario: ESP32 startup
- **WHEN** generating for ESP32 architecture
- **THEN** it SHALL use esp32_startup.c.j2 template (or FreeRTOS integration)
- **AND** it SHALL handle ESP32 bootloader integration

### Requirement: Generated Code Quality

Generated code SHALL be readable, navigable, and comply with project naming conventions.

#### Scenario: Generated headers
- **WHEN** code is generated
- **THEN** header files SHALL use snake_case.hpp naming
- **AND** they SHALL include proper include guards
- **AND** they SHALL be in namespace alloy::platform::<family>

#### Scenario: Generated code location
- **WHEN** code is generated
- **THEN** it SHALL be placed in ${CMAKE_BINARY_DIR}/generated/
- **AND** it SHALL be visible in IDE (for navigation and debugging)

