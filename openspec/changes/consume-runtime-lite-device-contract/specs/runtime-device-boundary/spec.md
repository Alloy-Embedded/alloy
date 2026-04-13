## ADDED Requirements

### Requirement: Alloy Hot Path Shall Target Runtime-Lite Contract

`alloy` SHALL consume the runtime-lite device contract as its default hot-path boundary. The
reflection contract SHALL remain available only for smoke, tooling, and optional inspection.

#### Scenario: Runtime includes only runtime-lite device headers

- **WHEN** a foundational driver implementation includes generated device artifacts for normal
  operation
- **THEN** it includes runtime-lite device headers
- **AND** it does not require reflection headers as a normal dependency
