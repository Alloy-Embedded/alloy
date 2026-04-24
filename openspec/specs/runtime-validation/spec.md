# runtime-validation Specification

## Purpose
TBD - created by archiving change add-runtime-validation-architecture. Update Purpose after archive.
## Requirements
### Requirement: Runtime Validation Shall Live In Alloy

Validation of descriptor-driven startup, bring-up, routing, and driver behavior SHALL live in the
`alloy` repo.

#### Scenario: Classifying validation ownership

- **WHEN** a test harness proves how the runtime consumes device descriptors
- **THEN** that harness lives in `alloy`
- **AND** `alloy-devices` remains an input source for descriptors and generated artifacts rather
  than the owner of runtime validation policy

### Requirement: Runtime Validation Shall Be Layered By Confidence

`alloy` SHALL organize runtime validation as a layered confidence ladder rather than one flat test
collection.

#### Scenario: Running validation in CI

- **WHEN** validation is executed in CI or locally
- **THEN** the available layers include compile-contract checks, ELF/startup inspection, host MMIO,
  emulation, hardware spot-checks, and assembly/size verification
- **AND** each layer answers a distinct validation question

### Requirement: Host MMIO Shall Validate Descriptor-Driven Bring-Up Without Hardware

`alloy` SHALL provide a host MMIO validation backend that records register effects for
descriptor-driven bring-up paths.

#### Scenario: Validating board initialization on the host

- **WHEN** a foundational board bring-up path is executed under the host MMIO backend
- **THEN** the test can assert which registers were accessed, in what order, and with what final
  values
- **AND** the production runtime path does not gain dynamic indirection from this test backend

### Requirement: Emulation Coverage Shall Start With Same70

`alloy` SHALL provide a representative emulation harness for descriptor-driven startup and
bring-up, and the first supported target SHALL be `same70`.

#### Scenario: Booting the first Renode target

- **WHEN** the initial emulation harness runs for `same70`
- **THEN** it loads the selected image, starts from the expected startup path, and reaches a
  deterministic boot milestone such as `main` or a known boot banner
- **AND** the test records failure if the image faults or never reaches that milestone

### Requirement: Representative Emulation Coverage Shall Be Explicit And UART-Backed

`alloy` SHALL document which foundational boards are currently covered by the representative
emulation ladder, and each covered board SHALL prove both boot progress and a deterministic debug
UART observable.

#### Scenario: Claiming emulation coverage for a foundational board

- **WHEN** `alloy` claims that a foundational board is covered by the runtime-validation emulation
  ladder
- **THEN** the documented board matrix identifies that board explicitly
- **AND** the test proves a deterministic boot milestone plus a deterministic debug-UART output for
  that board

### Requirement: Representative Runtime Validation Shall Run In CI

`alloy` SHALL execute its representative host MMIO and emulation ladders in GitHub Actions CI.

#### Scenario: Running the current runtime-validation matrix in CI

- **WHEN** GitHub Actions runs the runtime-validation matrix
- **THEN** representative host MMIO validation runs for the documented foundational board set
- **AND** representative emulation validation runs for the documented foundational board set
- **AND** a failure in either ladder fails CI

### Requirement: Validation Assets Shall Stay Thin Over Published Device Facts

Validation assets in `alloy` SHALL avoid becoming a second handwritten device database.

#### Scenario: Building a Renode overlay

- **WHEN** a `Renode` platform overlay or host MMIO scenario is added
- **THEN** it reuses published device descriptors, startup artifacts, or existing metadata where
  possible
- **AND** only test-specific glue is handwritten inside `alloy`

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

