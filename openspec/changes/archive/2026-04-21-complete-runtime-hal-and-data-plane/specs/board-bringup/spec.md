## ADDED Requirements

### Requirement: Board Helpers Shall Wrap The Public HAL

Board-level helper functions for foundational peripherals SHALL be thin wrappers over the public
HAL surface.

#### Scenario: Board helper opens debug UART
- **WHEN** a board exposes a helper such as a debug UART constructor or timer/PWM helper
- **THEN** the helper delegates to the public HAL path
- **AND** it does not become a second board-private API family for the same peripheral
