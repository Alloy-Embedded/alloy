# Core Circular Buffer Specification

## ADDED Requirements

### Requirement: Fixed-Size Circular Buffer
The system SHALL provide a CircularBuffer<T, N> template class with compile-time fixed size for efficient buffering in embedded systems.

#### Scenario: Buffer creation with compile-time size
- **GIVEN** CircularBuffer<uint8_t, 16> declaration
- **WHEN** buffer is instantiated
- **THEN** SHALL allocate 16-element array on stack
- **AND** no heap allocation SHALL occur
- **AND** capacity() SHALL return 16
- **AND** size() SHALL return 0 (initially empty)

#### Scenario: Buffer uses only stack memory
- **GIVEN** CircularBuffer<Data, 32> in function
- **WHEN** function is called
- **THEN** buffer SHALL be allocated on stack
- **AND** no dynamic memory allocation SHALL occur
- **AND** buffer SHALL be destroyed when function returns

### Requirement: Push and Pop Operations
The system SHALL provide push() and pop() methods for adding and removing elements from the buffer with wrap-around behavior.

#### Scenario: Push elements fills buffer
- **GIVEN** empty CircularBuffer<int, 4>
- **WHEN** pushing 1, 2, 3, 4
- **THEN** size() SHALL be 4
- **AND** full() SHALL return true
- **AND** empty() SHALL return false
- **AND** elements SHALL be stored in order

#### Scenario: Pop elements empties buffer
- **GIVEN** CircularBuffer with elements [1, 2, 3, 4]
- **WHEN** popping 4 times
- **THEN** SHALL return 1, 2, 3, 4 in order (FIFO)
- **AND** size() SHALL become 0
- **AND** empty() SHALL return true

#### Scenario: Push to full buffer overwrites oldest
- **GIVEN** full CircularBuffer<int, 4> with [1, 2, 3, 4]
- **WHEN** pushing 5
- **THEN** oldest element 1 SHALL be overwritten
- **AND** buffer SHALL contain [2, 3, 4, 5]
- **AND** size() SHALL remain 4

### Requirement: Wrap-Around Behavior
The system SHALL implement efficient wrap-around when head or tail reaches end of buffer using modulo or masking.

#### Scenario: Circular wrap-around on push
- **GIVEN** CircularBuffer<int, 4> with head at index 3
- **WHEN** pushing element
- **THEN** element SHALL be placed at index 3
- **AND** head SHALL wrap to index 0
- **AND** no array bounds violation SHALL occur

#### Scenario: Circular wrap-around on pop
- **GIVEN** CircularBuffer with tail at last index
- **WHEN** popping element
- **THEN** element SHALL be read from last index
- **AND** tail SHALL wrap to index 0
- **AND** buffer continuity SHALL be maintained

#### Scenario: Multiple wrap-arounds work correctly
- **GIVEN** CircularBuffer used for extended period
- **WHEN** head/tail wrap around multiple times
- **THEN** buffer SHALL continue working correctly
- **AND** no overflow or corruption SHALL occur
- **AND** wrap-around SHALL be transparent to user

### Requirement: State Predicates
The system SHALL provide empty(), full(), size(), and capacity() methods for querying buffer state.

#### Scenario: Empty buffer detection
- **GIVEN** newly created CircularBuffer
- **THEN** empty() SHALL return true
- **AND** size() SHALL return 0
- **AND** full() SHALL return false

#### Scenario: Full buffer detection
- **GIVEN** CircularBuffer<int, 8> with 8 elements
- **THEN** full() SHALL return true
- **AND** size() SHALL return 8
- **AND** capacity() SHALL return 8

#### Scenario: Partial fill state
- **GIVEN** CircularBuffer<int, 10> with 6 elements
- **THEN** empty() SHALL return false
- **AND** full() SHALL return false
- **AND** size() SHALL return 6
- **AND** capacity() SHALL return 10

### Requirement: Iterator Support
The system SHALL provide iterator support for range-based for loops and STL algorithm compatibility.

#### Scenario: Range-based for loop iterates elements
- **GIVEN** CircularBuffer<int, 4> with [1, 2, 3, 4]
- **WHEN** using for (auto elem : buffer)
- **THEN** SHALL iterate elements in insertion order
- **AND** SHALL visit each element exactly once
- **AND** SHALL handle wrap-around transparently

#### Scenario: STL algorithm compatibility
- **GIVEN** CircularBuffer<int, 8> with elements
- **WHEN** using std::find(buffer.begin(), buffer.end(), value)
- **THEN** algorithm SHALL work correctly
- **AND** iterators SHALL satisfy ForwardIterator requirements
- **AND** result SHALL be valid iterator or end()

#### Scenario: Iterator invalidation on modification
- **GIVEN** iterator pointing to buffer element
- **WHEN** buffer is modified (push/pop)
- **THEN** iterator MAY be invalidated (implementation-defined)
- **AND** documentation SHALL specify invalidation rules
- **AND** safe usage patterns SHALL be documented

### Requirement: Clear Operation
The system SHALL provide clear() method to reset buffer to empty state without deallocating memory.

#### Scenario: Clear resets buffer to empty
- **GIVEN** CircularBuffer with 10 elements
- **WHEN** calling clear()
- **THEN** size() SHALL return 0
- **AND** empty() SHALL return true
- **AND** capacity() SHALL remain unchanged
- **AND** head and tail SHALL be reset

