# migration-cleanup Specification

## Purpose
Migration cleanup defines the rules for retiring legacy code as the runtime converges on the
descriptor-driven public HAL surface.

When a foundational peripheral class gains a finished public API path, any residual
family-private glue for that class is either deleted from the active path or clearly
marked as temporary adaptation.  Documentation and examples stop teaching the old path
the moment the new path is the documented default.
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

### Requirement: Legacy generated C++ header usage is removed from Alloy

`alloy` MUST remove remaining dependence on legacy generated C++ headers after the runtime contract
becomes the sole supported published contract.

#### Scenario: Host validation overrides use runtime startup contract
- **WHEN** host MMIO or other selected-config overrides are generated
- **THEN** they include the typed runtime startup contract
- **AND** they do not include `startup_descriptors.hpp`

#### Scenario: Internal naming no longer implies a second public contract
- **WHEN** docs, selected import macros, or internal helper namespaces refer to the generated
  device contract
- **THEN** they describe it as the runtime contract
- **AND** they do not imply support for a second heavier published C++ contract

### Requirement: Active Docs Shall Teach The Supported Product Story

Active docs and examples SHALL teach the supported runtime product story rather than historical or
transitional repo layouts.

#### Scenario: User reads the main README
- **WHEN** a user reads active top-level docs
- **THEN** they learn the current board/build/runtime story
- **AND** they are not directed toward removed vendor trees or historical API layers as normal usage

