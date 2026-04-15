# runtime-device-boundary Specification

## Purpose
TBD - created by archiving change consume-runtime-lite-device-contract. Update Purpose after archive.
## Requirements
### Requirement: Alloy Hot Path Shall Target Runtime-Lite Contract

`alloy` SHALL consume the runtime-lite device contract as its default hot-path boundary. The
reflection contract SHALL remain available only for smoke, tooling, and optional inspection.

#### Scenario: Runtime includes only runtime-lite device headers

- **WHEN** a foundational driver implementation includes generated device artifacts for normal
  operation
- **THEN** it includes runtime-lite device headers
- **AND** it does not require reflection headers as a normal dependency

### Requirement: Runtime Must Consume `alloy-devices` Through a Stable Import Layer

The runtime SHALL consume selected device descriptors through a stable `src/device` import layer
rather than scattering direct vendor/family path includes across boards and drivers.

#### Scenario: Driver includes selected device descriptors
- **WHEN** a foundational driver needs pins, peripheral instances, startup descriptors, or package data
- **THEN** it imports them through `alloy::device::selected::*`
- **AND** it does not directly include a vendor/family path from `alloy-devices`

### Requirement: Runtime Must Not Own Cross-Vendor Hardware Facts

The runtime SHALL NOT define handwritten cross-vendor pin, peripheral, or signal enumerations as
its primary source of truth.

#### Scenario: Connector model uses generated facts
- **WHEN** the runtime validates a connection
- **THEN** it uses generated connector descriptors from `alloy-devices`
- **AND** it does not rely on a handwritten global runtime enum table

### Requirement: Runtime Must Be Allowed to Delete Incompatible Legacy Code

The migration SHALL permit deletion of incompatible legacy code when that is the clearest path to
the new runtime/device boundary.

#### Scenario: Legacy module conflicts with descriptor-driven runtime
- **WHEN** an old module duplicates or contradicts the descriptor-driven path
- **THEN** the migration may remove or archive it instead of preserving compatibility glue

