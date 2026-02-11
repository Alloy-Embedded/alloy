## ADDED Requirements

### Requirement: Installable SDK Package

The framework SHALL provide an installable SDK package exportable to external CMake consumers.

#### Scenario: SDK installation exports CMake package config
- **WHEN** the framework is installed
- **THEN** CMake package configuration files SHALL be installed
- **AND** public framework targets SHALL be exported for consumer linkage

#### Scenario: Consumer links with exported targets
- **WHEN** an external project calls `find_package(microcore CONFIG REQUIRED)`
- **THEN** it SHALL link against exported public targets without manual include/link path patching

### Requirement: Consumer and Contributor Delivery Profiles

Distribution SHALL define separate delivery profiles for framework consumers and framework contributors.

#### Scenario: Consumer profile excludes generator runtime dependency
- **WHEN** a consumer installs framework release artifacts
- **THEN** required files SHALL include runtime headers, libraries, board metadata, and generated artifacts
- **AND** generator development tooling SHALL be optional

#### Scenario: Contributor profile includes regeneration tooling
- **WHEN** a contributor sets up a development environment
- **THEN** contributor profile SHALL include codegen tooling and validation scripts
- **AND** generated artifact contracts SHALL be validated before publishing

### Requirement: Versioned Artifact Compatibility Contract

SDK releases SHALL define compatibility between framework versions and generated artifact versions.

#### Scenario: Release compatibility is validated
- **WHEN** a release is built
- **THEN** CI SHALL validate that packaged generated artifacts satisfy the declared compatibility contract
- **AND** incompatibilities SHALL fail release publication

#### Scenario: Rollback is possible with pinned artifact versions
- **WHEN** a generator or artifact incompatibility is detected
- **THEN** release pipelines SHALL support rollback to last known compatible artifact version
- **AND** rollback procedure SHALL be documented
