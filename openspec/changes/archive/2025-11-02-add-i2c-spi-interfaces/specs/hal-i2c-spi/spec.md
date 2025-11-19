## ADDED Requirements

### Requirement: I2cMaster Concept

The system SHALL define an I2cMaster concept for I2C master implementations.

#### Scenario: I2C operations
- **WHEN** a type implements I2cMaster
- **THEN** it MUST provide read(address, buffer, length) returning Result<size_t, ErrorCode>
- **AND** it MUST provide write(address, buffer, length) returning Result<size_t, ErrorCode>
- **AND** it MUST provide configure(I2cConfig) method

#### Scenario: I2C error handling
- **WHEN** I2C operation fails
- **THEN** it SHALL return Result::error() with appropriate ErrorCode
- **AND** ErrorCode SHALL include: NACK, BusError, ArbitrationLost, Timeout

### Requirement: SpiMaster Concept

The system SHALL define a SpiMaster concept for SPI master implementations.

#### Scenario: SPI operations
- **WHEN** a type implements SpiMaster
- **THEN** it MUST provide transfer(tx, rx, length) returning Result<size_t, ErrorCode>
- **AND** it MUST provide select() and deselect() for chip select
- **AND** it MUST provide configure(SpiConfig) method

#### Scenario: SPI configuration
- **WHEN** configuring SPI
- **THEN** SpiConfig SHALL include mode (CPOL, CPHA)
- **AND** it SHALL include bit order (MSB first / LSB first)
- **AND** it SHALL include clock speed
