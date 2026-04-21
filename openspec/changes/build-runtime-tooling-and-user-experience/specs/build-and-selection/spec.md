## ADDED Requirements

### Requirement: Build Selection Docs Shall Match Supported Runtime Entry Points

Build-and-selection documentation and presets SHALL describe only supported board-oriented runtime
entry points.

#### Scenario: User follows a documented preset flow
- **WHEN** a user follows the documented configure/build flow for a supported board
- **THEN** the named preset or target exists and succeeds on the claimed validation path
