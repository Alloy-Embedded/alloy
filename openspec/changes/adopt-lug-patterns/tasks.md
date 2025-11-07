# Implementation Tasks: Adopt LUG Framework Patterns

## Phase 1: Core Infrastructure (Week 1-2)

### 1. Result<T> Type Implementation
- [ ] 1.1 Create `src/core/result.hpp` header
- [ ] 1.2 Implement Result<T> template class
  - [ ] 1.2.1 Basic construction (value, error)
  - [ ] 1.2.2 Boolean conversion operator
  - [ ] 1.2.3 Value accessors (value(), operator*)
  - [ ] 1.2.4 Error accessor (error())
  - [ ] 1.2.5 Structured binding support (get<I>())
- [ ] 1.3 Add std::tuple_size and std::tuple_element specializations
- [ ] 1.4 Create unit tests (`tests/core/test_result.cpp`)
  - [ ] 1.4.1 Test construction with value
  - [ ] 1.4.2 Test construction with error
  - [ ] 1.4.3 Test boolean conversion
  - [ ] 1.4.4 Test value access
  - [ ] 1.4.5 Test error access
  - [ ] 1.4.6 Test structured binding
  - [ ] 1.4.7 Test move semantics
- [ ] 1.5 Write documentation (`docs/RESULT_TYPE.md`)
  - [ ] 1.5.1 API reference
  - [ ] 1.5.2 Usage examples
  - [ ] 1.5.3 Best practices
  - [ ] 1.5.4 Common patterns

### 2. Device Management Implementation
- [ ] 2.1 Create `src/core/device.hpp` header
- [ ] 2.2 Implement SharingPolicy enum
- [ ] 2.3 Implement Device base class
  - [ ] 2.3.1 Constructor with SharingPolicy
  - [ ] 2.3.2 acquire() method with reference counting
  - [ ] 2.3.3 release() method with reference counting
  - [ ] 2.3.4 State tracking (m_opened, m_owners_count)
  - [ ] 2.3.5 Accessors (isOpened(), ownersCount())
- [ ] 2.4 Create `src/core/device_registry.hpp` header
- [ ] 2.5 Implement GlobalRegistry template
  - [ ] 2.5.1 Static map for instances
  - [ ] 2.5.2 init() method
  - [ ] 2.5.3 get() method with type safety
  - [ ] 2.5.4 Error handling for missing devices
- [ ] 2.6 Create board-specific device IDs
  - [ ] 2.6.1 `boards/atmel_same70_xpld/device_ids.hpp`
  - [ ] 2.6.2 Define Same70DeviceId enum
  - [ ] 2.6.3 Add all peripherals (UART, GPIO, I2C, SPI, etc.)
- [ ] 2.7 Implement device factory functions
  - [ ] 2.7.1 Template getDevice<T>(DeviceId)
  - [ ] 2.7.2 returnDevice() for RAII
- [ ] 2.8 Create unit tests (`tests/core/test_device.cpp`)
  - [ ] 2.8.1 Test eSingle policy (exclusive access)
  - [ ] 2.8.2 Test eShared policy (shared access)
  - [ ] 2.8.3 Test reference counting
  - [ ] 2.8.4 Test acquire/release
  - [ ] 2.8.5 Test error on busy device
- [ ] 2.9 Create unit tests (`tests/core/test_device_registry.cpp`)
  - [ ] 2.9.1 Test registry initialization
  - [ ] 2.9.2 Test device retrieval
  - [ ] 2.9.3 Test missing device error
  - [ ] 2.9.4 Test type safety
- [ ] 2.10 Write documentation (`docs/DEVICE_MANAGEMENT.md`)
  - [ ] 2.10.1 SharingPolicy guide
  - [ ] 2.10.2 Device ownership patterns
  - [ ] 2.10.3 Registry usage examples
  - [ ] 2.10.4 Board configuration guide

