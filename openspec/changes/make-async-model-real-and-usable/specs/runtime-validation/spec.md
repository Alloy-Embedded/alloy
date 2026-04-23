## ADDED Requirements

### Requirement: Async Claims Shall Be Backed By A Real Driver Validation Path

Claims about the runtime async/event model SHALL be backed by validation of at least one real
driver path.

#### Scenario: Validating completion-driven runtime behavior

- **WHEN** the repo claims a supported completion-driven path
- **THEN** validation covers that path through host, emulation, or hardware evidence
- **AND** the example/docs point at the same path
