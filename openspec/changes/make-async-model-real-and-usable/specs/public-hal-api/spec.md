## ADDED Requirements

### Requirement: Completion-Driven Paths Shall Stay On The Public HAL Surface

Interrupt-driven and DMA-driven completion paths SHALL stay on the same public HAL surface rather
than opening a separate family-private async API tier.

#### Scenario: User waits on UART DMA completion

- **WHEN** a user follows the supported completion-driven path for a public HAL class
- **THEN** they remain on the documented public HAL path plus runtime completion primitives
- **AND** they do not switch into a separate vendor-specific async API family