### 3. Error Code Standardization
- [ ] 3.1 Create `src/core/error_codes.hpp`
- [ ] 3.2 Define CoreZero error enum
  - [ ] 3.2.1 Common errors (InvalidArgument, NotOpened, etc.)
  - [ ] 3.2.2 Hardware errors (Timeout, BusError, etc.)
  - [ ] 3.2.3 Resource errors (DeviceBusy, NotAvailable, etc.)
- [ ] 3.3 Implement std::error_code integration
  - [ ] 3.3.1 Error category class
  - [ ] 3.3.2 make_error_code() function
  - [ ] 3.3.3 is_error_code_enum specialization
- [ ] 3.4 Create unit tests (`tests/core/test_error_codes.cpp`)
- [ ] 3.5 Update documentation

## Phase 2: HAL Updates (Week 3-4)

### 4. Template-Based UART Implementation
- [ ] 4.1 Update `src/hal/interfaces/uart_interface.hpp`
  - [ ] 4.1.1 Inherit from Device base class
  - [ ] 4.1.2 Add Result<T> return types
  - [ ] 4.1.3 Implement NVI pattern (public/private virtual)
  - [ ] 4.1.4 Keep legacy API with [[deprecated]]
- [ ] 4.2 Create template implementation `src/hal/vendors/atmel/same70/uart.hpp`
  - [ ] 4.2.1 Template parameters (BASE_ADDR, IRQ_ID)
  - [ ] 4.2.2 Implement drvOpen() with Result<void>
  - [ ] 4.2.3 Implement drvClose() with Result<void>
  - [ ] 4.2.4 Implement drvWrite() with Result<size_t>
  - [ ] 4.2.5 Implement drvRead() with Result<uint8_t>
  - [ ] 4.2.6 Implement drvAvailable() with Result<size_t>
- [ ] 4.3 Create type aliases (Uart0, Uart1, Uart2)
- [ ] 4.4 Update board configuration
  - [ ] 4.4.1 Register UART instances in device registry
- [ ] 4.5 Create unit tests (`tests/hal/test_uart_template.cpp`)
- [ ] 4.6 Create integration tests (`tests/integration/test_uart_result.cpp`)
- [ ] 4.7 Update examples
  - [ ] 4.7.1 Update `examples/uart_echo/` to use Result<T>
  - [ ] 4.7.2 Add example of structured binding

### 5. Template-Based GPIO Implementation
- [ ] 5.1 Update `src/hal/interfaces/gpio_interface.hpp`
  - [ ] 5.1.1 Inherit from Device base class
  - [ ] 5.1.2 Add Result<T> return types
  - [ ] 5.1.3 Implement NVI pattern
  - [ ] 5.1.4 Keep legacy API with [[deprecated]]
- [ ] 5.2 Create template implementation `src/hal/vendors/atmel/same70/gpio.hpp`
  - [ ] 5.2.1 Template parameters (BASE_ADDR, PIN)
  - [ ] 5.2.2 Implement setMode() with Result<void>
  - [ ] 5.2.3 Implement write() with Result<void>
  - [ ] 5.2.4 Implement read() with Result<bool>
  - [ ] 5.2.5 Implement toggle() with Result<void>
- [ ] 5.3 Create type aliases for board pins
- [ ] 5.4 Update board configuration
- [ ] 5.5 Create unit tests
- [ ] 5.6 Update examples
  - [ ] 5.6.1 Update `examples/same70_blink/` (optional, show new API)

### 6. Template-Based I2C Implementation
- [ ] 6.1 Update `src/hal/interfaces/i2c_interface.hpp`
  - [ ] 6.1.1 Inherit from Device base class
  - [ ] 6.1.2 Add Result<T> return types
  - [ ] 6.1.3 Implement NVI pattern
  - [ ] 6.1.4 Add bus locking methods
