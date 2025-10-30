## ADDED Requirements

### Requirement: Directory Structure

The project SHALL have a standardized directory structure that separates core utilities, hardware abstraction layers, drivers, platform-specific code, examples, and tests.

#### Scenario: Core directory exists
- **WHEN** the project is initialized
- **THEN** `src/core/` directory SHALL exist for fundamental types and utilities

#### Scenario: HAL directories exist
- **WHEN** the project is initialized
- **THEN** `src/hal/interface/` SHALL exist for platform-agnostic interfaces
- **AND** `src/hal/host/` SHALL exist for host mock implementations
- **AND** `src/hal/rp2040/` SHALL exist (empty initially) for RP2040 implementations
- **AND** `src/hal/stm32f4/` SHALL exist (empty initially) for STM32F4 implementations

#### Scenario: Support directories exist
- **WHEN** the project is initialized
- **THEN** `src/drivers/` SHALL exist for external peripheral drivers
- **AND** `src/platform/` SHALL exist for generated code
- **AND** `cmake/toolchains/` SHALL exist for toolchain files
- **AND** `cmake/boards/` SHALL exist for board definitions
- **AND** `examples/` SHALL exist for example projects
- **AND** `tests/unit/` and `tests/integration/` SHALL exist for tests
- **AND** `tools/codegen/` SHALL exist for code generation tools

### Requirement: CMake Configuration

The project SHALL use CMake 3.25+ with C++20 standard (without modules) for all builds.

#### Scenario: Root CMakeLists.txt exists
- **WHEN** CMake is configured
- **THEN** root `CMakeLists.txt` SHALL define the project with C++20 requirements
- **AND** it SHALL NOT enable C++ modules (use traditional headers only)

#### Scenario: Host toolchain available
- **WHEN** building for host platform
- **THEN** `cmake/toolchains/host.cmake` SHALL configure the native compiler
- **AND** it SHALL set C++20 standard flags

#### Scenario: ARM toolchain available
- **WHEN** building for ARM targets
- **THEN** `cmake/toolchains/arm-none-eabi.cmake` SHALL configure arm-none-eabi-gcc 11+
- **AND** it SHALL set appropriate CPU flags (e.g., -mcpu=cortex-m0plus)

#### Scenario: Compiler options configured
- **WHEN** CMake is configured
- **THEN** `cmake/compiler_options.cmake` SHALL define common flags
- **AND** it SHALL enable warnings (-Wall -Wextra -Wpedantic)
- **AND** it SHALL enable optimizations for Release builds

### Requirement: Naming Conventions

The project SHALL follow consistent naming conventions for files, directories, and CMake variables.

#### Scenario: File names use snake_case
- **WHEN** creating C++ source files
- **THEN** headers SHALL use `snake_case.hpp` format
- **AND** sources SHALL use `snake_case.cpp` format

#### Scenario: CMake variables use ALLOY_ prefix
- **WHEN** defining CMake variables
- **THEN** public variables SHALL use `ALLOY_` prefix
- **AND** they SHALL use `UPPER_SNAKE_CASE` format
- **AND** functions SHALL use `alloy_` prefix with `snake_case`

#### Scenario: Directory names are lowercase
- **WHEN** creating directories
- **THEN** all directory names SHALL use lowercase with hyphens or underscores
- **AND** they SHALL be descriptive and concise

### Requirement: Version Control Configuration

The project SHALL have proper .gitignore configuration to exclude build artifacts and temporary files.

#### Scenario: Build artifacts ignored
- **WHEN** git status is checked
- **THEN** `build/` directory SHALL be ignored
- **AND** `*.bak` files SHALL be ignored
- **AND** `external/` directory (for dependencies) SHALL be ignored

#### Scenario: IDE files ignored
- **WHEN** developers use different IDEs
- **THEN** `.vscode/` SHALL be ignored (users can maintain locally)
- **AND** `.idea/` SHALL be ignored
- **AND** `*.swp` and `*~` files SHALL be ignored

### Requirement: Documentation Files

The project SHALL include essential documentation files at the root level.

#### Scenario: License file exists
- **WHEN** the project is initialized
- **THEN** `LICENSE` file SHALL exist with MIT license text

#### Scenario: Contributing guidelines exist
- **WHEN** new contributors want to help
- **THEN** `CONTRIBUTING.md` SHALL exist with development guidelines
- **AND** it SHALL reference naming conventions and ADRs

#### Scenario: Architecture documentation accessible
- **WHEN** developers need technical details
- **THEN** `architecture.md`, `plan.md`, and `decisions.md` SHALL be present
- **AND** they SHALL be referenced from README.md
