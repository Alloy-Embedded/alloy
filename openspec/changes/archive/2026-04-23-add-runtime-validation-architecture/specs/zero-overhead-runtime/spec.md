## ADDED Requirements

### Requirement: Zero-Overhead Claims Shall Be Backed By Explicit Validation Gates

Claims that foundational runtime paths are zero-overhead SHALL be validated by dedicated gates and
not by architectural intent alone.

#### Scenario: Changing a foundational GPIO or UART hot path

- **WHEN** a change affects a foundational GPIO or UART bring-up path
- **THEN** assembly or size verification proves that the hot path still collapses to the expected
  low-level operations
- **AND** host MMIO or equivalent behavioral validation proves that the same path still performs
  the intended register effects