- [ ] 6.2 Create template implementation `src/hal/vendors/atmel/same70/i2c.hpp`
  - [ ] 6.2.1 Template parameters (BASE_ADDR, IRQ_ID)
  - [ ] 6.2.2 Implement drvOpen() with Result<void>
  - [ ] 6.2.3 Implement drvWrite() with Result<size_t>
  - [ ] 6.2.4 Implement drvRead() with Result<size_t>
  - [ ] 6.2.5 Implement bus locking (acquire/release)
- [ ] 6.3 Create type aliases (I2c0, I2c1)
- [ ] 6.4 Update board configuration
- [ ] 6.5 Create unit tests
- [ ] 6.6 Create integration tests with sensor example

### 7. Template-Based SPI Implementation
- [ ] 7.1 Update `src/hal/interfaces/spi_interface.hpp`
  - [ ] 7.1.1 Inherit from Device base class
  - [ ] 7.1.2 Add Result<T> return types
  - [ ] 7.1.3 Implement NVI pattern
  - [ ] 7.1.4 Add bus locking methods
- [ ] 7.2 Create template implementation `src/hal/vendors/atmel/same70/spi.hpp`
  - [ ] 7.2.1 Template parameters (BASE_ADDR, IRQ_ID)
  - [ ] 7.2.2 Implement drvOpen() with Result<void>
  - [ ] 7.2.3 Implement drvTransfer() with Result<size_t>
  - [ ] 7.2.4 Implement bus locking
- [ ] 7.3 Create type aliases (Spi0, Spi1)
- [ ] 7.4 Update board configuration
- [ ] 7.5 Create unit tests
- [ ] 7.6 Create integration tests

### 8. RAII Wrappers Implementation
- [ ] 8.1 Create `src/core/scoped_device.hpp` header
- [ ] 8.2 Implement ScopedDevice<T> template
  - [ ] 8.2.1 Constructor (acquires device)
  - [ ] 8.2.2 Destructor (releases device)
  - [ ] 8.2.3 Deleted copy constructor/assignment
  - [ ] 8.2.4 Move constructor/assignment
  - [ ] 8.2.5 operator-> and operator*
- [ ] 8.3 Implement ScopedI2c class
  - [ ] 8.3.1 Constructor (locks bus with timeout)
  - [ ] 8.3.2 Destructor (unlocks bus)
  - [ ] 8.3.3 get() accessor
  - [ ] 8.3.4 Move semantics
- [ ] 8.4 Implement ScopedSpi class (similar to ScopedI2c)
- [ ] 8.5 Create unit tests (`tests/core/test_scoped_device.cpp`)
  - [ ] 8.5.1 Test automatic cleanup
  - [ ] 8.5.2 Test early return scenarios
  - [ ] 8.5.3 Test move semantics
  - [ ] 8.5.4 Test nested scopes
- [ ] 8.6 Create unit tests (`tests/core/test_scoped_i2c.cpp`)
  - [ ] 8.6.1 Test bus locking
  - [ ] 8.6.2 Test timeout behavior
  - [ ] 8.6.3 Test automatic unlock
- [ ] 8.7 Write documentation (`docs/RAII_WRAPPERS.md`)
  - [ ] 8.7.1 ScopedDevice usage guide
  - [ ] 8.7.2 ScopedI2c/ScopedSpi patterns
  - [ ] 8.7.3 Best practices
  - [ ] 8.7.4 Common pitfalls

## Phase 3: Memory Optimization (Week 5-6)

### 9. Circular Buffer Implementation
- [ ] 9.1 Create `src/core/circular_buffer.hpp` header
- [ ] 9.2 Implement CircularBuffer<T, N> template
  - [ ] 9.2.1 Fixed-size array storage
  - [ ] 9.2.2 Head/tail/count tracking
  - [ ] 9.2.3 push() method (with optional overwrite)
  - [ ] 9.2.4 pop() method (with Result<T> for error)
  - [ ] 9.2.5 empty() and full() predicates
  - [ ] 9.2.6 size() and capacity() accessors
  - [ ] 9.2.7 clear() method
