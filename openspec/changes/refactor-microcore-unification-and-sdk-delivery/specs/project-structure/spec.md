## ADDED Requirements

### Requirement: Canonical Naming and Migration Compatibility

The project SHALL define a canonical naming contract for namespaces, CMake variables, compile definitions, and public identifiers, while preserving migration compatibility for legacy identifiers during a documented deprecation period.

#### Scenario: Canonical naming in new code paths
- **WHEN** new code, docs, or build modules are added
- **THEN** they SHALL use the canonical naming contract (`ucore` namespace and `MICROCORE_*` configuration identifiers)
- **AND** they SHALL NOT introduce new legacy-prefixed identifiers

#### Scenario: Legacy compatibility remains available during migration
- **WHEN** existing code uses legacy identifiers
- **THEN** compatibility aliases SHALL keep builds working
- **AND** those aliases SHALL map to canonical identifiers without changing runtime behavior

#### Scenario: Deprecation diagnostics are emitted
- **WHEN** a legacy identifier is used in supported build paths
- **THEN** the build or tooling SHALL emit a deprecation warning with migration guidance

### Requirement: Canonical Public Build Target Graph

The project SHALL expose stable public CMake targets for framework consumption, independent from internal target layout.

#### Scenario: Public targets are stable and discoverable
- **WHEN** a consumer links against the framework
- **THEN** canonical public targets SHALL be available (`microcore::hal` and board/rtos targets where applicable)
- **AND** target names SHALL remain stable across patch and minor releases

#### Scenario: Legacy target names resolve during migration
- **WHEN** an existing project links legacy target names
- **THEN** alias targets SHALL resolve to the canonical public targets
- **AND** migration guidance SHALL be provided for future removal
