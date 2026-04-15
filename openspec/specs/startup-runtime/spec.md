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

The runtime SHALL consume startup vectors and startup descriptors from `alloy-devices`.

#### Scenario: Device vector integration
- **WHEN** a selected device provides startup vectors
- **THEN** the build integrates those generated vectors into the runtime startup path
- **AND** does not depend on handwritten family-specific startup translation units in the runtime repo

