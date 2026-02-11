## ADDED Requirements

### Requirement: Board Metadata Single Source of Truth

Board configuration SHALL be defined from a canonical board metadata source, and all derived board artifacts SHALL be generated or validated against that source.

#### Scenario: Board metadata drives generated artifacts
- **WHEN** board metadata is updated
- **THEN** generated board artifacts (headers/config fragments/CMake fragments) SHALL be regenerated or validated
- **AND** the resulting build configuration SHALL remain semantically consistent with metadata

#### Scenario: Manual drift is detected
- **WHEN** a manually edited board artifact diverges from canonical metadata
- **THEN** validation SHALL fail with a clear mismatch report
- **AND** the report SHALL indicate the metadata fields responsible for the divergence

### Requirement: Namespace and Board Symbol Consistency

Board-generated and board-authored symbols SHALL use consistent namespace and naming contracts.

#### Scenario: Board namespace consistency
- **WHEN** board headers and board configuration files are included together
- **THEN** they SHALL resolve the same board namespace contract
- **AND** they SHALL expose consistent board-level aliases for clocks, pins, and peripherals

#### Scenario: Generated board artifacts compile without manual namespace patching
- **WHEN** a board configuration is generated from metadata
- **THEN** board integration SHALL compile without requiring hand-edited namespace fixes
