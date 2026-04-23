## ADDED Requirements

### Requirement: Zero-Overhead Claims Shall Be Guarded By Explicit Regression Gates

Public claims about zero-overhead runtime behavior SHALL be guarded by explicit regression gates.

#### Scenario: Release claims zero-overhead foundational UART path
- **WHEN** a release or support matrix claims zero-overhead behavior for a foundational path
- **THEN** that claim is backed by an explicit validation gate such as assembly, size, or focused hot-path verification
- **AND** regressions fail before the claim ships