- [ ] 9.3 Add iterator support
  - [ ] 9.3.1 Iterator class implementation
  - [ ] 9.3.2 begin() and end() methods
  - [ ] 9.3.3 Range-based for loop support
- [ ] 9.4 Create unit tests (`tests/core/test_circular_buffer.cpp`)
  - [ ] 9.4.1 Test push/pop operations
  - [ ] 9.4.2 Test wrap-around behavior
  - [ ] 9.4.3 Test full buffer behavior
  - [ ] 9.4.4 Test empty buffer behavior
  - [ ] 9.4.5 Test iterator behavior
  - [ ] 9.4.6 Test with different types (uint8_t, uint16_t, struct)
- [ ] 9.5 Create performance benchmarks
  - [ ] 9.5.1 Compare vs std::vector
  - [ ] 9.5.2 Compare vs std::deque
  - [ ] 9.5.3 Memory usage analysis
- [ ] 9.6 Write documentation (`docs/CIRCULAR_BUFFER.md`)
  - [ ] 9.6.1 API reference
  - [ ] 9.6.2 Usage patterns
  - [ ] 9.6.3 Size selection guide
  - [ ] 9.6.4 Performance characteristics

### 10. UART Buffering with Circular Buffer
- [ ] 10.1 Add CircularBuffer to UART implementation
  - [ ] 10.1.1 RX buffer (interrupt-driven)
  - [ ] 10.1.2 TX buffer (interrupt-driven)
- [ ] 10.2 Update UART interrupt handlers
  - [ ] 10.2.1 RX interrupt pushes to buffer
  - [ ] 10.2.2 TX interrupt pops from buffer
- [ ] 10.3 Update UART API
  - [ ] 10.3.1 Non-blocking read (from buffer)
  - [ ] 10.3.2 Non-blocking write (to buffer)
  - [ ] 10.3.3 available() returns buffer size
- [ ] 10.4 Create integration tests
- [ ] 10.5 Update examples with buffered UART

## Phase 4: Documentation & Examples (Week 7-8)

### 11. Core Documentation
- [ ] 11.1 Write `docs/PATTERNS.md` (Pattern Overview)
  - [ ] 11.1.1 Result<T> pattern section
  - [ ] 11.1.2 Device Management pattern section
  - [ ] 11.1.3 RAII pattern section
  - [ ] 11.1.4 Template peripherals pattern section
  - [ ] 11.1.5 When to use each pattern
  - [ ] 11.1.6 Pattern combinations
- [ ] 11.2 Write `docs/MIGRATION.md` (Migration Guide)
  - [ ] 11.2.1 Breaking changes summary
  - [ ] 11.2.2 Before/after examples for each API
  - [ ] 11.2.3 Common migration patterns
  - [ ] 11.2.4 Error handling migration
  - [ ] 11.2.5 Resource management migration
  - [ ] 11.2.6 Troubleshooting section
- [ ] 11.3 Update `README.md`
  - [ ] 11.3.1 Add new features section
  - [ ] 11.3.2 Link to pattern guide
  - [ ] 11.3.3 Quick start with new patterns
- [ ] 11.4 Create API reference documentation
  - [ ] 11.4.1 Doxygen comments for all public APIs
  - [ ] 11.4.2 Generate HTML documentation
  - [ ] 11.4.3 Host documentation online

### 12. Example Updates
- [ ] 12.1 Update `examples/same70_blink/`
  - [ ] 12.1.1 Add variant using ScopedDevice
  - [ ] 12.1.2 Show error handling with Result<T>
  - [ ] 12.1.3 Add comprehensive comments
- [ ] 12.2 Update `examples/uart_echo/`
  - [ ] 12.2.1 Use Result<T> for all UART operations
  - [ ] 12.2.2 Use ScopedDevice for UART
  - [ ] 12.2.3 Demonstrate structured binding
  - [ ] 12.2.4 Show error handling patterns