#### Scenario: Clear does not deallocate memory
- **GIVEN** CircularBuffer with allocated storage
- **WHEN** calling clear()
- **THEN** underlying array SHALL remain allocated
- **AND** no memory deallocation SHALL occur
- **AND** buffer SHALL be ready for reuse

### Requirement: Type Safety and Generic Programming
The system SHALL support any copyable or movable type T and work with both POD and complex types.

#### Scenario: Buffer with primitive types
- **GIVEN** CircularBuffer<uint8_t, 256>
- **WHEN** pushing bytes
- **THEN** SHALL store bytes efficiently
- **AND** SHALL work with memcpy optimization if possible

#### Scenario: Buffer with struct types
- **GIVEN** CircularBuffer<SensorData, 16>
- **WHERE** SensorData is struct with multiple fields
- **WHEN** pushing struct instances
- **THEN** SHALL copy or move structs correctly
- **AND** SHALL call constructors/destructors as needed

#### Scenario: Buffer with move-only types
- **GIVEN** CircularBuffer<std::unique_ptr<int>, 4>
- **WHEN** pushing unique_ptr (move)
- **THEN** move semantics SHALL be used
- **AND** no copy SHALL occur
- **AND** ownership SHALL be transferred to buffer

### Requirement: Performance Characteristics
The system SHALL provide O(1) push, pop, and access operations with predictable performance.

#### Scenario: Push operation is constant time
- **GIVEN** CircularBuffer of any size
- **WHEN** pushing element
- **THEN** operation SHALL be O(1)
- **AND** no loops over buffer SHALL occur
- **AND** execution time SHALL be predictable

#### Scenario: Pop operation is constant time
- **GIVEN** CircularBuffer of any size
- **WHEN** popping element
- **THEN** operation SHALL be O(1)
- **AND** no data shifting SHALL occur
- **AND** execution time SHALL be predictable

#### Scenario: Memory layout is cache-friendly
- **GIVEN** CircularBuffer storage
- **THEN** elements SHALL be contiguous in memory
- **AND** cache locality SHALL be good for sequential access
- **AND** performance SHALL be better than linked list

### Requirement: Error Handling
The system SHALL provide safe error handling for underflow (pop from empty) and overflow (push to full) conditions.

#### Scenario: Pop from empty buffer returns error
- **GIVEN** empty CircularBuffer
- **WHEN** calling pop()
- **THEN** SHALL return Result<T> with error
- **AND** error code SHALL indicate buffer empty
- **AND** buffer state SHALL remain unchanged

#### Scenario: Push to full buffer behavior is configurable
- **GIVEN** full CircularBuffer
- **WHEN** calling push()
- **THEN** SHALL either:
  - Overwrite oldest element (ring buffer behavior), OR
  - Return error without modifying buffer (queue behavior)
- **AND** behavior SHALL be documented
- **AND** MAY be template parameter or runtime option

#### Scenario: Bounds checking in debug builds
- **GIVEN** CircularBuffer in debug mode
- **WHEN** accessing out of bounds
- **THEN** SHALL assert or throw error
- **AND** SHALL help catch bugs during development
- **AND** release builds MAY skip checks for performance

### Requirement: Common Use Cases
The system SHALL be optimized for common embedded use cases including UART buffers, DMA, and event queues.

#### Scenario: UART RX buffering
- **GIVEN** CircularBuffer<uint8_t, 256> for UART RX
- **WHEN** interrupt handler pushes received bytes
- **AND** main loop pops bytes for processing
- **THEN** SHALL handle continuous data stream
- **AND** no data loss if buffer not full
- **AND** overflow handling SHALL be clear

#### Scenario: DMA transfer buffering
- **GIVEN** CircularBuffer<Sample, 512> for ADC DMA
- **WHEN** DMA writes samples to buffer
- **AND** processing task reads samples
- **THEN** SHALL support producer-consumer pattern
- **AND** SHALL work with interrupt-driven updates
- **AND** SHALL avoid race conditions with proper usage

#### Scenario: Event queue
- **GIVEN** CircularBuffer<Event, 32> for event system
- **WHEN** events are pushed from various sources
- **AND** event loop pops and processes events
- **THEN** SHALL maintain FIFO ordering
- **AND** SHALL handle burst of events
- **AND** full buffer behavior SHALL be defined

### Requirement: Documentation and Examples
The system SHALL provide comprehensive documentation with usage examples for common patterns.

#### Scenario: Documentation includes UART example
- **GIVEN** developer implementing UART buffering
- **WHEN** reading CircularBuffer documentation
- **THEN** SHALL find complete UART RX/TX example
- **AND** SHALL show interrupt handler integration
- **AND** SHALL show non-blocking read/write patterns

#### Scenario: Documentation covers thread safety
- **GIVEN** multi-threaded or interrupt-driven usage
- **WHEN** consulting documentation
- **THEN** SHALL explain thread safety guarantees (or lack thereof)
- **AND** SHALL show how to add synchronization if needed
- **AND** SHALL document single-producer-single-consumer patterns

#### Scenario: Performance guidelines provided
- **GIVEN** developer choosing buffer size
- **WHEN** reading documentation
- **THEN** SHALL find guidelines for size selection
- **AND** SHALL explain trade-offs (memory vs overflow risk)
- **AND** SHALL provide sizing formulas for common cases
