## ADDED Requirements

### Requirement: Async Claims Shall Be Backed By A Real Driver Validation Path

Claims about the runtime async/event model SHALL be backed by validation of at least one real
driver path.

#### Scenario: Validating completion-driven runtime behavior

- **WHEN** the repo claims a supported completion-driven path
- **THEN** validation covers that path through host, emulation, or hardware evidence
- **AND** the example/docs point at the same path

### Requirement: Canonical Completion Path Shall Carry Host-Level Coverage

The canonical completion+timeout path SHALL carry host-MMIO or equivalent scripted coverage that
exercises completion and timeout outcomes without requiring silicon.

#### Scenario: Completion fires before the deadline

- **WHEN** the host-level harness simulates the completion signal before the timeout expires
- **THEN** the test observes the completed outcome
- **AND** the test fails if the runtime reports timeout in this case

#### Scenario: Completion never fires before the deadline

- **WHEN** the host-level harness withholds the completion signal past the deadline
- **THEN** the test observes the timeout outcome
- **AND** the test fails if the runtime reports a spurious completion

### Requirement: Blocking-Only Build Shall Stay Gated In Validation

At least one validation target SHALL build and exercise a blocking-only configuration of the
canonical path to prove the async layer stays optional.

#### Scenario: Blocking-only configuration build

- **WHEN** validation builds the canonical driver path with the async adapter disabled
- **THEN** the build succeeds
- **AND** the resulting image does not reference any symbol from the async adapter layer

### Requirement: Low Power Wake Path Shall Carry Observable Coverage

At least one low-power wake path coordinated with the runtime time/event model SHALL carry
observable coverage (host-MMIO, emulation, or recorded hardware spot-check).

#### Scenario: Supported wake source triggers a waiting operation

- **WHEN** the validation harness enters a supported low-power mode with a valid wake source
- **AND** the wake source fires
- **THEN** the harness observes the runtime completing the waiting operation on the same
  time/event model
- **AND** the evidence is referenced from the runtime async validation notes
