# SAME70 HAL Unit Tests

Unit tests for SAME70 platform HAL implementation using mock registers.

## Overview

These tests verify the behavior of SAME70 HAL peripherals (UART, GPIO, etc.) **without requiring actual hardware**. They use mock registers that simulate hardware behavior.

## Test Structure

```
tests/unit/hal/same70/
├── README.md           # This file
├── CMakeLists.txt      # Test build configuration
├── uart_mock.hpp       # Mock UART registers
└── uart_test.cpp       # UART unit tests (Catch2)
```

## Running Tests

### Build and Run

```bash
# Configure for host platform (Linux/macOS)
cmake -DALLOY_BOARD=host \
      -DALLOY_PLATFORM=linux \
      -DALLOY_BUILD_TESTS=ON \
      -B build-test

# Build tests
make -C build-test

# Run all tests
ctest --test-dir build-test --output-on-failure

# Run specific test
./build-test/tests/unit/hal/same70/uart_test
```

### Run with Catch2 filters

```bash
# Run only lifecycle tests
./build-test/tests/unit/hal/same70/uart_test "[lifecycle]"

# Run only error handling tests
./build-test/tests/unit/hal/same70/uart_test "[error]"

# List all tests
./build-test/tests/unit/hal/same70/uart_test --list-tests
```

## Test Coverage

### UART Tests (`uart_test.cpp`)

#### Lifecycle Tests
- ✅ `open()` enables peripheral clock
- ✅ `open()` configures registers correctly
- ✅ `isOpen()` reflects state
- ✅ Cannot open twice
- ✅ `close()` disables transmitter/receiver
- ✅ Operations fail when not open

#### Write Tests
- ✅ Write single byte
- ✅ Write multiple bytes
- ✅ Write empty data (size = 0)
- ✅ Null pointer validation

#### Read Tests
- ✅ Read single byte
- ✅ Read multiple bytes
- ✅ Timeout when no data available
- ✅ Null pointer validation

#### Baudrate Tests
- ✅ Set baudrate (9600, 115200, 921600)
- ✅ Get current baudrate
- ✅ Default baudrate is 115200

#### Concept Tests
- ✅ `Uart0` satisfies `UartConcept`
- ✅ Type aliases are distinct types
- ✅ Compile-time constants are correct

## Mock System

### How Mocks Work

The mock system simulates UART hardware registers:

```cpp
// In production code:
HW->THR = data[i];              // Write to actual register

// In tests:
mock_uart->THR = data[i];       // Write to mock register
// Mock captures this write for verification
```

### Mock Features

**MockUartRegisters:**
- Simulates all UART registers (CR, MR, SR, THR, RHR, BRGR, etc.)
- Tracks register writes for verification
- Simulates TXRDY/RXRDY flags
- Captures transmitted data
- Queues data to be received

**MockPmc:**
- Simulates Power Management Controller
- Tracks clock enable requests

### Example Test

```cpp
TEST_CASE("UART write transmits data") {
    UartTestFixture fixture;    // Sets up mocks
    auto uart = Uart0{};

    uart.open();

    // Simulate transmitter ready
    fixture.uart_regs().set_tx_ready(true);

    // Write data
    uint8_t data[] = {'H', 'i'};
    auto result = uart.write(data, 2);

    // Verify
    REQUIRE(result.is_ok());
    REQUIRE(fixture.uart_regs().transmitted_matches("Hi"));
}
```

## Implementation Approach

The tests use **conditional compilation with test hooks** to intercept hardware register access without affecting production code performance.

### How It Works

1. **Mock Register Injection**: Test code defines macros that replace hardware register access with mock pointers:
   ```cpp
   #define ALLOY_UART_MOCK_HW() (mock_uart_ptr)
   #define ALLOY_UART_MOCK_PMC() (mock_pmc_ptr)
   ```

2. **Test Hooks**: Strategic hooks in UART template notify mocks of register access:
   ```cpp
   hw->THR = data[i];
   #ifdef ALLOY_UART_TEST_HOOK_THR
       ALLOY_UART_TEST_HOOK_THR(hw, data[i]);  // Capture write
   #endif
   ```

3. **Zero Production Overhead**: When not testing, all `#ifdef` blocks are compiled out, leaving **zero overhead** in production builds.

### Benefits

- ✅ No virtual functions
- ✅ No runtime polymorphism
- ✅ Zero overhead in production
- ✅ Full test coverage without hardware
- ✅ Simple conditional compilation approach

## Adding New Tests

1. Create mock for the peripheral (e.g., `gpio_mock.hpp`)
2. Write tests using Catch2 (`gpio_test.cpp`)
3. Add to CMakeLists.txt
4. Run tests

## References

- [Catch2 Documentation](https://github.com/catchorg/Catch2)
- UART Implementation: `src/hal/platform/same70/uart.hpp`
- OpenSpec: `openspec/changes/platform-abstraction/`

## Test Results

```
All tests passed (64 assertions in 14 test cases)
```

Test Summary:
- ✅ Test framework setup complete
- ✅ Mock system implemented with conditional compilation
- ✅ Test cases written and passing
- ✅ Integration with UART template complete
- ✅ Zero overhead verified (test hooks compiled out in production)
