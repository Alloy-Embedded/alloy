# board-bringup Spec Delta: Custom Board Bring-Up

## ADDED Requirements

### Requirement: The Build SHALL Accept A Board Declared Outside The Runtime Tree

The runtime SHALL provide a documented bring-up path through which a consuming project
declares its own board outside the runtime tree without bypassing the descriptor-driven
runtime services or the public board contract.

#### Scenario: User project declares a custom board
- **WHEN** a downstream project sets `ALLOY_BOARD=custom` and provides the documented
  custom-board cache variables before adding alloy as a subproject
- **THEN** the runtime resolves the device contract, clocks, startup, and connectors
  for the supplied vendor/family/device tuple through the same descriptor-driven path
  used by in-tree boards
- **AND** the user-supplied board header and linker script are consumed without the
  runtime injecting any board logic of its own

### Requirement: The Custom Board Path SHALL Validate Its Inputs

The custom-board bring-up path SHALL fail fast at configure time when its contract is
not satisfied, with diagnostics that name the offending input.

#### Scenario: Missing required variable
- **WHEN** the custom-board branch is selected and a required cache variable is unset
- **THEN** configure fails with a single error line that names the missing variable
- **AND** descriptor resolution is not attempted

#### Scenario: Descriptor missing for declared device
- **WHEN** the custom-board branch is selected and the supplied vendor/family/device
  tuple has no descriptor under `ALLOY_DEVICES_ROOT`
- **THEN** configure fails with an error that names the missing descriptor path
- **AND** points the user at `alloy-devices` as the place to add support

#### Scenario: Invalid architecture or path inputs
- **WHEN** the user supplies `ALLOY_DEVICE_ARCH` outside the accepted set or a relative
  path for the board header or linker script
- **THEN** configure fails with an error that names the rejected value and the accepted
  values or expected form

### Requirement: The Custom Board Path SHALL Not Diverge From The In-Tree Bring-Up Contract

The custom-board branch SHALL emit the same set of outputs and use the same downstream
device-contract pipeline as in-tree foundational boards.

#### Scenario: Custom board uses the same device contract
- **WHEN** the custom-board branch and an in-tree branch resolve to the same
  vendor/family/device tuple
- **THEN** the device contract, startup behavior, and clock services produced for the
  custom board are identical to those produced for the in-tree board
- **AND** the runtime does not gate features on whether the board lives in the runtime
  tree
