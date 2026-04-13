# migration-cleanup Specification

## Purpose
TBD - created by archiving change consume-runtime-lite-device-contract. Update Purpose after archive.
## Requirements
### Requirement: Reflection-Table Dependency Shall Be Removed From Foundational Hot Paths

The runtime migration SHALL remove or isolate any helper that keeps foundational driver and board
paths dependent on reflection-table lookup.

#### Scenario: Transitional runtime lookup helper remains only in non-hot-path code

- **WHEN** cleanup for this change is complete
- **THEN** foundational GPIO/UART/SPI/I2C paths no longer depend on transitional reflection-table
  helpers
- **AND** any remaining reflection helpers are isolated to smoke, debug, or tooling-only paths

