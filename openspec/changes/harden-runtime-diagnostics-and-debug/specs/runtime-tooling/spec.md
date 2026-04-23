## ADDED Requirements

### Requirement: Diagnostics Shall Prefer Valid Alternatives

User-facing runtime tooling diagnostics SHALL prefer actionable alternatives over generic failure
messages when the repo has enough information to do so.

#### Scenario: Invalid board-oriented connector choice

- **WHEN** a user selects an unsupported or conflicting connector/resource path
- **THEN** the tooling explains the conflict in board/runtime terms
- **AND** it lists valid alternatives when that information is available

### Requirement: Target Diffs Shall Support Migration Work

The runtime tooling diff path SHALL summarize target-to-target differences in a form useful for
migration work.

#### Scenario: User compares two foundational boards

- **WHEN** a user compares two supported boards
- **THEN** the diff highlights meaningful differences in clocks, required examples, release gates,
  and supported paths
- **AND** it does not stop at raw metadata dumps