- [ ] 12.3 Create `examples/i2c_sensor/`
  - [ ] 12.3.1 Basic I2C sensor reading
  - [ ] 12.3.2 Use ScopedI2c for bus locking
  - [ ] 12.3.3 Use Result<T> for error handling
  - [ ] 12.3.4 Show multiple sensors on same bus
- [ ] 12.4 Create `examples/spi_flash/`
  - [ ] 12.4.1 SPI flash memory operations
  - [ ] 12.4.2 Use ScopedSpi for bus locking
  - [ ] 12.4.3 Demonstrate circular buffer for DMA
- [ ] 12.5 Create `examples/resource_management/`
  - [ ] 12.5.1 Show device conflicts (Single policy)
  - [ ] 12.5.2 Show device sharing (Shared policy)
  - [ ] 12.5.3 Demonstrate RAII cleanup
  - [ ] 12.5.4 Show error recovery patterns

### 13. Advanced Examples
- [ ] 13.1 Create `examples/multi_uart/`
  - [ ] 13.1.1 Multiple UART instances simultaneously
  - [ ] 13.1.2 Device registry usage
  - [ ] 13.1.3 Buffered communication
- [ ] 13.2 Create `examples/error_propagation/`
  - [ ] 13.2.1 Nested function error handling
  - [ ] 13.2.2 Error logging patterns
  - [ ] 13.2.3 Recovery strategies
- [ ] 13.3 Create `examples/performance_comparison/`
  - [ ] 13.3.1 Old API vs new API benchmarks
  - [ ] 13.3.2 Binary size comparison
  - [ ] 13.3.3 Memory usage comparison

## Phase 5: Testing & Validation (Week 9-10)

### 14. Comprehensive Testing
- [ ] 14.1 Unit test coverage >90%
  - [ ] 14.1.1 All core classes tested
  - [ ] 14.1.2 All HAL interfaces tested
  - [ ] 14.1.3 Edge cases covered
- [ ] 14.2 Integration test suite
  - [ ] 14.2.1 Multi-peripheral tests
  - [ ] 14.2.2 Resource conflict tests
  - [ ] 14.2.3 Error propagation tests
- [ ] 14.3 Performance benchmarks
  - [ ] 14.3.1 Result<T> overhead measurement
  - [ ] 14.3.2 Template peripheral performance
  - [ ] 14.3.3 RAII wrapper overhead
  - [ ] 14.3.4 Circular buffer performance
- [ ] 14.4 Binary size analysis
  - [ ] 14.4.1 Measure before/after for each pattern
  - [ ] 14.4.2 Verify <10% increase (or reduction)
  - [ ] 14.4.3 Identify optimization opportunities
- [ ] 14.5 Static analysis
  - [ ] 14.5.1 Run clang-tidy
  - [ ] 14.5.2 Run cppcheck
  - [ ] 14.5.3 Fix all warnings
- [ ] 14.6 Memory leak detection
  - [ ] 14.6.1 Valgrind on host tests
  - [ ] 14.6.2 Manual review of RAII wrappers
  - [ ] 14.6.3 Verify all resources released

### 15. Compatibility Testing
- [ ] 15.1 Test old API (deprecated)
  - [ ] 15.1.1 Verify all old APIs still work
  - [ ] 15.1.2 Verify deprecation warnings appear
- [ ] 15.2 Test new API
  - [ ] 15.2.1 All new features functional
  - [ ] 15.2.2 Examples compile and run
- [ ] 15.3 Test mixed usage (old + new)
  - [ ] 15.3.1 Verify interoperability
  - [ ] 15.3.2 No conflicts or crashes
- [ ] 15.4 Test on target hardware
  - [ ] 15.4.1 SAME70 Xplained board
  - [ ] 15.4.2 Other supported boards
  - [ ] 15.4.3 All peripherals tested

