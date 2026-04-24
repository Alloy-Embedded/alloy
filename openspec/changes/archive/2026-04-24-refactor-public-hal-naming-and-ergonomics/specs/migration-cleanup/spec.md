## ADDED Requirements

### Requirement: Ergonomic Renaming Shall Not Rewrite The Canonical Runtime Contract

Public API ergonomic renaming SHALL leave the canonical generated runtime contract stable across
the runtime/device boundary.

#### Scenario: Runtime imports the published device contract

- **WHEN** the runtime consumes `alloy-devices` artifacts
- **THEN** it continues to import canonical generated ids and facts through the existing contract
- **AND** the ergonomic public aliases remain a thin runtime-side façade rather than a second code
  generation contract

### Requirement: Docs Shall Teach The Ergonomic Surface First

Active docs and examples SHALL teach the ergonomic public spellings first once that layer exists.

#### Scenario: User reads a raw HAL cookbook page

- **WHEN** a user reads active documentation for direct HAL usage
- **THEN** the page teaches the concise public aliases and route names as the normal path
- **AND** canonical internal spellings only appear as expert detail, migration note, or boundary
  explanation

### Requirement: Migration Shall Preserve Compatibility Before Any Deprecation

The migration to ergonomic public names SHALL preserve source compatibility for the canonical forms
before any selective deprecation is considered.

#### Scenario: Existing canonical-form application compiles during migration

- **WHEN** an application still uses canonical names such as `device::PinId::*` or
  `connection::connector<...>`
- **THEN** the build remains supported during the documented migration window
- **AND** the repo adds the ergonomic aliases and updated docs before removing or de-emphasizing
  the old public spellings
