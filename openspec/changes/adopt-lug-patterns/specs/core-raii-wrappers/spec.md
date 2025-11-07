# Core RAII Wrappers Specification

## ADDED Requirements

### Requirement: ScopedDevice Automatic Resource Management
The system SHALL provide ScopedDevice<T> template class that automatically acquires device on construction and releases on destruction.

#### Scenario: Device acquired on construction
- **GIVEN** ScopedDevice<IUart> constructor with device ID
- **WHEN** ScopedDevice is constructed
- **THEN** device SHALL be retrieved from registry
- **AND** device SHALL be acquired (reference count incremented)
- **AND** ScopedDevice SHALL hold shared_ptr to device

#### Scenario: Device released on destruction
- **GIVEN** ScopedDevice<IUart> variable in scope
- **WHEN** variable goes out of scope
- **THEN** destructor SHALL be called automatically
- **AND** device SHALL be returned to registry
- **AND** reference count SHALL be decremented
- **AND** device SHALL be available for reuse

#### Scenario: Early return releases device
- **GIVEN** function using ScopedDevice<IUart>
- **WHEN** function returns early due to error
- **THEN** ScopedDevice destructor SHALL still execute
- **AND** device SHALL be properly released
- **AND** no resource leak SHALL occur

### Requirement: ScopedDevice Move Semantics
The system SHALL support move semantics for ScopedDevice to enable efficient ownership transfer without affecting reference counts.

#### Scenario: Move construction transfers ownership
- **GIVEN** ScopedDevice<ISpi> spi1 holding device
- **WHEN** constructing ScopedDevice<ISpi> spi2(std::move(spi1))
- **THEN** spi2 SHALL take ownership of device
- **AND** spi1 SHALL be null/empty
- **AND** no reference count change SHALL occur
- **AND** device SHALL be released only when spi2 is destroyed

#### Scenario: Move assignment transfers ownership
- **GIVEN** ScopedDevice<II2c> holding device A
- **WHEN** assigning another ScopedDevice holding device B
- **THEN** device A SHALL be released first
- **AND** ownership of device B SHALL be transferred
- **AND** reference counts SHALL be updated correctly

#### Scenario: Copy construction is deleted
- **GIVEN** ScopedDevice<IUart> uart
- **WHEN** attempting to copy ScopedDevice
- **THEN** compilation SHALL fail
- **AND** copy constructor SHALL be explicitly deleted
- **AND** error message SHALL be clear

### Requirement: ScopedDevice Access Operators
The system SHALL provide pointer-like access operators for convenient usage of wrapped device.

#### Scenario: Arrow operator accesses device methods
- **GIVEN** ScopedDevice<IUart> uart
- **WHEN** calling uart->write(data, size)
- **THEN** SHALL forward call to underlying IUart device
- **AND** SHALL work like pointer dereference
- **AND** SHALL be intuitive for C++ developers

#### Scenario: Dereference operator accesses device
- **GIVEN** ScopedDevice<IGpio> led
- **WHEN** accessing (*led).write(true)
- **THEN** SHALL return reference to underlying device
- **AND** SHALL allow all device operations

#### Scenario: Null pointer safety
- **GIVEN** ScopedDevice that failed to acquire device
- **WHEN** attempting to use arrow or dereference operator
- **THEN** SHALL either be null (safe to check) or assert in debug
- **AND** SHALL NOT cause undefined behavior if device is null

### Requirement: ScopedI2c Bus Locking
The system SHALL provide ScopedI2c class that locks I2C bus on construction and unlocks on destruction, preventing concurrent access.

#### Scenario: Bus locked on construction
- **GIVEN** ScopedI2c constructor with I2C device and timeout
- **WHEN** ScopedI2c is constructed
- **THEN** I2C bus SHALL be locked for exclusive access
- **AND** other threads/tasks SHALL wait if they try to lock
- **AND** SHALL timeout if lock cannot be acquired

#### Scenario: Bus unlocked on destruction
- **GIVEN** ScopedI2c variable holding locked bus
- **WHEN** variable goes out of scope
- **THEN** I2C bus SHALL be unlocked automatically
- **AND** waiting threads/tasks SHALL be able to acquire bus
- **AND** lock SHALL be released even on early return

#### Scenario: Multiple I2C transactions in one lock
- **GIVEN** ScopedI2c bus holding lock
- **WHEN** performing multiple I2C read/write operations
- **THEN** all operations SHALL use locked bus
- **AND** no other code SHALL interrupt I2C communication
- **AND** atomic multi-register access SHALL be guaranteed