### 16. Documentation Review
- [ ] 16.1 Technical review
  - [ ] 16.1.1 Accuracy of all documentation
  - [ ] 16.1.2 Code examples compile and run
  - [ ] 16.1.3 No broken links
- [ ] 16.2 User testing
  - [ ] 16.2.1 Get feedback from 2-3 users
  - [ ] 16.2.2 Identify confusing areas
  - [ ] 16.2.3 Update based on feedback
- [ ] 16.3 Migration guide validation
  - [ ] 16.3.1 Follow guide to migrate example project
  - [ ] 16.3.2 Document any missing steps
  - [ ] 16.3.3 Update guide

## Phase 6: Release Preparation (Week 11-12)

### 17. Final Validation
- [ ] 17.1 Run all tests on all platforms
- [ ] 17.2 Verify no regressions
- [ ] 17.3 Check documentation completeness
- [ ] 17.4 Review migration guide one final time
- [ ] 17.5 Update CHANGELOG.md
  - [ ] 17.5.1 List all new features
  - [ ] 17.5.2 List all breaking changes
  - [ ] 17.5.3 Migration instructions link

### 18. Release Candidate
- [ ] 18.1 Create RC branch
- [ ] 18.2 Tag RC version (e.g., v2.0.0-rc1)
- [ ] 18.3 Build and test RC on all platforms
- [ ] 18.4 Send RC to early adopters
- [ ] 18.5 Collect feedback
- [ ] 18.6 Fix critical issues
- [ ] 18.7 Create RC2 if needed

### 19. Final Release
- [ ] 19.1 Merge to main branch
- [ ] 19.2 Tag release version (v2.0.0)
- [ ] 19.3 Build release artifacts
- [ ] 19.4 Publish documentation
- [ ] 19.5 Write release announcement
- [ ] 19.6 Update project README
- [ ] 19.7 Notify users

## Phase 7: Post-Release (Week 13+)

### 20. Migration Support
- [ ] 20.1 Monitor for issues
- [ ] 20.2 Answer questions (GitHub issues, forum)
- [ ] 20.3 Update FAQ based on common questions
- [ ] 20.4 Fix bugs discovered in field
- [ ] 20.5 Release patches as needed (v2.0.1, v2.0.2, etc.)

### 21. Deprecation Timeline
- [ ] 21.1 Week 13-24: Old APIs deprecated with warnings
- [ ] 21.2 Week 25-26: Final reminder of upcoming removal
- [ ] 21.3 Week 27+: Remove old APIs in v3.0.0

### 22. Future Enhancements
- [ ] 22.1 Add more template peripherals (ADC, PWM, TC)
- [ ] 22.2 Implement advanced PWM features (fault protection)
- [ ] 22.3 Add DMA support with circular buffers
- [ ] 22.4 Implement interrupt management layer
- [ ] 22.5 Add power management features
- [ ] 22.6 Port to additional platforms (STM32, ESP32, etc.)

## Success Criteria Checklist

- [ ] ✅ All 180+ tasks completed
- [ ] ✅ Test coverage >90%
- [ ] ✅ No regressions in existing functionality
- [ ] ✅ Binary size within targets (<10% increase or better)
- [ ] ✅ Performance within targets (<1% regression)
- [ ] ✅ Documentation coverage >90%
- [ ] ✅ Migration guide tested by external users
- [ ] ✅ All examples updated and working
- [ ] ✅ Positive feedback from early adopters
- [ ] ✅ Clean static analysis results
- [ ] ✅ Zero memory leaks detected

## Notes

- Each task should be completed and tested before moving to next
- Mark tasks with `[x]` when completed
- Add notes inline for any blockers or issues
- Update timeline if delays occur
- Prioritize critical path items (Phase 1-2)
- Phases 3-4 can be done in parallel if resources available
