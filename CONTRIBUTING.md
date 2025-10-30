# Contributing to Alloy

Thank you for your interest in contributing to Alloy! This document provides guidelines for contributing to the project.

## Getting Started

1. **Read the documentation**
   - [README.md](README.md) - Project overview
   - [architecture.md](architecture.md) - Technical architecture
   - [decisions.md](decisions.md) - Architecture Decision Records (ADRs)
   - [plan.md](plan.md) - Project vision and roadmap

2. **Set up your development environment**
   - GCC 11+ or Clang 13+ with C++20 support
   - CMake 3.25+
   - Python 3.10+ (for code generator)
   - Google Test (fetched automatically)

## Coding Standards

### Naming Conventions

We follow strict naming conventions as defined in [ADR-011](decisions.md#adr-011-naming-conventions-snake_case):

**Files and Directories:**
- Headers: `snake_case.hpp` (e.g., `gpio_pin.hpp`)
- Sources: `snake_case.cpp` (e.g., `uart_driver.cpp`)
- Directories: `snake_case` (e.g., `hal/`, `drivers/`)

**C++ Code:**
- Namespaces: `snake_case` (e.g., `alloy::hal::`)
- Classes/Structs: `PascalCase` (e.g., `GpioPin`, `UartDriver`)
- Functions/Methods: `snake_case` (e.g., `set_high()`, `read_byte()`)
- Variables: `snake_case` (e.g., `led_pin`, `baud_rate`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`)
- Template Parameters: `PascalCase` (e.g., `template<typename PinImpl>`)

**CMake:**
- Variables: `ALLOY_` prefix + `UPPER_SNAKE_CASE` (e.g., `ALLOY_BOARD`)
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

Alloy uses [OpenSpec](openspec/AGENTS.md) for spec-driven development. Before implementing features:

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

### Unit Tests

- Use Google Test for unit tests
- Place tests in `tests/unit/test_<component>.cpp`
- Test naming: `TEST(ComponentName, TestCase)`

```cpp
// tests/unit/test_gpio_host.cpp
#include "hal/host/gpio.hpp"
#include <gtest/gtest.h>

TEST(GpioHost, SetHighChangesState) {
    alloy::hal::host::GpioPin<25> pin;

    pin.set_high();
    EXPECT_TRUE(pin.read());
}
```

### Integration Tests

- For hardware tests, place in `tests/integration/`
- Document required hardware setup
- These will run in CI with hardware runners (future)

## Building

### Host (Native)

```bash
cmake -B build -S . -DALLOY_BOARD=host
cmake --build build
./build/examples/blinky/blinky
```

### ARM (Cross-compilation)

```bash
cmake -B build -S . -DALLOY_BOARD=rp_pico -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
cmake --build build
```

## Questions?

- **Bugs/Features**: [GitHub Issues](https://github.com/alloy-embedded/alloy/issues)
- **Discussions**: [GitHub Discussions](https://github.com/alloy-embedded/alloy/discussions)
- **Specs**: Check `openspec/` directory

## License

By contributing to Alloy, you agree that your contributions will be licensed under the [MIT License](LICENSE).