### Requirement: ScopedSpi Bus Locking
The system SHALL provide ScopedSpi class similar to ScopedI2c for exclusive SPI bus access.

#### Scenario: SPI bus locked on construction
- **GIVEN** ScopedSpi constructor with SPI device and timeout
- **WHEN** ScopedSpi is constructed
- **THEN** SPI bus SHALL be locked
- **AND** chip select management SHALL be handled
- **AND** bus configuration SHALL be set

#### Scenario: SPI bus unlocked on destruction
- **GIVEN** ScopedSpi holding locked bus
- **WHEN** going out of scope
- **THEN** SPI bus SHALL be unlocked
- **AND** chip select SHALL be deasserted
- **AND** next device can acquire bus

#### Scenario: SPI multi-byte transaction atomicity
- **GIVEN** ScopedSpi for flash memory device
- **WHEN** performing command + address + data transfer
- **THEN** entire sequence SHALL be atomic
- **AND** no other SPI device SHALL interrupt
- **AND** chip select SHALL remain asserted

### Requirement: Exception Safety
The system SHALL ensure RAII wrappers are exception-safe, properly releasing resources even when exceptions occur (if exceptions are enabled).

#### Scenario: Exception in scoped block releases device
- **GIVEN** ScopedDevice<IUart> in try block
- **WHEN** exception is thrown before end of block
- **THEN** stack unwinding SHALL call destructors
- **AND** ScopedDevice destructor SHALL release device
- **AND** no resource leak SHALL occur

#### Scenario: No exceptions configuration works
- **GIVEN** CoreZero compiled with -fno-exceptions
- **WHEN** using RAII wrappers
- **THEN** SHALL still work correctly
- **AND** cleanup SHALL rely on scope exit, not exception handling
- **AND** early returns SHALL still trigger destructors

### Requirement: Nested Scopes and Locking
The system SHALL support nested scopes with multiple RAII wrappers, releasing resources in reverse order of acquisition.

#### Scenario: Nested UART and I2C usage
- **GIVEN** outer scope with ScopedDevice<IUart>
- **AND** inner scope with ScopedDevice<II2c>
- **WHEN** inner scope exits
- **THEN** I2C SHALL be released first
- **WHEN** outer scope exits
- **THEN** UART SHALL be released second
- **AND** order SHALL follow C++ stack unwinding rules

#### Scenario: Multiple bus locks (deadlock prevention)
- **GIVEN** code that needs both I2C0 and I2C1
- **WHEN** acquiring locks in consistent order
- **THEN** SHALL avoid deadlock
- **AND** documentation SHALL recommend lock ordering
- **AND** debug builds MAY detect inconsistent ordering

### Requirement: Performance Overhead
The system SHALL implement RAII wrappers with minimal performance overhead, compiling to same code as manual cleanup.

#### Scenario: Destructor is inlined
- **GIVEN** ScopedDevice<T> destructor
- **WHEN** compiled with optimization
- **THEN** destructor call SHALL be inlined
- **AND** SHALL compile to direct release() call
- **AND** no virtual function overhead SHALL exist

#### Scenario: Zero size overhead for move-only wrapper
- **GIVEN** ScopedDevice<T> with move semantics
- **WHEN** measuring sizeof(ScopedDevice<T>)
- **THEN** SHALL be same as sizeof(shared_ptr<T>)
- **AND** no additional overhead SHALL exist

### Requirement: Documentation and Examples
The system SHALL provide comprehensive documentation with examples demonstrating RAII patterns and best practices.

#### Scenario: Documentation includes common patterns
- **GIVEN** developer learning RAII wrappers
- **WHEN** reading documentation
- **THEN** SHALL find examples of ScopedDevice usage
- **AND** SHALL find I2C sensor reading with ScopedI2c
- **AND** SHALL find SPI flash access with ScopedSpi

#### Scenario: Migration guide shows before/after
- **GIVEN** code using manual acquire/release
- **WHEN** consulting migration guide
- **THEN** SHALL find side-by-side comparison
- **AND** SHALL see how RAII simplifies code
- **AND** SHALL understand benefits of automatic cleanup

#### Scenario: Error handling patterns documented
- **GIVEN** developer handling errors with RAII
- **WHEN** reading documentation
- **THEN** SHALL find examples of early return with cleanup
- **AND** SHALL find examples of nested error handling
- **AND** SHALL find examples of error propagation
