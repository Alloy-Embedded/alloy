# Platform Abstraction Implementation Tasks

**Total Estimated Time**: 8 weeks
**Approach**: Template-based, ZERO virtual functions
**Performance Goal**: Binary size and runtime identical to manual register access

---

## Phase 1: Concept Layer & Foundation (Week 1-2)

### Week 1: C++20 Concepts (or C++17 Fallback)

#### Task 1.1: Create UART Concept
- **File**: `src/hal/concepts/uart_concept.hpp`
- **Estimate**: 4 hours
- **Details**:
  - Define `UartConcept` using C++20 `concept` keyword
  - Validate: `open()`, `close()`, `write()`, `read()`, `setBaudrate()`
  - Check return types: `Result<void>`, `Result<size_t>`
  - Add C++17 fallback using `is_uart_v` type trait + static_assert
- **Acceptance**: Compiles with both C++17 and C++20, rejects non-UART types

#### Task 1.2: Create GPIO Concept
- **File**: `src/hal/concepts/gpio_concept.hpp`
- **Estimate**: 3 hours
- **Details**:
  - Define `GpioConcept` with methods: `setMode()`, `write()`, `read()`, `toggle()`
  - Validate GPIO modes enum
  - C++17 fallback with type traits
- **Acceptance**: Generic GPIO code works with any compliant implementation

#### Task 1.3: Create I2C/SPI Concepts
- **Files**: `src/hal/concepts/i2c_concept.hpp`, `src/hal/concepts/spi_concept.hpp`
- **Estimate**: 4 hours each (8 hours total)
- **Details**:
  - Define bus interface contracts
  - Include bus locking methods
  - Transfer methods with timeout
- **Acceptance**: Generic I2C/SPI code type-checks at compile-time

#### Task 1.4: Create Concept Tests
- **File**: `tests/hal/concepts_test.cpp`
- **Estimate**: 4 hours
- **Details**:
  - Create mock types that satisfy concepts
  - Create mock types that violate concepts (should not compile)
  - Verify error messages are clear
- **Acceptance**: All tests pass, invalid types rejected

**Week 1 Total**: 19 hours

### Week 2: Common Types & Utilities

#### Task 2.1: Define Common HAL Types
- **File**: `src/hal/types.hpp`
- **Estimate**: 2 hours
- **Details**:
  - Baudrate enum class
  - GPIO Mode enum class
  - Parity, StopBits enums
  - I2C/SPI modes
- **Acceptance**: All types are platform-independent

#### Task 2.2: Create Platform Selection CMake Module
- **File**: `cmake/platform_selection.cmake`
- **Estimate**: 6 hours
- **Details**:
  - Define `ALLOY_PLATFORM` cache variable
  - Include correct platform files based on selection
  - Set preprocessor defines: `ALLOY_PLATFORM_SAME70`, etc.
  - Error if invalid platform selected
- **Acceptance**: Only selected platform compiled, clean build system

#### Task 2.3: Create Board Configuration Structure
- **File**: `cmake/board_selection.cmake`
- **Estimate**: 4 hours
- **Details**:
  - Define `ALLOY_BOARD` cache variable
  - Map boards to platforms (same70_xplained → same70)
  - Include board-specific headers
- **Acceptance**: Board selection works, type aliases correct

#### Task 2.4: Update Root CMakeLists.txt
- **File**: `CMakeLists.txt`
- **Estimate**: 3 hours
- **Details**:
  - Include platform_selection.cmake
  - Add hal library with platform sources
  - Set up proper include paths
- **Acceptance**: Clean CMake configuration, no errors

**Week 2 Total**: 15 hours

**Phase 1 Total**: 34 hours (~1.7 weeks)

---

## Phase 2: SAME70 Platform Templates (Week 3-4)

### Week 3: UART Template Implementation

#### Task 3.1: Create SAME70 UART Template
- **File**: `src/hal/platform/same70/uart.hpp`
- **Estimate**: 8 hours
- **Details**:
  - Template class: `template <uint32_t BASE_ADDR, uint32_t IRQ_ID> class Uart`
  - Implement: `open()`, `close()`, `setBaudrate()`, `write()`, `read()`
  - Use `constexpr` for BASE_ADDR register access
  - All methods return `Result<T>`
  - NO virtual functions
- **Acceptance**: Compiles, satisfies UartConcept, generates optimal assembly

#### Task 3.2: Create UART Type Aliases
- **File**: `src/hal/platform/same70/uart.hpp`
- **Estimate**: 2 hours
- **Details**:
  - `using Uart0 = Uart<0x400E0800, ID_UART0>`
  - `using Uart1 = Uart<0x400E0A00, ID_UART1>`
  - `using Uart2 = Uart<0x400E0C00, ID_UART2>`
