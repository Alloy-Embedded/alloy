# Alloy Testing Guide

This directory contains the testing infrastructure for the Alloy framework.

## Overview

All tests use [Google Test](https://github.com/google/googletest) framework, automatically fetched via CMake FetchContent. Tests are organized into:

- **Unit Tests** (`tests/unit/`): Fast tests for individual components running on host
- **Integration Tests** (`tests/integration/`): Future - tests with real hardware

## Running Tests

### Quick Start

```bash
# Configure with tests enabled (default)
cmake -B build -S . -DALLOY_BOARD=host

# Build tests
cmake --build build

# Run all tests
cd build && ctest

# Run with verbose output
cd build && ctest --output-on-failure

# Run with even more detail
cd build && ctest --verbose
```

### Run Specific Tests

```bash
# Run only GPIO tests
cd build && ./tests/unit/alloy_unit_tests --gtest_filter="GpioHostTest.*"

# Run a single test
cd build && ./tests/unit/alloy_unit_tests --gtest_filter="GpioHostTest.SetHighChangesState"

# List all tests
cd build && ./tests/unit/alloy_unit_tests --gtest_list_tests
```

### Disable Tests

```bash
cmake -B build -S . -DALLOY_BUILD_TESTS=OFF
```

## Writing Tests

### Test Structure

Follow the **Given-When-Then** pattern:

```cpp
#include <gtest/gtest.h>
#include "hal/host/gpio.hpp"

TEST(ComponentName, TestName) {
    // Given: Setup initial state
    alloy::hal::host::GpioPin<13> pin;

    // When: Perform action
    pin.set_high();

    // Then: Verify result
    EXPECT_TRUE(pin.read());
}
```

### Naming Conventions

#### File Names
- Format: `test_<component>.cpp`
- Examples:
  - `test_gpio_host.cpp` - GPIO host implementation tests
  - `test_error_handling.cpp` - Error handling tests
  - `test_uart_driver.cpp` - UART driver tests

#### Test Names
- Use descriptive names: `TEST(Component, WhatItDoes)`
- Examples:
  - `TEST(GpioHostTest, SetHighChangesState)`
  - `TEST(UartDriver, SendsDataCorrectly)`
  - `TEST(ErrorHandling, ResultHoldsValue)`

### Test Fixtures

Use fixtures for common setup/teardown:

```cpp
class MyComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Runs before each test
    }

    void TearDown() override {
        // Runs after each test
    }

    // Shared test data
    alloy::hal::host::GpioPin<13> pin;
};

TEST_F(MyComponentTest, DoSomething) {
    // Use pin from fixture
    pin.set_high();
    EXPECT_TRUE(pin.read());
}
```

### Assertions

#### Common Assertions

```cpp
// Boolean conditions
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// Equality/inequality
EXPECT_EQ(actual, expected);
EXPECT_NE(actual, expected);

// Comparisons
EXPECT_LT(val1, val2);  // Less than
EXPECT_LE(val1, val2);  // Less than or equal
EXPECT_GT(val1, val2);  // Greater than
EXPECT_GE(val1, val2);  // Greater than or equal

// String comparisons
EXPECT_STREQ(str1, str2);
EXPECT_STRNE(str1, str2);

// Floating point
EXPECT_FLOAT_EQ(actual, expected);
EXPECT_DOUBLE_EQ(actual, expected);
```

#### EXPECT vs ASSERT

- **EXPECT_**: Test continues after failure (use for multiple checks)
- **ASSERT_**: Test stops after failure (use for preconditions)

```cpp
TEST(Example, MultipleChecks) {
    ASSERT_TRUE(setup_successful);  // Stop if setup fails

    EXPECT_EQ(result1, 10);  // Continue even if fails
    EXPECT_EQ(result2, 20);  // This still runs
}
```

### Compile-Time Tests

Test concepts and static properties:

```cpp
TEST(ConceptTest, TypeSatisfiesConcept) {
    // Verify type meets concept requirements
    static_assert(alloy::hal::GpioPin<alloy::hal::host::GpioPin<0>>,
                  "Type must satisfy GpioPin concept");

    // Verify size
    static_assert(sizeof(MyType) <= 64,
                  "Type must fit in 64 bytes");

    // Test always passes if it compiles
    SUCCEED();
}
```

## Example: GPIO Host Tests

See `tests/unit/test_gpio_host.cpp` for a complete example:

```cpp
/// Test: GPIO pin initial state
TEST_F(GpioHostTest, InitialState) {
    // Given: A newly created GPIO pin
    alloy::hal::host::GpioPin<13> pin;

    // When: Reading the initial state
    bool state = pin.read();

    // Then: State should be LOW (false)
    EXPECT_FALSE(state);
}
```

## Adding New Tests

1. **Create test file** in `tests/unit/test_<component>.cpp`

2. **Add to CMakeLists.txt**:
   ```cmake
   add_executable(alloy_unit_tests
       test_gpio_host.cpp
       test_new_component.cpp  # Add here
   )
   ```

3. **Write tests** following naming conventions

4. **Build and run**:
   ```bash
   cmake --build build
   cd build && ctest
   ```

## CI Integration

Tests run automatically on every commit via GitHub Actions (`.github/workflows/ci.yml`).

The CI:
- Builds for host target
- Runs all unit tests
- Fails the build if any test fails

## Best Practices

### DO
✅ Write tests for all public APIs
✅ Test edge cases and error conditions
✅ Keep tests independent (no shared state between tests)
✅ Use descriptive test names
✅ Follow Given-When-Then structure
✅ Mock hardware dependencies for unit tests

### DON'T
❌ Don't test private implementation details
❌ Don't write tests that depend on execution order
❌ Don't use sleeps/delays (use mocks with deterministic timing)
❌ Don't commit tests that require hardware (use integration tests)

### Test Coverage Goals

- **Core types**: 100% coverage
- **HAL interfaces**: 100% coverage
- **HAL implementations**: >90% coverage
- **Drivers**: >80% coverage
- **Examples**: Manual testing OK

## Debugging Failed Tests

### Verbose Output

```bash
# Show test output even on success
cd build && ctest --verbose

# Show only failures
cd build && ctest --output-on-failure

# Run specific failing test with gtest
cd build && ./tests/unit/alloy_unit_tests --gtest_filter="FailingTest.*"
```

### Using a Debugger

```bash
# Run tests under lldb/gdb
cd build
lldb ./tests/unit/alloy_unit_tests

# Set breakpoint
(lldb) b test_gpio_host.cpp:42
(lldb) run
```

### Common Issues

**Test compiles but crashes:**
- Check for uninitialized variables
- Verify mock implementations are complete

**Test is flaky (passes/fails randomly):**
- Likely timing issue - use mocks, not real delays
- Check for shared mutable state between tests

**EXPECT_EQ fails with weird output:**
- Ensure types are comparable
- Add `operator<<` for custom types if needed

## Resources

- [Google Test Primer](https://google.github.io/googletest/primer.html)
- [Google Test Advanced Guide](https://google.github.io/googletest/advanced.html)
- [Google Mock](https://google.github.io/googletest/gmock_for_dummies.html) (for future use)

## Questions?

See `CONTRIBUTING.md` for contribution guidelines or open an issue on GitHub.
