# startup-runtime Specification

## Purpose
TBD - created by archiving change rebuild-runtime-around-alloy-devices. Update Purpose after archive.
## Requirements
### Requirement: Startup Behavior Shall Live in the Runtime

Reset handling, memory initialization, and architecture bootstrap behavior SHALL live in `alloy`
runtime code.

#### Scenario: Cortex-M reset sequence
- **WHEN** a foundational Cortex-M target starts
- **THEN** the runtime executes the reset algorithm from shared architecture code
- **AND** the algorithm consumes startup descriptors from `alloy-devices`

### Requirement: Startup Data Shall Come from Published Device Descriptors

`alloy` SHALL consume startup descriptors through the typed runtime startup contract.

#### Scenario: Device startup wrapper aliases runtime startup contract
- **WHEN** `alloy::device::startup` is included
- **THEN** it aliases the generated runtime startup contract
- **AND** it does not depend on `generated/devices/<device>/startup_descriptors.hpp`

#### Scenario: Boards still link generated startup implementation units
- **WHEN** a descriptor-driven board build is configured
- **THEN** it can still compile and link generated startup implementation sources
- **AND** its descriptor-facing startup include path remains runtime-scoped

### Requirement: Startup Runtime Shall Provide Typed Interrupt Integration For Runtime Events

The startup/runtime layer SHALL expose typed interrupt entry points that the runtime event model
can bind to without recreating dynamic interrupt ownership outside the generated contract.

#### Scenario: Runtime event model consumes generated interrupt stubs
- **WHEN** the runtime attaches an interrupt-driven completion path
- **THEN** it binds through generated interrupt ids and startup stubs
- **AND** it does not depend on handwritten family-wide interrupt registries