- **Acceptance**: Type aliases work, each is unique type

#### Task 3.3: Verify Assembly Output
- **File**: `tools/verify_assembly.sh`
- **Estimate**: 4 hours
- **Details**:
  - Compile simple UART write with -S flag
  - Compare to manual register access assembly
  - Verify no vtable, no extra instructions
  - Document findings
- **Acceptance**: Assembly identical to manual code

#### Task 3.4: Create UART Unit Tests
- **File**: `tests/hal/same70/uart_test.cpp`
- **Estimate**: 6 hours
- **Details**:
  - Test open/close lifecycle
  - Test write operations
  - Test error handling (not opened, etc.)
  - Use mock register access for testing
- **Acceptance**: All tests pass, 100% coverage

**Week 3 Total**: 20 hours

### Week 4: GPIO & Board Configuration

#### Task 4.1: Create SAME70 GPIO Template
- **File**: `src/hal/platform/same70/gpio.hpp`
- **Estimate**: 8 hours
- **Details**:
  - Template class: `template <uint32_t PORT_BASE, uint8_t PIN_NUM> class GpioPin`
  - Compile-time bit mask: `static constexpr uint32_t MASK = (1U << PIN_NUM)`
  - Implement: `setMode()`, `write()`, `read()`, `toggle()`
  - NO virtual functions
- **Acceptance**: Single instruction for write/read, satisfies GpioConcept

#### Task 4.2: Create GPIO Type Aliases
- **File**: `src/hal/platform/same70/gpio.hpp`
- **Estimate**: 2 hours
- **Details**:
  - `using LedGreen = GpioPin<PIOC_BASE, 8>`
  - Aliases for all board pins
- **Acceptance**: Type-safe pin access

#### Task 4.3: Create SAME70 Xplained Board Config
- **File**: `boards/same70_xplained/board.hpp`
- **Estimate**: 4 hours
- **Details**:
  - Type aliases: `using uart0 = hal::same70::Uart0`
  - GPIO aliases for LEDs, buttons
  - Clock configuration
  - Board initialization function
- **Acceptance**: User code uses `board::uart0`, platform-agnostic

#### Task 4.4: Create GPIO Unit Tests
- **File**: `tests/hal/same70/gpio_test.cpp`
- **Estimate**: 4 hours
- **Details**:
  - Test all GPIO operations
  - Verify compile-time bit masks
  - Test error handling
- **Acceptance**: All tests pass

#### Task 4.5: Port Existing Blink Example
- **File**: `examples/same70_blink/main.cpp`
- **Estimate**: 2 hours
- **Details**:
  - Refactor to use template-based GPIO
  - Use `board::led_green` instead of direct register
  - Verify binary size unchanged
- **Acceptance**: Blink works, binary size ≤ old version

**Week 4 Total**: 20 hours

**Phase 2 Total**: 40 hours (~2 weeks)

---

## Phase 3: Linux Platform Templates (Week 5-6)

### Week 5: Linux UART Implementation

#### Task 5.1: Create Linux UART Template
- **File**: `src/hal/platform/linux/uart.hpp`
- **Estimate**: 10 hours
- **Details**:
  - Template class using POSIX termios
  - Constructor takes device path (e.g., "/dev/ttyUSB0")
  - Implement same interface as SAME70 UART
  - NO virtual functions (same template approach)
- **Acceptance**: Compiles, satisfies UartConcept

#### Task 5.2: Create Linux Board Config
- **File**: `boards/linux_host/board.hpp`
- **Estimate**: 3 hours
- **Details**:
  - Type aliases: `using uart0 = hal::linux::Uart{"/dev/ttyUSB0"}`
  - Simulated GPIO (optional, or use sysfs)
- **Acceptance**: Board config works, matches SAME70 structure

#### Task 5.3: Create Host-Based UART Tests
- **File**: `tests/hal/linux/uart_test.cpp`
- **Estimate**: 6 hours
- **Details**:
  - Tests that run on host (no hardware)
  - Use pty (pseudo-terminal) for testing
  - Test open, close, write, read
  - Test error conditions
- **Acceptance**: Tests run on host, no hardware needed

#### Task 5.4: Add CMake Linux Platform Support
- **File**: `cmake/platforms/linux.cmake`
- **Estimate**: 3 hours
- **Details**:
  - Set platform sources
  - Link pthread, rt
  - Native compiler (not cross-compile)
