# runtime-tooling Spec Delta: Project Scaffolding CLI

## ADDED Requirements

### Requirement: Runtime Tooling Shall Provide A Stable Project Scaffolding Command

The runtime tooling layer SHALL expose a stable, documented command that scaffolds a new
downstream project for a chosen board or MCU target without requiring the user to clone the
runtime, edit CMake by hand, or copy files from in-tree examples.

#### Scenario: User scaffolds a project for a supported board
- **WHEN** a user runs the scaffolding command with a supported board target
- **THEN** the command produces a self-contained project tree containing build, source,
  editor, and ignore configuration sufficient to configure, build, and flash the board
- **AND** the generated project consumes Alloy through the documented downstream CMake
  contract rather than internal targets

#### Scenario: User scaffolds for an unsupported MCU
- **WHEN** the user requests a target whose descriptor is not available in `alloy-devices`
- **THEN** the command fails with an actionable message that names the missing descriptor
  and points at `alloy-devices` as the place to add support
- **AND** no partial or synthesized project tree is left on disk

### Requirement: Runtime Tooling Shall Manage Pinned SDK Versions

The runtime tooling layer SHALL provide commands to install, list, select, and inspect
versioned Alloy runtime and `alloy-devices` checkouts so that scaffolded projects resolve
against an explicit, reproducible source.

#### Scenario: User installs and selects a runtime version
- **WHEN** a user installs a tagged runtime version through the tooling layer
- **THEN** the runtime and matching `alloy-devices` descriptors are placed in a versioned
  cache with pinned commit SHAs
- **AND** subsequent scaffolding commands resolve against the active version until the user
  selects a different one

#### Scenario: User opts into per-project vendoring
- **WHEN** a user scaffolds with the vendored option
- **THEN** the runtime and descriptors are checked out into the project tree
- **AND** a project-local lockfile records the pinned commit SHAs so the project builds
  reproducibly without relying on the shared cache

### Requirement: Runtime Tooling Shall Manage Pinned Cross-Toolchains

The runtime tooling layer SHALL provide commands to install, list, and select pinned
cross-toolchains required by supported boards, and SHALL surface those toolchain paths to
generated CMake presets without requiring the user to edit toolchain files by hand.

#### Scenario: User installs the ARM toolchain through the tooling layer
- **WHEN** a user installs the pinned ARM cross-toolchain through the tooling layer
- **THEN** the toolchain is placed in a versioned cache and is verified against a published
  checksum before use
- **AND** generated project presets reference that toolchain path automatically

#### Scenario: Doctor offers to fix missing toolchains
- **WHEN** the diagnostic command detects a missing or mismatched toolchain
- **THEN** it reports which toolchain is missing and offers to install the pinned version
  after explicit user consent
- **AND** it never silently mutates the user's environment without consent

### Requirement: Scaffolded Projects Shall Include Working Editor Integration

Scaffolded projects SHALL include editor configuration sufficient for one-click build, flash,
debug, and language-server integration in VS Code, generated from the same board metadata
the runtime already consumes.

#### Scenario: User opens a scaffolded project in VS Code
- **WHEN** a user opens a freshly scaffolded project in VS Code
- **THEN** clangd resolves headers and compile flags from the generated compile commands
- **AND** build, flash, and monitor are available as tasks
- **AND** at least one debug launch configuration is available for boards whose manifest
  declares a supported debug probe

### Requirement: Runtime Tooling Shall Support Raw-MCU Scaffolding Without Synthesizing Boards

The scaffolding command SHALL accept a raw MCU part number and produce a minimal board
layer derived from `alloy-devices` descriptors, without synthesizing peripheral support that
the descriptor does not declare.

#### Scenario: User scaffolds for a raw MCU with descriptor support
- **WHEN** a user scaffolds with a raw MCU part number whose descriptor is available
- **THEN** the generated project includes startup, clock, and linker configuration derived
  from the descriptor
- **AND** the generated project does not declare or expose peripherals beyond what the
  descriptor guarantees

#### Scenario: Raw MCU scaffolding does not invent hardware facts
- **WHEN** the descriptor lacks data needed for a generated board layer
- **THEN** the scaffolding command fails with a message that names the missing fact
- **AND** it does not write a partial project that papers over the missing data

### Requirement: Runtime Tooling Shall Maintain A Single User-Facing Entry Point

The runtime tooling layer SHALL expose its lifecycle commands (scaffolding, configure,
build, flash, monitor, diagnostics, SDK management, toolchain management) under a single
documented entry point installable independently of a runtime checkout.

#### Scenario: User installs the tooling without cloning the runtime
- **WHEN** a user installs the published tooling package
- **THEN** the user can run scaffolding, configure, build, flash, and monitor without first
  cloning the Alloy runtime repository
- **AND** the tooling acquires the runtime and descriptors on demand through the SDK manager

#### Scenario: Legacy script remains a working alias during transition
- **WHEN** a user invokes the legacy `alloyctl.py` script during the transition window
- **THEN** the script delegates to the new entry point and produces equivalent results
- **AND** it prints a one-line notice naming the supported entry point
