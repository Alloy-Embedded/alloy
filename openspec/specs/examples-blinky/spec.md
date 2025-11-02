# examples-blinky Specification

## Purpose
TBD - created by archiving change add-blinky-example. Update Purpose after archive.
## Requirements
### Requirement: Blinky Example

The system SHALL provide a blinky example that demonstrates GPIO usage on the host platform.

#### Scenario: Example compiles
- **WHEN** running `cmake --build build`
- **THEN** blinky executable SHALL be created without errors

#### Scenario: Example runs on host
- **WHEN** running `./build/examples/blinky/blinky`
- **THEN** it SHALL print GPIO toggle messages to console
- **AND** messages SHALL alternate between HIGH and LOW

#### Scenario: Example uses GPIO interface
- **WHEN** reviewing blinky source code
- **THEN** it SHALL use ConfiguredGpioPin from hal/interface/gpio.hpp
- **AND** it SHALL NOT directly depend on host implementation details

### Requirement: Platform Delay

The system SHALL provide a delay_ms() function for host platform.

#### Scenario: Delay function exists
- **WHEN** including `platform/delay.hpp`
- **THEN** delay_ms(uint32_t) function SHALL be available

#### Scenario: Delay blocks execution
- **WHEN** calling delay_ms(500)
- **THEN** execution SHALL pause for approximately 500 milliseconds

