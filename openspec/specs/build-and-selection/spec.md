# build-and-selection Specification

## Purpose
Build and selection define the supported way a user or CI job chooses a board, pulls the matching
descriptor-backed device contract, and gets a reproducible build without reverse-engineering
family-local build logic.

The build path is board-oriented and descriptor-driven: selecting a supported board resolves the
architecture, startup artifacts, generated runtime contract, and validation presets that belong to
that target.
## Requirements
### Requirement: Foundational Build Paths Shall Compile Against Runtime-Lite

Foundational board and smoke builds SHALL prove that normal runtime code compiles against the
runtime-lite generated contract without requiring reflection-table headers in the hot path.

#### Scenario: Foundational smoke build excludes reflection dependency

- **WHEN** foundational compile coverage is executed
- **THEN** the selected runtime path is satisfied by runtime-lite generated headers
- **AND** reflection-only headers remain optional or test-only

### Requirement: Build Selection Shall Be Device-Descriptor Driven

The build system SHALL resolve board, vendor, family, device, and architecture through a compact
selection mechanism that imports `alloy-devices`.

#### Scenario: Selecting a board for build
- **WHEN** the user configures a build for a supported board
- **THEN** CMake resolves the selected device descriptors and architecture artifacts automatically
- **AND** the top-level build does not require a large handwritten family switch to select startup
  and device code

### Requirement: Foundational Builds Shall Compile Against Published Descriptor Artifacts

Foundational targets SHALL build against published device descriptors rather than generated code
checked into the runtime repo.

#### Scenario: Foundational family compile
- **WHEN** a foundational board is compiled
- **THEN** its runtime build uses `alloy-devices` artifacts for device facts
- **AND** not legacy generated headers stored inside `alloy`

### Requirement: Build Selection Docs Shall Match Supported Runtime Entry Points

Build-and-selection documentation and presets SHALL describe only supported board-oriented runtime
entry points.

#### Scenario: User follows a documented preset flow
- **WHEN** a user follows the documented configure/build flow for a supported board
- **THEN** the named preset or target exists and succeeds on the claimed validation path

### Requirement: Validation Profiles Shall Be First-Class Build Selections

The build system SHALL expose named validation profiles for the runtime validation layers.

#### Scenario: Selecting host MMIO validation

- **WHEN** a developer or CI job selects host MMIO validation
- **THEN** the build resolves the required test targets and labels without ad hoc local scripts
- **AND** the selected tests are distinguishable from compile-only, emulation, hardware, and
  assembly validation

#### Scenario: Selecting the first Renode target

- **WHEN** a developer or CI job selects the first `Renode` validation profile for `same70`
- **THEN** the build resolves the selected image and emulation assets needed for that scenario
- **AND** the selection mechanism stays compatible with descriptor-driven device selection

#### Scenario: Selecting the representative runtime-validation presets

- **WHEN** a developer or CI job selects the representative host MMIO or `Renode` validation
  presets for the documented foundational boards
- **THEN** the build resolves the required board-specific targets, toolchains, and staged assets
- **AND** those presets remain stable enough to be consumed directly by GitHub Actions workflows

### Requirement: alloy SHALL load AlloyHelpers automatically when consumed via add_subdirectory or FetchContent

The top-level `CMakeLists.txt` SHALL `include()` the rendered
`AlloyHelpers.cmake` immediately after configuring it, so
downstream projects that consume alloy via
`FetchContent_Declare(alloy ...)` + `FetchContent_MakeAvailable`
(which dispatches to `add_subdirectory`) get the
`alloy_add_runtime_executable(...)` helper without additional
ceremony.  The helper SHALL operate against the existing
`Alloy::hal` (canonical) and `alloy::hal` (lowercase alias)
targets.

#### Scenario: a downstream FetchContent consumer calls alloy_add_runtime_executable

- **WHEN** a downstream project mounts alloy via
  `FetchContent_Declare(alloy ... )` +
  `FetchContent_MakeAvailable(alloy)` and the configure pass
  reaches `alloy_add_runtime_executable(my_app SOURCES
  main.cpp)`
- **THEN** the configure pass SHALL succeed
- **AND** the resulting `my_app` target SHALL link against
  `Alloy::hal`
- **AND** the helper invocation SHALL NOT require an extra
  `include(${alloy_BINARY_DIR}/generated/cmake/AlloyHelpers.cmake)`
  call from the consumer

### Requirement: alloy SHALL ship a usable install-tree CMake package

`cmake --install <build>` SHALL deposit `AlloyConfig.cmake`,
`AlloyConfigVersion.cmake`, `AlloyHelpers.cmake`, and the
namespaced `AlloyTargets.cmake` under
`<prefix>/lib/cmake/Alloy/`, plus public headers under
`<prefix>/include/alloy/`.  External consumers calling
`find_package(alloy CONFIG)` from a system prefix SHALL get a
working `Alloy::hal` target + helper.

#### Scenario: an installed prefix exposes Alloy::hal via find_package

- **WHEN** a maintainer runs `cmake --install <build>
  --prefix /opt/alloy`
- **AND** a downstream project calls `find_package(alloy
  CONFIG REQUIRED PATHS /opt/alloy)`
- **THEN** the target `Alloy::hal` SHALL exist
- **AND** `alloy_add_runtime_executable(...)` SHALL be
  available in the caller's scope

