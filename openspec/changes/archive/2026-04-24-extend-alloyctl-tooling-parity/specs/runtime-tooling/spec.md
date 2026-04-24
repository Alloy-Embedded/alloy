## ADDED Requirements

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
