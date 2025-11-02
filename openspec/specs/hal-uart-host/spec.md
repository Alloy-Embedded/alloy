# hal-uart-host Specification

## Purpose
TBD - created by archiving change add-uart-host-impl. Update Purpose after archive.
## Requirements
### Requirement: Host UART Implementation

The system SHALL provide host-based UART using stdin/stdout.

#### Scenario: Write outputs to stdout
- **WHEN** calling write_byte('A')
- **THEN** 'A' SHALL be written to stdout
- **AND** Result<void>::ok() SHALL be returned

#### Scenario: Read uses stdin
- **WHEN** calling read_byte() with data in stdin
- **THEN** it SHALL return Result<uint8_t>::ok(data)
- **WHEN** calling read_byte() with no data
- **THEN** it SHALL return Result<uint8_t>::error(ErrorCode::Timeout)

#### Scenario: Available checks stdin buffer
- **WHEN** calling available()
- **THEN** it SHALL return number of bytes in stdin buffer

### Requirement: Concept Compliance

The host UART implementation SHALL satisfy the UartDevice concept.

#### Scenario: Concept requirements met
- **WHEN** static_assert(UartDevice<host::Uart>) is evaluated
- **THEN** it SHALL compile without errors

