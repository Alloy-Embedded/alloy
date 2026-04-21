## ADDED Requirements

### Requirement: Optional Async Support Shall Not Penalize Blocking-Only Hot Paths

The runtime SHALL keep blocking-only foundational hot paths free from async-layer overhead when
the async layer is not selected.

#### Scenario: Blocking SPI transaction in non-async build
- **WHEN** a foundational SPI transaction is compiled in a blocking-only build
- **THEN** the generated code does not pay for optional async infrastructure
- **AND** zero-overhead expectations remain enforced
