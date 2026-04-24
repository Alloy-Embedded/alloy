## ADDED Requirements

### Requirement: Tooling Diagnostics Shall Prefer Ergonomic API Spellings

User-facing diagnostics and explain/diff tooling SHALL prefer the ergonomic public API spellings
when presenting connector, route, or capability information.

#### Scenario: User inspects a failed UART route choice

- **WHEN** a user asks the tooling to explain a failed or invalid route/configuration choice
- **THEN** the first rendered spelling uses the ergonomic public names such as `dev::USART2`,
  `dev::PA2`, and `tx<dev::PA2>`
- **AND** canonical generated names may still appear as secondary detail for expert debugging
