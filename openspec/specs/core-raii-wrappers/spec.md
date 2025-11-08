# Spec: core-raii-wrappers

## Overview
RAII (Resource Acquisition Is Initialization) wrappers for automatic resource management in embedded systems, preventing resource leaks through guaranteed cleanup.

## Requirements

### Functional Requirements

**REQ-RW-001**: The system SHALL provide a generic RAII device wrapper `ScopedDevice<T>`
- **Rationale**: Enables automatic resource cleanup for any device type
- **Implementation**: Template wrapper with factory method pattern

**REQ-RW-002**: The system SHALL provide I2C bus locking wrapper `ScopedI2c<T>`
- **Rationale**: Prevents race conditions on shared I2C bus
- **Implementation**: RAII wrapper with automatic lock/unlock

**REQ-RW-003**: The system SHALL provide SPI bus locking wrapper `ScopedSpi<T, CS>`
- **Rationale**: Prevents race conditions on shared SPI bus with chip select management
- **Implementation**: RAII wrapper with automatic CS activation/deactivation

**REQ-RW-004**: The system SHALL enforce non-copyable semantics for bus locks
- **Rationale**: Bus locks cannot be duplicated to maintain exclusivity
- **Implementation**: Deleted copy constructor and copy assignment

**REQ-RW-005**: The system SHALL support move semantics for device wrappers
- **Rationale**: Enables efficient transfer of ownership
- **Implementation**: Move constructor and move assignment for ScopedDevice

**REQ-RW-006**: The system SHALL provide pointer and reference access operators
- **Rationale**: Transparent access to underlying device
- **Implementation**: operator->, operator*, get()

**REQ-RW-007**: The system SHALL return Result<T, ErrorCode> for creation
- **Rationale**: Type-safe error handling without exceptions
- **Implementation**: Factory method returning Result

### Non-Functional Requirements

**REQ-RW-NF-001**: The implementation SHALL have zero overhead compared to manual lock/unlock
- **Rationale**: RAII should not add runtime cost
- **Implementation**: Fully inlined methods, compiler optimization

**REQ-RW-NF-002**: The implementation SHALL work without exceptions
- **Rationale**: Embedded systems often disable exceptions
- **Implementation**: Result-based error handling

**REQ-RW-NF-003**: The implementation SHALL guarantee cleanup on all exit paths
- **Rationale**: Critical for embedded resource management
- **Implementation**: RAII destructor pattern

## Implementation

### Files
- `src/core/scoped_device.hpp` - Generic device wrapper
- `src/core/scoped_i2c.hpp` - I2C bus locking
- `src/core/scoped_spi.hpp` - SPI bus locking
- `tests/unit/test_scoped_device.cpp` - Unit tests (27 tests)

### API Surface
```cpp
// Generic device wrapper
template <typename Device>
class ScopedDevice {
public:
    static Result<ScopedDevice<Device>, ErrorCode> create(Device& device);
    ~ScopedDevice();

    Device* operator->() noexcept;
    Device& operator*() noexcept;
    Device* get() noexcept;
};

// I2C bus locking
template <typename I2cDevice>
class ScopedI2c {
public:
    static Result<ScopedI2c<I2cDevice>, ErrorCode> create(
        I2cDevice& device,
        uint32_t timeout_ms = 100
    );
    ~ScopedI2c();

    // Convenience methods
    Result<size_t, ErrorCode> write(uint8_t addr, const uint8_t* data, size_t size);
    Result<size_t, ErrorCode> read(uint8_t addr, uint8_t* data, size_t size);
    Result<void, ErrorCode> writeRegister(uint8_t addr, uint8_t reg, uint8_t value);
    Result<void, ErrorCode> readRegister(uint8_t addr, uint8_t reg, uint8_t* value);
};

// SPI bus locking
template <typename SpiDevice, typename ChipSelect>
class ScopedSpi {
public:
    static Result<ScopedSpi<SpiDevice, ChipSelect>, ErrorCode> create(
        SpiDevice& device,
        ChipSelect cs,
        uint32_t timeout_ms = 100
    );
    ~ScopedSpi();

    // Convenience methods
    Result<size_t, ErrorCode> transfer(const uint8_t* tx, uint8_t* rx, size_t size);
    Result<size_t, ErrorCode> write(const uint8_t* data, size_t size);
    Result<size_t, ErrorCode> read(uint8_t* data, size_t size);
    Result<void, ErrorCode> writeByte(uint8_t byte);
    Result<uint8_t, ErrorCode> readByte();
};
```

### Usage Examples
```cpp
// I2C sensor reading with automatic bus locking
auto result = ScopedI2c::create(i2c_bus);
if (result.is_ok()) {
    auto scoped = std::move(result).unwrap();
    scoped->write(sensor_addr, cmd, 1);
    scoped->read(sensor_addr, data, 6);
}  // Bus automatically unlocked

// SPI flash access with CS management
auto result = ScopedSpi::create(spi_bus, ChipSelect::CS0);
if (result.is_ok()) {
    auto scoped = std::move(result).unwrap();
    scoped.transfer(tx_data, rx_data, 256);
}  // CS deactivated, bus unlocked

// Helper functions with type deduction
auto scoped_i2c = makeScopedI2c(i2c_bus, 200);  // 200ms timeout
auto scoped_spi = makeScopedSpi(spi_bus, ChipSelect::CS1);
```

## Testing
- 27 unit tests covering all wrappers
- Tests include: creation, access, RAII guarantees, error handling
- Mock devices for isolated testing

## Dependencies
- `core/error.hpp` - Error codes
- `core/result.hpp` - Result<T, E> type
- `<utility>` - Move semantics

## References
- LUG Framework RAII patterns
- Implementation: `src/core/scoped_*.hpp`
