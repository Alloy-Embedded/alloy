# migration-cleanup Specification

## Purpose
TBD - created by archiving change consume-runtime-lite-device-contract. Update Purpose after archive.
## Requirements
### Requirement: Reflection-Table Dependency Shall Be Removed From Foundational Hot Paths

The runtime migration SHALL remove or isolate any helper that keeps foundational driver and board
paths dependent on reflection-table lookup.

#### Scenario: Transitional runtime lookup helper remains only in non-hot-path code

- **WHEN** cleanup for this change is complete
- **THEN** foundational GPIO/UART/SPI/I2C paths no longer depend on transitional reflection-table
  helpers
- **AND** any remaining reflection helpers are isolated to smoke, debug, or tooling-only paths

### Requirement: Cleanup Shall Be a First-Class Migration Phase

The migration SHALL begin with an explicit cleanup phase that identifies obsolete and incompatible
runtime layers.

#### Scenario: Legacy architecture audit
- **WHEN** the rebuild starts
- **THEN** the repo classifies legacy files and directories as keep, rewrite, or delete
- **AND** the implementation plan follows that classification

### Requirement: Public Story Shall Match the Rebuilt Runtime

Documentation and examples SHALL reflect the active public architecture only.

#### Scenario: Post-cleanup documentation
- **WHEN** the rebuild removes an old API or build path
- **THEN** active docs and examples stop teaching that path
- **AND** the new runtime path becomes the documented default

