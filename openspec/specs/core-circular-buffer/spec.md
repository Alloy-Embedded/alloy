# Spec: core-circular-buffer

## Purpose
Provides a lock-free circular buffer (ring buffer) implementation for embedded systems with zero overhead abstractions, enabling efficient FIFO buffering for UART, SPI, DMA, and sensor data.

## Requirements

### Requirement: Fixed-size circular buffer template
The system SHALL provide a fixed-size circular buffer template `CircularBuffer<T, N>`

#### Scenario: Creating a buffer
- **WHEN** User instantiates `CircularBuffer<uint8_t, 256>`
- **THEN** Buffer is created with stack-allocated storage for 255 usable elements

**REQ-CB-002**: The system SHALL support O(1) push and pop operations
- **Rationale**: Constant-time operations are essential for real-time systems
- **Implementation**: Head and tail index tracking with modulo arithmetic

**REQ-CB-003**: The system SHALL detect full and empty states
- **Rationale**: Prevents buffer overflows and underflows
- **Implementation**: Reserve one slot to distinguish full from empty (capacity N-1)

**REQ-CB-004**: The system SHALL support bulk operations (push_bulk, pop_bulk, peek_bulk)
- **Rationale**: Efficient multi-element transfers for DMA and buffering
- **Implementation**: Batch operations with single size calculation

**REQ-CB-005**: The system SHALL provide overwrite mode for circular logging
- **Rationale**: Newest data takes priority in fixed-size logs
- **Implementation**: push_overwrite() advances tail when full

**REQ-CB-006**: The system SHALL support iterator-based access
- **Rationale**: Enables range-based for loops and STL algorithms
- **Implementation**: Forward iterators with proper invalidation semantics

**REQ-CB-007**: The system SHALL provide optional atomic operations for thread-safety
- **Rationale**: Lock-free single producer/single consumer patterns
- **Implementation**: Template parameter for std::atomic indices

### Non-Functional Requirements

**REQ-CB-NF-001**: The implementation SHALL have zero runtime overhead compared to manual ring buffer
- **Rationale**: Embedded systems require minimal abstraction cost
- **Implementation**: Fully inlined methods, no virtual functions

**REQ-CB-NF-002**: The implementation SHALL not use dynamic allocation
- **Rationale**: Embedded systems often disable heap allocation
- **Implementation**: Stack-allocated std::array storage

**REQ-CB-NF-003**: The implementation SHALL support move-only types
- **Rationale**: Enables efficient transfer of non-copyable resources
- **Implementation**: Move semantics throughout API

## Implementation

### Files
- `src/core/circular_buffer.hpp` - Main implementation
- `tests/unit/test_circular_buffer.cpp` - Unit tests (33 tests)

### API Surface
```cpp
template <typename T, size_t N, bool Atomic = false>
class CircularBuffer {
public:
    // Basic operations
    Result<void, ErrorCode> push(const T& value);
    Result<void, ErrorCode> push(T&& value);
    Result<T, ErrorCode> pop();

    // State queries
    bool empty() const;
    bool full() const;
    size_t size() const;
    size_t available() const;

    // Peek operations
    const T* peek() const;
    T* peek();

    // Bulk operations
    size_t peek_bulk(T* dest, size_t count) const;
    size_t pop_bulk(T* dest, size_t count);
    size_t push_bulk(const T* src, size_t count);

    // Overwrite mode
    void push_overwrite(const T& value);
    void push_overwrite(T&& value);

    // Maintenance
    void clear();

    // Iterators
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
};
```

### Usage Examples
```cpp
// UART RX buffering
CircularBuffer<uint8_t, 256> uart_rx;
uart_rx.push(byte);  // In ISR
auto result = uart_rx.pop();  // In main loop

// Thread-safe queue
CircularBuffer<int, 1024, true> atomic_queue;  // With atomic indices

// Circular logging
CircularBuffer<LogEntry, 100> log_buffer;
log_buffer.push_overwrite(entry);  // Overwrites oldest when full

// Bulk transfers
uint8_t data[128];
size_t count = buffer.pop_bulk(data, 128);
```

## Testing
- 33 unit tests covering all operations
- 100% test pass rate
- Coverage includes: basic ops, wraparound, bulk ops, overwrite, iterators, edge cases

## Dependencies
- `core/error.hpp` - Error codes
- `core/result.hpp` - Result<T, E> type
- `<array>` - Fixed-size storage
- `<atomic>` - Optional thread-safety

## References
- LUG Framework ring buffer pattern
- Implementation: `src/core/circular_buffer.hpp`