- **Acceptance**: `cmake -DALLOY_PLATFORM=linux` works

**Week 5 Total**: 22 hours

### Week 6: CI/CD Integration & Testing

#### Task 6.1: Create Linux GPIO Simulation
- **File**: `src/hal/platform/linux/gpio.hpp`
- **Estimate**: 4 hours
- **Details**:
  - Template GPIO with in-memory state (simulation)
  - OR use sysfs GPIO (if available)
  - Same interface as SAME70 GPIO
- **Acceptance**: GPIO tests run on host

#### Task 6.2: Create GitHub Actions CI Workflow
- **File**: `.github/workflows/hal_tests.yml`
- **Estimate**: 6 hours
- **Details**:
  - Build for Linux platform
  - Run HAL unit tests
  - Generate coverage report
  - Fail if tests don't pass
- **Acceptance**: CI runs on every commit, reports pass/fail

#### Task 6.3: Create Platform Comparison Tests
- **File**: `tests/hal/platform_comparison_test.cpp`
- **Estimate**: 4 hours
- **Details**:
  - Generic tests that work on any platform
  - Use concepts to validate interface
  - Run same tests on SAME70 (hardware) and Linux (simulated)
- **Acceptance**: Tests prove platform abstraction works

#### Task 6.4: Create Documentation for Host Testing
- **File**: `docs/HOST_TESTING.md`
- **Estimate**: 4 hours
- **Details**:
  - Explain Linux platform purpose
  - Step-by-step: run HAL tests on laptop
  - Benefits: fast iteration, no hardware
- **Acceptance**: Developer can run tests following guide

**Week 6 Total**: 18 hours

**Phase 3 Total**: 40 hours (~2 weeks)

---

## Phase 4: ESP32 Platform Templates (Week 7-8)

### Week 7: ESP32 UART Wrapper

#### Task 7.1: Create ESP32 UART Template
- **File**: `src/hal/platform/esp32/uart.hpp`
- **Estimate**: 10 hours
- **Details**:
  - Template wrapping ESP-IDF `uart_*` functions
  - Same interface as SAME70/Linux
  - Handle FreeRTOS delays/timeouts
  - NO virtual functions
- **Acceptance**: Compiles with ESP-IDF, satisfies UartConcept

#### Task 7.2: Create ESP32 GPIO Template
- **File**: `src/hal/platform/esp32/gpio.hpp`
- **Estimate**: 6 hours
- **Details**:
  - Template wrapping ESP-IDF `gpio_*` functions
  - Same interface as other platforms
- **Acceptance**: GPIO works on ESP32

#### Task 7.3: Create ESP32 DevKit Board Config
- **File**: `boards/esp32_devkit/board.hpp`
- **Estimate**: 3 hours
- **Details**:
  - Type aliases for ESP32 peripherals
  - Pin mappings for DevKit
- **Acceptance**: Board config works

#### Task 7.4: Add CMake ESP32 Platform Support
- **File**: `cmake/platforms/esp32.cmake`
- **Estimate**: 6 hours
- **Details**:
  - Integrate with ESP-IDF build system
  - Set platform sources
  - Link ESP-IDF libraries
- **Acceptance**: `cmake -DALLOY_PLATFORM=esp32` works

**Week 7 Total**: 25 hours

### Week 8: ESP32 Testing & Validation

#### Task 8.1: Port Blink Example to ESP32
- **File**: `examples/esp32_blink/main.cpp`
- **Estimate**: 3 hours
- **Details**:
  - Use same code structure as SAME70 blink
  - Only change board header include
- **Acceptance**: Blink works on ESP32

#### Task 8.2: Create Multi-Platform Example
- **File**: `examples/multi_platform_demo/main.cpp`
- **Estimate**: 4 hours
- **Details**:
  - Single source file that compiles for all platforms
  - Uses UART and GPIO
  - Platform selected via CMake
- **Acceptance**: Same code runs on SAME70, Linux, ESP32

#### Task 8.3: Binary Size Comparison
- **File**: `docs/BINARY_SIZE_ANALYSIS.md`
- **Estimate**: 4 hours
- **Details**:
  - Compile same app for SAME70 with template vs direct register
  - Compare binary sizes
  - Compare assembly output
  - Document findings
- **Acceptance**: Template version ≤ manual version

#### Task 8.4: Performance Benchmarks
- **File**: `benchmarks/hal_performance.cpp`
- **Estimate**: 6 hours
- **Details**:
  - Benchmark UART write throughput
  - Benchmark GPIO toggle frequency
  - Compare template vs manual
  - Document results
