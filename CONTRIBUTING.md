# Contributing to MicroCore

Thank you for your interest in contributing to MicroCore! This document provides guidelines for contributing to the project.

## Getting Started

1. **Read the documentation**
   - [README.md](README.md) - Project overview
   - [architecture.md](architecture.md) - Technical architecture
   - [decisions.md](decisions.md) - Architecture Decision Records (ADRs)
   - [plan.md](plan.md) - Project vision and roadmap

2. **Set up your development environment**
   - **Clang 21+** (required for macOS to avoid build issues) or GCC 11+
   - CMake 3.25+
   - Python 3.10+ (for code generator)
   - Catch2 v3 (fetched automatically)

   **macOS Setup:**
   ```bash
   brew install llvm@21 cmake ninja
   export PATH="$(brew --prefix llvm@21)/bin:$PATH"
   export CC=$(brew --prefix llvm@21)/bin/clang
   export CXX=$(brew --prefix llvm@21)/bin/clang++
   ```

   **Linux Setup:**
   ```bash
   sudo apt-get install clang-14 cmake ninja-build
   export CC=clang-14
   export CXX=clang++-14
   ```

## Coding Standards

### Naming Conventions

We follow strict naming conventions as defined in [ADR-011](decisions.md#adr-011-naming-conventions-snake_case):

**Files and Directories:**
- Headers: `snake_case.hpp` (e.g., `gpio_pin.hpp`)
- Sources: `snake_case.cpp` (e.g., `uart_driver.cpp`)
- Directories: `snake_case` (e.g., `hal/`, `drivers/`)

**C++ Code:**
- Namespaces: `snake_case` (e.g., `ucore::hal::`)
- Classes/Structs: `PascalCase` (e.g., `GpioPin`, `UartDriver`)
- Functions/Methods: `camelCase` (e.g., `setHigh()`, `readByte()`)
- Variables: `snake_case` (e.g., `led_pin`, `baud_rate`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`)
- Template Parameters: `PascalCase` (e.g., `template<typename PinImpl>`)

**CMake:**
- Variables: `MICROCORE_` prefix + `UPPER_SNAKE_CASE` (e.g., `MICROCORE_BOARD`)
- Functions: `alloy_` prefix + `snake_case` (e.g., `alloy_target_compile_options()`)

**Macros:**
- `ALLOY_` prefix + `UPPER_SNAKE_CASE` (e.g., `ALLOY_ENABLE_LOGGING`)

### Code Style

- **C++ Standard**: C++20 (no modules, use headers)
- **Error Handling**: Use `Result<T, ErrorCode>` ([ADR-012](decisions.md#adr-012-custom-error-codes-não-exceptions))
- **Memory Allocation**: No dynamic allocation in HAL ([ADR-009](decisions.md#adr-009-zero-dynamic-allocation-na-hal))
- **Header Guards**: Use `#pragma once`
- **Include Order**:
  1. Corresponding header (for .cpp files)
  2. Project headers
  3. External library headers
  4. Standard library headers

### Example

```cpp
// src/hal/interface/gpio.hpp
#pragma once

#include "core/types.hpp"
#include <cstdint>

namespace alloy::hal {

enum class PinMode : uint8_t {
    Input,
    Output,
    InputPullUp,
    InputPullDown
};

template<typename T>
concept GpioPin = requires(T pin) {
    { pin.set_high() } -> std::same_as<void>;
    { pin.set_low() } -> std::same_as<void>;
    { pin.read() } -> std::same_as<bool>;
};

} // namespace alloy::hal
```

## Development Workflow

### 1. Using OpenSpec

MicroCore uses [OpenSpec](openspec/AGENTS.md) for spec-driven development. Before implementing features:

1. Check existing specs: `openspec list --specs`
2. Check active changes: `openspec list`
3. Review the change proposal: `openspec show <change-id>`

### 2. Implementing Changes

For each change:

1. **Read the spec**
   ```bash
   openspec show add-gpio-interface
   cat openspec/changes/add-gpio-interface/tasks.md
   ```

2. **Implement tasks sequentially**
   - Follow the checklist in `tasks.md`
   - Mark tasks as `[x]` when complete

3. **Test your implementation**
   ```bash
   cmake -B build -S .
   cmake --build build
   ctest --test-dir build
   ```

4. **Commit with clear messages**
   ```bash
   git add .
   git commit -m "feat(hal): implement GPIO interface concepts

   - Add GpioPin concept in hal/interface/gpio.hpp
   - Define PinMode enum
   - Add ConfiguredGpioPin template

   Implements tasks 1.1-1.6 of add-gpio-interface"
   ```

### 3. Creating Pull Requests

- **One PR per change proposal** (follow OpenSpec changes)
- **Clear title**: `feat(hal): add GPIO interface (#3)`
- **Description should include**:
  - Link to OpenSpec change: `Implements openspec/changes/add-gpio-interface/`
  - List of completed tasks
  - Testing performed
  - Breaking changes (if any)

## Testing

MicroCore uses **Catch2 v3** as the testing framework for C++ unit tests. All tests run automatically in CI on every push and pull request.

### Running Tests Locally

```bash
# Configure with tests enabled (host platform)
cmake -S . -B build-host -DMICROCORE_BOARD=host -DMICROCORE_BUILD_TESTS=ON

# Build tests
cmake --build build-host

# Run all tests with CTest
cd build-host && ctest --output-on-failure

# Run specific test executable
./build-host/tests/test_result

# Run tests with Catch2 filters
./build-host/tests/test_result "[error][result]"

# List all test cases
./build-host/tests/test_result --list-tests

# List all tags
./build-host/tests/test_result --list-tags
```

### Writing Unit Tests

MicroCore uses **Catch2 v3** for all new tests. Tests should be placed in `tests/unit/test_<component>.cpp`.

#### Basic Test Structure

```cpp
// tests/unit/test_gpio_host.cpp
#include "hal/host/gpio.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("GPIO set_high changes state", "[gpio][host]") {
    // Given: A GPIO pin
    alloy::hal::host::GpioPin<25> pin;

    // When: Setting pin HIGH
    pin.set_high();

    // Then: State should be HIGH
    REQUIRE(pin.read());
}
```

#### Using Sections

```cpp
TEST_CASE("GPIO multiple modes", "[gpio][mode]") {
    alloy::hal::host::GpioPin<13> pin;

    SECTION("Input mode") {
        pin.configure(alloy::hal::PinMode::Input);
        REQUIRE(pin.get_mode() == alloy::hal::PinMode::Input);
    }

    SECTION("Output mode") {
        pin.configure(alloy::hal::PinMode::Output);
        REQUIRE(pin.get_mode() == alloy::hal::PinMode::Output);
    }
}
```

#### Testing with Tags

Use tags to organize and filter tests:

```cpp
TEST_CASE("Fast unit test", "[core][fast]") {
    // Quick test
}

TEST_CASE("Slow integration test", "[hal][slow][integration]") {
    // Longer test
}

// Run only fast tests:
// ./test_executable "[fast]"

// Run only core tests:
// ./test_executable "[core]"
```

#### Common Assertions

```cpp
// Boolean checks
REQUIRE(condition);           // Must be true
REQUIRE_FALSE(condition);     // Must be false

// Equality
REQUIRE(a == b);
REQUIRE(a != b);

// Comparisons
REQUIRE(value > 0);
REQUIRE(value <= 100);

// Result<T, E> testing
auto result = some_function();
REQUIRE(result.is_ok());
REQUIRE(result.value() == expected);

// Error cases
REQUIRE(result.is_error());
REQUIRE(result.error() == ErrorCode::Timeout);
```

#### Testing Best Practices

1. **Use Given-When-Then structure** for clarity:
   ```cpp
   TEST_CASE("Example test", "[component]") {
       // Given: Initial state
       Component comp;

       // When: Action performed
       auto result = comp.do_something();

       // Then: Expected outcome
       REQUIRE(result.is_ok());
   }
   ```

2. **Test one thing per test case** - Keep tests focused and simple

3. **Use descriptive names** - Test names should explain what is being tested

4. **Tag appropriately** - Use tags like `[core]`, `[hal]`, `[fast]`, `[slow]`, `[integration]`

5. **Test error paths** - Don't just test success cases:
   ```cpp
   TEST_CASE("Function rejects invalid input", "[validation]") {
       auto result = function_under_test(invalid_input);
       REQUIRE(result.is_error());
       REQUIRE(result.error() == ErrorCode::InvalidParameter);
   }
   ```

### Test Coverage

We aim for high test coverage on:
- **Core library** (`src/core/`) - 90%+ coverage
- **HAL interfaces** (`src/hal/interface/`) - 80%+ coverage
- **Platform implementations** (`src/hal/platform/`) - 70%+ coverage (hardware-dependent)

### Continuous Integration

Tests run automatically on GitHub Actions:
- **Ubuntu** + **macOS**
- **GCC** and **Clang** compilers
- **AddressSanitizer + UBSan** for memory safety
- All tests must pass before merging

### Integration Tests

For hardware-dependent tests:
- Place in `tests/integration/`
- Document required hardware setup
- Mark with `[integration]` tag
- These may be skipped in CI (run manually on hardware)

## Code Generation and Validation

MicroCore uses Python-based code generation to create hardware register definitions from vendor SVD/ATDF files. All generated code must pass validation before being committed.

### Generating Code

```bash
# Generate register definitions for STM32F4
python3 tools/codegen/generate_stm32.py --device STM32F401 --output src/hal/vendors/st/stm32f4/generated/

# Generate register definitions for SAME70
python3 tools/codegen/generate_same70.py --device ATSAME70Q21B --output src/hal/vendors/atmel/same70/generated/
```

### Validating Generated Code

All generated code must pass compile-time validation tests:

```bash
# Configure with validation tests
cmake -S . -B build-validation -DMICROCORE_BOARD=host

# Run validation targets
cmake --build build-validation --target validate-generated-code
cmake --build build-validation --target validate-generated-style
```

#### Validation Tests

The validation system checks:

1. **Compile-time correctness** (`validate_stm32f4_generated.cpp`, `validate_same70_generated.cpp`):
   - Register struct sizes (using `sizeof`)
   - Register field offsets (using `offsetof`)
   - Bitfield positions and widths
   - Type safety (strong typing, no implicit conversions)

2. **Code style** (`validate-generated-style`):
   - clang-format compliance
   - Naming conventions
   - Comment formatting

3. **Static analysis** (optional, `validate-generated-quality`):
   - clang-tidy checks
   - No warnings with `-Wall -Wextra -Wpedantic -Werror`

#### Example Validation Test

```cpp
// tools/codegen/tests/validate_stm32f4_generated.cpp
#include "hal/vendors/st/stm32f4/generated/registers/gpio_registers.hpp"

using namespace ucore::hal::stm32f4::gpio;

// Verify register struct size matches hardware
static_assert(sizeof(GPIO_Registers) == 0x400, "GPIO register block size incorrect");

// Verify individual register offsets
static_assert(offsetof(GPIO_Registers, MODER) == 0x00, "MODER offset incorrect");
static_assert(offsetof(GPIO_Registers, ODR) == 0x14, "ODR offset incorrect");
static_assert(offsetof(GPIO_Registers, BSRR) == 0x18, "BSRR offset incorrect");

// Verify register field sizes
static_assert(sizeof(GPIO_Registers::MODER) == 4, "MODER should be 32-bit");
```

### Code Generation Best Practices

When adding or modifying code generators:

1. **Preserve hardware semantics**:
   - Register sizes must match datasheet exactly
   - Bit positions must be hardware-accurate
   - Reserved fields must be marked clearly

2. **Type safety**:
   - Use `enum class` for register fields
   - Use `uint32_t`, `uint16_t`, `uint8_t` (never `int`)
   - Avoid bitfields (use explicit masks instead)

3. **Documentation**:
   - Generate Doxygen comments from SVD/ATDF descriptions
   - Include register addresses and reset values
   - Link to datasheet sections

4. **Validation**:
   - Add compile-time assertions for all generated code
   - Test on actual hardware when possible
   - Compare with vendor HAL implementations

### CI/CD Integration

Generated code validation runs automatically in CI:

```yaml
# .github/workflows/validate-codegen.yml
- name: Validate generated code
  run: |
    cmake -B build -DMICROCORE_BOARD=host
    cmake --build build --target validate-generated-code
```

CI will fail if:
- Generated code doesn't compile
- Static assertions fail
- Code style doesn't match .clang-format
- Any validation test fails

### Regenerating Code

When updating code generators or vendor files:

1. Regenerate all affected code
2. Run validation tests locally
3. Review generated diffs carefully
4. Update validation tests if hardware definitions changed
5. Document changes in commit message

```bash
# Typical workflow
python3 tools/codegen/generate_stm32.py --device STM32F401 --output src/hal/vendors/st/stm32f4/generated/
cmake --build build --target validate-generated-code
git diff src/hal/vendors/st/stm32f4/generated/  # Review changes
git add src/hal/vendors/st/stm32f4/generated/
git commit -m "refactor(codegen): regenerate STM32F4 registers

- Updated register definitions from STM32CubeMX v1.2.3
- Fixed GPIO BSRR register offset
- Added missing AFRL/AFRH fields

All validation tests pass."
```

## Building

### Host (Native)

```bash
cmake -B build -S . -DMICROCORE_BOARD=host
cmake --build build
./build/examples/blinky/blinky
```

### ARM (Cross-compilation)

```bash
cmake -B build -S . -DMICROCORE_BOARD=rp_pico -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build
```

## Questions?

- **Bugs/Features**: [GitHub Issues](https://github.com/microcore-embedded/microcore/issues)
- **Discussions**: [GitHub Discussions](https://github.com/microcore-embedded/microcore/discussions)
- **Specs**: Check `openspec/` directory

## License

By contributing to Alloy, you agree that your contributions will be licensed under the [MIT License](LICENSE).
