## ADDED Requirements

### Requirement: Runtime-Device Compatibility Shall Be A Managed Product Boundary

The runtime/device boundary SHALL be versioned and managed as a compatibility boundary, not just
an implementation detail.

#### Scenario: Runtime expects a newer generated contract
- **WHEN** `alloy` requires generated contract features not present in an older `alloy-devices` publish
- **THEN** the incompatibility is detected and reported explicitly
- **AND** it is not left to fail later as an unclear compile or link break