- **Acceptance**: Template performance ≥ 95% of manual

**Week 8 Total**: 17 hours

**Phase 4 Total**: 42 hours (~2.1 weeks)

---

## Phase 5: Documentation & Polish (Week 9-10)

### Week 9: Comprehensive Documentation

#### Task 9.1: Platform Abstraction Architecture Doc
- **File**: `docs/PLATFORM_ABSTRACTION.md`
- **Estimate**: 6 hours
- **Details**:
  - Explain template-based design
  - Why no virtual functions
  - How concepts work
  - Platform selection flow
- **Acceptance**: Developer understands architecture

#### Task 9.2: Adding New Platform Guide
- **File**: `docs/ADDING_PLATFORMS.md`
- **Estimate**: 6 hours
- **Details**:
  - Step-by-step guide
  - Template requirements
  - Concept validation
  - Board configuration
- **Acceptance**: Developer can add STM32F4 following guide

#### Task 9.3: Migration Guide from Direct Register Access
- **File**: `docs/MIGRATION_GUIDE.md`
- **Estimate**: 4 hours
- **Details**:
  - Before/after examples
  - Performance guarantees
  - Common pitfalls
- **Acceptance**: Existing code can be migrated

#### Task 9.4: API Reference Documentation
- **File**: `docs/API_REFERENCE.md`
- **Estimate**: 6 hours
- **Details**:
  - Document all concepts
  - Document all template parameters
  - Document all methods
  - Include examples
- **Acceptance**: Complete API reference

**Week 9 Total**: 22 hours

### Week 10: Final Testing & Release Prep

#### Task 10.1: Integration Tests
- **File**: `tests/integration/`
- **Estimate**: 8 hours
- **Details**:
  - End-to-end tests on all platforms
  - Multi-peripheral tests (UART + GPIO + I2C)
  - Error handling tests
- **Acceptance**: All integration tests pass

#### Task 10.2: Code Review & Cleanup
- **Estimate**: 6 hours
- **Details**:
  - Review all code for consistency
  - Remove debug code
  - Add missing comments
  - Format with clang-format
- **Acceptance**: Code is clean and consistent

#### Task 10.3: Update Examples & README
- **Files**: `examples/*/README.md`, `README.md`
- **Estimate**: 4 hours
- **Details**:
  - Update all example READMEs
  - Update main README with platform info
  - Add badges for CI status
- **Acceptance**: Documentation is up-to-date

#### Task 10.4: Create Release Notes
- **File**: `CHANGELOG.md`
- **Estimate**: 2 hours
- **Details**:
  - Document all changes
  - Breaking changes
  - New features
  - Migration path
- **Acceptance**: Release notes are complete

**Week 10 Total**: 20 hours

**Phase 5 Total**: 42 hours (~2.1 weeks)

---

## Summary

| Phase | Duration | Hours | Key Deliverables |
|-------|----------|-------|------------------|
| Phase 1: Concepts & Foundation | 2 weeks | 34h | C++20 concepts, CMake platform selection |
| Phase 2: SAME70 Templates | 2 weeks | 40h | Template UART/GPIO, board config, tests |
| Phase 3: Linux Platform | 2 weeks | 40h | Host testing, CI/CD integration |
| Phase 4: ESP32 Platform | 2 weeks | 42h | ESP32 wrappers, multi-platform examples |
| Phase 5: Documentation & Polish | 2 weeks | 42h | Comprehensive docs, integration tests |
| **TOTAL** | **10 weeks** | **198 hours** | **Multi-platform HAL with zero overhead** |

## Critical Success Factors

1. ✅ **Zero Virtual Functions**: Verified via binary analysis
2. ✅ **Performance Parity**: Benchmarks show ≥95% of manual code
3. ✅ **Binary Size**: Template version ≤ manual version
4. ✅ **Compile-Time Validation**: Concepts catch errors early
5. ✅ **Host Testing**: CI/CD runs on every commit
6. ✅ **Multi-Platform Examples**: Same code on 3+ platforms

## Risks & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Template bloat increases binary size | Medium | High | Profile early, use -Os, extern template if needed |
| Concepts not available (old compiler) | Low | Medium | Provide C++17 fallback with static_assert |
| ESP-IDF integration complex | Medium | Medium | Start early, use ESP-IDF CMake integration |
| Performance doesn't match manual | Low | High | Benchmark continuously, inspect assembly |

## Next Steps

1. ✅ Review and approve tasks breakdown
2. ✅ Validate OpenSpec
3. ✅ Begin Phase 1: Concepts & Foundation
4. Set up project tracking (GitHub Projects/Issues)
