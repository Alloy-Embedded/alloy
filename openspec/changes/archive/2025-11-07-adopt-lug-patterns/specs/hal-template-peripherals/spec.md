# HAL Template Peripherals Specification

## MODIFIED Requirements

### Requirement: UART Interface with Result Type
The UART interface SHALL return Result<T> types instead of raw error codes and SHALL inherit from Device base class.

#### Scenario: UART read returns Result
- **GIVEN** UART interface
- **WHEN** calling read() method
- **THEN** SHALL return Result<uint8_t>
- **AND** success case contains byte value
- **AND** error case contains std::error_code

#### Scenario: UART write returns Result
- **GIVEN** UART interface with buffer
- **WHEN** calling write(data, size)
- **THEN** SHALL return Result<size_t>
- **AND** success case contains bytes written
- **AND** error case contains timeout or hardware error

### Requirement: GPIO Interface with Result Type
The GPIO interface SHALL return Result<T> types and SHALL inherit from Device base class for ownership tracking.

#### Scenario: GPIO read returns Result
- **GIVEN** GPIO pin configured as input
- **WHEN** calling read()
- **THEN** SHALL return Result<bool>
- **AND** success case contains pin state (true/false)
- **AND** error case contains error code (not opened, hardware error)

#### Scenario: GPIO write returns Result
- **GIVEN** GPIO pin configured as output
- **WHEN** calling write(value)
- **THEN** SHALL return Result<void> or Result<std::monostate>
- **AND** success case indicates write completed
- **AND** error case contains error code

### Requirement: I2C Interface with Result Type and Bus Locking
The I2C interface SHALL return Result<T> types and SHALL provide bus locking mechanisms to prevent simultaneous access.

#### Scenario: I2C transfer returns Result
- **GIVEN** I2C interface
- **WHEN** calling transfer(addr, tx_data, rx_data)
- **THEN** SHALL return Result<size_t>
- **AND** success case contains bytes transferred
- **AND** error cases include: NACK, timeout, bus error

#### Scenario: I2C bus lock acquisition
- **GIVEN** I2C interface with lock support
- **WHEN** calling acquireBus(timeout)
- **THEN** SHALL return Result<void>
- **AND** success means bus is locked for exclusive access
- **AND** error if timeout expires waiting for bus

## ADDED Requirements

### Requirement: Template-Based Peripheral Implementation
The system SHALL provide template-based peripheral implementations that accept base address and channel as template parameters for zero-overhead abstraction.

#### Scenario: UART template instantiation
- **GIVEN** template class UartX<BASE_ADDR, IRQ_ID>
- **WHEN** creating type alias Uart0 = UartX<0x400E0800, ID_UART0>
- **THEN** BASE_ADDR SHALL be compile-time constant
- **AND** compiler SHALL inline all register accesses
- **AND** no runtime address lookup SHALL occur

#### Scenario: GPIO template instantiation
- **GIVEN** template class GpioPin<PORT_BASE, PIN_NUM>
- **WHEN** creating type alias LedGreen = GpioPin<PIOC_BASE, 8>
- **THEN** PIN_NUM SHALL be compile-time constant
- **AND** bit mask SHALL be computed at compile-time
- **AND** resulting code SHALL be equivalent to direct register access

#### Scenario: Multiple instances share code
- **GIVEN** UartX template implemented once
- **WHEN** instantiating Uart0, Uart1, Uart2
- **THEN** common code SHALL be shared across instances
- **AND** only address constants SHALL differ
- **AND** binary size SHALL be smaller than manual duplication

### Requirement: Zero-Overhead Template Abstraction
The system SHALL ensure template-based peripherals have zero runtime overhead compared to manual register access.

#### Scenario: Compile-time address resolution
- **GIVEN** UART template with BASE_ADDR template parameter
- **WHEN** accessing UART->BRGR register
- **THEN** compiler SHALL resolve address at compile-time
- **AND** generated assembly SHALL be identical to manual access
- **AND** no pointer indirection SHALL exist in generated code

#### Scenario: Inline register operations
- **GIVEN** GPIO template read() method
- **WHEN** compiled with -O2 optimization
- **THEN** entire method SHALL be inlined
- **AND** SHALL compile to single register read instruction
- **AND** no function call overhead SHALL exist

#### Scenario: Binary size comparison
- **GIVEN** application using template peripherals
- **WHEN** comparing binary size to manual register access
- **THEN** template version SHALL be equal or smaller size
- **AND** size increase SHALL be <5% if larger
- **AND** benefits of abstraction SHALL outweigh size cost

### Requirement: Type Aliases for Board Configuration
The system SHALL provide type aliases for each peripheral instance on a specific board for convenient access.

#### Scenario: SAME70 UART aliases
- **GIVEN** SAME70 board configuration
- **WHEN** defining peripheral aliases
- **THEN** SHALL provide Uart0, Uart1, Uart2 type aliases
- **AND** each SHALL be instantiation of UartX template
- **AND** addresses SHALL match SAME70 datasheet

#### Scenario: Board-specific pin aliases
- **GIVEN** SAME70 Xplained board
- **WHEN** defining LED pins
- **THEN** SHALL provide LedGreen, LedRed type aliases
- **AND** each SHALL be GpioPin<BASE, PIN> instantiation
- **AND** PIN numbers SHALL match board schematic

### Requirement: Non-Virtual Interface (NVI) Pattern
The system SHALL implement NVI pattern where public methods validate state and call private virtual methods for hardware operations.

#### Scenario: Public method validates before calling driver
- **GIVEN** UART interface with NVI pattern
- **WHEN** calling public read() method
- **THEN** SHALL check device is opened
- **AND** SHALL check device is acquired
- **THEN** SHALL call private drvRead() if checks pass
- **AND** SHALL return error if checks fail

#### Scenario: Driver implementation is simplified
- **GIVEN** UART driver implementing drvRead()
- **WHEN** implementing driver method
- **THEN** SHALL NOT need to check if device is opened
- **AND** SHALL NOT need to validate preconditions
- **AND** SHALL only focus on hardware operations

#### Scenario: Consistent error handling
- **GIVEN** all HAL interfaces using NVI
- **WHEN** device is not opened
- **THEN** all operations SHALL return same error code
- **AND** user SHALL get consistent behavior across peripherals
- **AND** validation code SHALL NOT be duplicated in drivers
