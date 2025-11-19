## ADDED Requirements

### Requirement: UART Echo Example

The system SHALL provide a UART echo example demonstrating UART usage.

#### Scenario: Example compiles
- **WHEN** running `cmake --build build`
- **THEN** uart_echo executable SHALL be created without errors

#### Scenario: Example echoes input
- **WHEN** running `./build/examples/uart_echo/uart_echo`
- **AND** typing characters into stdin
- **THEN** each character SHALL be echoed back to stdout

#### Scenario: Example uses UART interface
- **WHEN** reviewing uart_echo source code
- **THEN** it SHALL use UartDevice from hal/interface/uart.hpp
- **AND** it SHALL handle errors using Result<T, ErrorCode>
- **AND** it SHALL NOT directly use std::cin/cout
