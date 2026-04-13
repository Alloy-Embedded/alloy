# zero-overhead-runtime Specification

## Purpose
TBD - created by archiving change consume-runtime-lite-device-contract. Update Purpose after archive.
## Requirements
### Requirement: Foundational Runtime Paths Shall Not Require Family-Wide Descriptor Scans

Normal foundational driver paths in `alloy` SHALL NOT require generic scans over family-wide
generated descriptor tables to resolve one selected peripheral instance or route.

#### Scenario: Opening board debug UART

- **WHEN** board code opens its selected debug UART through the public HAL
- **THEN** the implementation resolves instance, clock/reset, and route data through generated
  runtime-lite compile-time constructs
- **AND** it does not depend on a generic family-wide table walk at runtime

### Requirement: Runtime-Lite Consumption Shall Be Zero-Overhead By Construction

The runtime SHALL treat generated runtime-lite refs, traits, and operation packs as the primary
implementation boundary for foundational bring-up paths.

#### Scenario: GPIO mode setup on a foundational board

- **WHEN** a foundational board configures a GPIO pin through the public HAL
- **THEN** the compiled path can be reduced to direct register writes implied by the selected
  generated runtime-lite data

