## ADDED Requirements

### Requirement: Cleanup Shall Be a First-Class Migration Phase

The migration SHALL begin with an explicit cleanup phase that identifies obsolete and incompatible
runtime layers.

#### Scenario: Legacy architecture audit
- **WHEN** the rebuild starts
- **THEN** the repo classifies legacy files and directories as keep, rewrite, or delete
- **AND** the implementation plan follows that classification

### Requirement: Public Story Shall Match the Rebuilt Runtime

Documentation and examples SHALL reflect the active public architecture only.

#### Scenario: Post-cleanup documentation
- **WHEN** the rebuild removes an old API or build path
- **THEN** active docs and examples stop teaching that path
- **AND** the new runtime path becomes the documented default
