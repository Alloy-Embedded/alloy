# runtime-tooling Specification

## Purpose
Runtime tooling defines the stable user-facing product layer for board-oriented configure, build,
flash, monitor, validation, diagnostics, and downstream consumption.

The goal is that users can discover what is supported, how to run it, and why a configuration
fails without reading internal CMake files or runtime implementation details first.
## Requirements
### Requirement: Runtime Tooling Shall Expose Stable Board-Oriented Entry Points

The repo SHALL expose stable and documented board-oriented entry points for configure, build,
validation, and supported flash/debug flows.

#### Scenario: User builds a foundational board
- **WHEN** a user selects a foundational board
- **THEN** the repo documents and exposes one supported path to configure and build it
- **AND** the user does not need to reverse-engineer internal CMake files first

### Requirement: Runtime Tooling Shall Surface Actionable Configuration Diagnostics

The user-facing tooling layer SHALL surface actionable diagnostics for invalid connector, clock,
or ownership configurations.

#### Scenario: Invalid connector choice
- **WHEN** a user selects an invalid connector or conflicting resource combination
- **THEN** the reported error explains the conflict in runtime terms
- **AND** it points toward valid alternatives when the generated contract provides them

### Requirement: Runtime Tooling Shall Support Explainable Diagnostics

When the generated contract publishes enough data, the runtime tooling layer SHALL be able to
explain why a connector, clock, or capability fact exists and how it differs across targets.

#### Scenario: User asks why a pin choice failed
- **WHEN** a user inspects a failed or unavailable connector/resource choice
- **THEN** the tooling can explain the relevant fact in runtime terms
- **AND** it may surface provenance or target-to-target differences when those inputs are available

### Requirement: Runtime Tooling Shall Support Downstream CMake Consumption

If `alloy` claims package-style downstream consumption, the runtime tooling layer SHALL provide a
documented and validated CMake integration path.

#### Scenario: Downstream application consumes Alloy through CMake package integration
- **WHEN** a downstream CMake project follows the documented package-style integration path
- **THEN** it can configure and link against `alloy` without reverse-engineering internal targets
- **AND** the documented integration path is covered by validation

### Requirement: Official Examples Shall Cover The Public Runtime Surface

The official example set SHALL cover the primary public runtime surface.

#### Scenario: User looks for a CAN or RTC example
- **WHEN** a primary public HAL class is documented as supported
- **THEN** the repo provides at least one official example or explicit cookbook path for that class
