## ADDED Requirements

### Requirement: Deterministic Example Target Resolution

The CLI SHALL resolve example invocations to canonical build targets using explicit mappings rather than path heuristics.

#### Scenario: Nested example resolves to canonical target
- **WHEN** a user runs `ucore build <board> <nested-example-path>`
- **THEN** the CLI SHALL resolve to the mapped canonical CMake target
- **AND** it SHALL not infer target names by only truncating path segments

#### Scenario: Missing target mapping produces actionable error
- **WHEN** an example exists but has no configured target mapping
- **THEN** the CLI SHALL fail with an explicit mapping error
- **AND** the error SHALL list available mapped targets and correction hints

### Requirement: Metadata-Driven Board and Example Discovery

The CLI SHALL use canonical metadata sources for board/example listing and validation.

#### Scenario: Board listing reflects canonical metadata
- **WHEN** a user runs `ucore list boards`
- **THEN** listed boards SHALL come from canonical board metadata
- **AND** displayed properties SHALL match schema-defined metadata fields

#### Scenario: Example listing reflects buildable entries
- **WHEN** a user runs `ucore list examples`
- **THEN** examples SHALL include only configured buildable entries
- **AND** each entry SHALL have an associated canonical target mapping

### Requirement: Automation-Friendly Command Behavior

The CLI SHALL support non-interactive execution suitable for CI/CD workflows.

#### Scenario: Non-interactive build/flash mode
- **WHEN** non-interactive mode is enabled
- **THEN** CLI commands SHALL avoid blocking prompts
- **AND** commands requiring user interaction SHALL fail fast with machine-readable guidance
