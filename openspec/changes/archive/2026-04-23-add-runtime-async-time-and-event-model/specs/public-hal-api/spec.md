## ADDED Requirements

### Requirement: Blocking And Event-Driven Use Shall Share The Same Public HAL Shape

The public HAL SHALL keep one primary API shape even when a peripheral supports blocking,
event-driven, and async-backed operation.

#### Scenario: UART used in blocking and event-driven modes
- **WHEN** a UART is used first in blocking mode and later with event/completion handling
- **THEN** both uses remain on the same primary public HAL surface
- **AND** the user does not switch to a second family-specific API for event support
