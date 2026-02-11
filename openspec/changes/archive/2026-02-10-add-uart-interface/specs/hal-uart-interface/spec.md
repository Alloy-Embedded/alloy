## ADDED Requirements

### Requirement: UartDevice Concept

The system SHALL define a UartDevice concept for all UART implementations.

#### Scenario: Concept defines required operations
- **WHEN** a type implements UartDevice concept
- **THEN** it MUST provide `read_byte()` returning Result<uint8_t, ErrorCode>
- **AND** it MUST provide `write_byte(uint8_t)` returning Result<void, ErrorCode>
- **AND** it MUST provide `available()` returning size_t
- **AND** it MUST provide `configure(UartConfig)` method

### Requirement: UART Configuration

The system SHALL provide UartConfig struct for UART parameters.

#### Scenario: Config includes baud rate
- **WHEN** creating UartConfig
- **THEN** baud_rate field SHALL accept BaudRate type
- **AND** common rates (9600, 115200) SHALL have literal helpers

#### Scenario: Config includes format parameters
- **WHEN** creating UartConfig
- **THEN** it SHALL include data_bits (5, 6, 7, 8, 9)
- **AND** it SHALL include parity (None, Even, Odd)
- **AND** it SHALL include stop_bits (One, Two)

### Requirement: Error Handling

UART operations SHALL use Result<T, ErrorCode> for error reporting.

#### Scenario: Read returns error on timeout
- **WHEN** no data is available and timeout expires
- **THEN** read_byte() SHALL return Result<uint8_t>::error(ErrorCode::Timeout)

#### Scenario: Write returns error when busy
- **WHEN** transmit buffer is full
- **THEN** write_byte() SHALL return Result<void>::error(ErrorCode::Busy)
