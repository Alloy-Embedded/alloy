## ADDED Requirements

### Requirement: Consumer Mode Without Runtime Codegen Dependency

Framework consumers SHALL be able to build and use supported boards/examples without executing code generation tools at build time.

#### Scenario: Consumer build does not require Python generator runtime
- **WHEN** a user consumes a release artifact in consumer mode
- **THEN** build and link SHALL succeed using pre-generated HAL artifacts
- **AND** no Python codegen execution SHALL be required

#### Scenario: Contributor mode supports regeneration workflows
- **WHEN** a contributor updates metadata/templates
- **THEN** contributor workflows SHALL allow regeneration and validation of generated artifacts
- **AND** regenerated artifacts SHALL be verifiable against schema and compatibility checks

### Requirement: Codegen Product Boundary for Extraction

The code generator SHALL expose a stable contract that allows extraction to a dedicated repository without breaking framework consumption.

#### Scenario: Stable generator interface contract exists
- **WHEN** codegen tooling is invoked in CI or contributor workflows
- **THEN** invocation SHALL use versioned commands, schemas, and artifact outputs
- **AND** framework CI SHALL verify compatibility between generator version and artifact contract

#### Scenario: Extraction readiness gate is enforced
- **WHEN** maintainers evaluate moving codegen to a separate repository
- **THEN** extraction SHALL be allowed only if compatibility gates pass
- **AND** rollback to previously pinned generated artifacts SHALL be documented and available
