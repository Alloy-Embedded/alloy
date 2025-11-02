# hal-gpio-interface Specification

## Purpose
TBD - created by archiving change add-gpio-interface. Update Purpose after archive.
## Requirements
### Requirement: GpioPin Concept

The system SHALL define a GpioPin concept that specifies the interface for all GPIO pin implementations.

#### Scenario: Concept defines required operations
- **WHEN** a type implements GpioPin concept
- **THEN** it MUST provide `set_high()` method returning void
- **AND** it MUST provide `set_low()` method returning void
- **AND** it MUST provide `toggle()` method returning void
- **AND** it MUST provide `read()` const method returning bool

### Requirement: Pin Mode Configuration

The system SHALL provide a PinMode enumeration for configuring GPIO pins.

#### Scenario: PinMode enum exists
- **WHEN** including `hal/interface/gpio.hpp`
- **THEN** PinMode enum class SHALL be available
- **AND** it SHALL include: Input, Output, InputPullUp, InputPullDown, Alternate, Analog

### Requirement: Compile-Time Pin Configuration

The system SHALL provide ConfiguredGpioPin template for compile-time pin and mode configuration.

#### Scenario: Template validates pin number
- **WHEN** creating ConfiguredGpioPin<PIN, MODE>
- **THEN** invalid pin numbers SHALL cause compile-time error via static_assert

#### Scenario: Template restricts operations by mode
- **WHEN** pin is configured as Output
- **THEN** set_high(), set_low(), toggle() SHALL be available
- **AND** read() SHALL NOT be available (compile-time restriction)

#### Scenario: Template restricts operations for input
- **WHEN** pin is configured as Input or InputPullUp
- **THEN** read() SHALL be available
- **AND** set_high(), set_low() SHALL NOT be available (compile-time restriction)

