# runtime-device-boundary Specification

## Purpose
The runtime-device boundary defines the only supported generated C++ contract between
`alloy-devices` and `alloy`.

`alloy` consumes typed runtime artifacts from the published device contract through a stable import
layer in `src/device/**`. Reflection-oriented or legacy generated headers are allowed only for
smoke, tooling, or migration shims, never as the normal hot-path dependency.
## Requirements
### Requirement: Alloy Hot Path Shall Target Runtime-Lite Contract

`alloy` SHALL consume the runtime device contract as its default hot-path boundary. The
reflection contract SHALL remain available only for smoke, tooling, and optional inspection.

#### Scenario: Runtime includes only runtime device headers

- **WHEN** a foundational driver implementation includes generated device artifacts for normal
  operation
- **THEN** it includes runtime device headers
- **AND** it does not require reflection headers as a normal dependency

### Requirement: Runtime Must Consume `alloy-devices` Through a Stable Import Layer

`alloy` SHALL consume the published device contract through `generated/runtime/**` as its only
supported generated C++ boundary.

#### Scenario: Selected config includes only runtime contract headers
- **WHEN** a board selects a published device from `alloy-devices`
- **THEN** the generated selected config includes the typed runtime contract headers
- **AND** it does not include legacy generated descriptor headers from `generated/devices/<device>/`
  for runtime behavior

#### Scenario: Runtime path does not depend on legacy generated tables
- **WHEN** GPIO/UART/SPI/I2C/DMA/timer/PWM/ADC/DAC/startup/system clock are compiled
- **THEN** they consume the typed runtime contract
- **AND** they do not require table-oriented generated C++ reflection headers

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

### Requirement: Runtime-Device Compatibility Shall Be A Managed Product Boundary

The runtime/device boundary SHALL be versioned and managed as a compatibility boundary, not just
an implementation detail.

#### Scenario: Runtime expects a newer generated contract
- **WHEN** `alloy` requires generated contract features not present in an older `alloy-devices` publish
- **THEN** the incompatibility is detected and reported explicitly
- **AND** it is not left to fail later as an unclear compile or link break
