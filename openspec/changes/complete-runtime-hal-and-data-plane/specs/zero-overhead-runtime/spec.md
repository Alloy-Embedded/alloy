## ADDED Requirements

### Requirement: DMA-Backed Foundational Hot Paths Shall Stay Zero-Overhead By Construction

DMA-backed foundational driver paths SHALL preserve the same descriptor-driven zero-overhead
property expected from non-DMA hot paths.

#### Scenario: DMA-backed UART transmit path
- **WHEN** a foundational board uses a DMA-backed UART transmit path through the public HAL
- **THEN** the compiled hot path is resolved from typed runtime descriptors and direct register operations
- **AND** it does not introduce runtime scans, reflection lookups, or virtual dispatch
