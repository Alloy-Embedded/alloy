## MODIFIED Requirements

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
