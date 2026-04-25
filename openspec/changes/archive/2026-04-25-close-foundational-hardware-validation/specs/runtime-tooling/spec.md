## ADDED Requirements

### Requirement: Runtime Tooling Shall Expose Supported Recovery Paths For Foundational Boards

The board-oriented tooling layer SHALL expose documented recovery paths where a foundational board
can be trapped by a bad firmware image.

#### Scenario: User recovers a trapped STM32 board

- **WHEN** a foundational STM32 board needs recovery after a bad flash
- **THEN** the supported tooling/doc path explains how to recover it
- **AND** the recovery path is part of the documented product story
