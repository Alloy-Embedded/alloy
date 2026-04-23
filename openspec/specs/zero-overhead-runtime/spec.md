# zero-overhead-runtime Specification

## Purpose
TBD - created by archiving change consume-runtime-device-contract. Update Purpose after archive.
## Requirements
### Requirement: Foundational Runtime Paths Shall Not Require Family-Wide Descriptor Scans

Normal foundational driver paths in `alloy` SHALL NOT require generic scans over family-wide
generated descriptor tables to resolve one selected peripheral instance or route.

#### Scenario: Opening board debug UART

- **WHEN** board code opens its selected debug UART through the public HAL
- **THEN** the implementation resolves instance, clock/reset, and route data through generated
  runtime compile-time constructs
- **AND** it does not depend on a generic family-wide table walk at runtime

### Requirement: Runtime-Lite Consumption Shall Be Zero-Overhead By Construction

The runtime SHALL treat generated runtime refs, traits, and operation packs as the primary
implementation boundary for foundational bring-up paths.

#### Scenario: GPIO mode setup on a foundational board

- **WHEN** a foundational board configures a GPIO pin through the public HAL
- **THEN** the compiled path can be reduced to direct register writes implied by the selected
  generated runtime data

### Requirement: Optional Async Support Shall Not Penalize Blocking-Only Hot Paths

The runtime SHALL keep blocking-only foundational hot paths free from async-layer overhead when
the async layer is not selected.

#### Scenario: Blocking SPI transaction in non-async build
- **WHEN** a foundational SPI transaction is compiled in a blocking-only build
- **THEN** the generated code does not pay for optional async infrastructure
- **AND** zero-overhead expectations remain enforced

