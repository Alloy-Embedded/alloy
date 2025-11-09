# Alloy Coding Style Guide

**Version**: 1.0
**Last Updated**: 2025-11-08
**For**: Developers, AI Assistants, Contributors

---

## 📋 Table of Contents

1. [Philosophy](#philosophy)
2. [Language Standard](#language-standard)
3. [Naming Conventions](#naming-conventions)
4. [Code Formatting](#code-formatting)
5. [Modern C++ Features](#modern-cpp-features)
6. [Embedded Best Practices](#embedded-best-practices)
7. [Generated Code](#generated-code)
8. [Documentation](#documentation)
9. [Testing](#testing)
10. [Tooling](#tooling)

---

## 🎯 Philosophy

Alloy is a **modern C++20 embedded systems framework** focused on:

- **Zero-overhead abstractions**: No runtime cost for high-level features
- **Type safety**: Compile-time error detection
- **Bare-metal efficiency**: No dynamic allocation, no exceptions, no RTTI
- **Cross-platform**: Works on ARM, ESP32, RP2040, x86 (host)
- **Template-based**: Code generation at compile-time

### Core Principles

1. **Explicit is better than implicit**
2. **Compile-time over runtime**
3. **Type safety over convenience**
4. **Zero runtime overhead**
5. **Platform independence through abstraction**

---

## 📚 Language Standard

### Required

- **C++20** minimum
- Concepts, constexpr, consteval, requires
- No exceptions (`-fno-exceptions`)
- No RTTI (`-fno-rtti`)
- No dynamic allocation (embedded targets)

### Allowed Features

```cpp
✅ Constexpr everything
✅ Templates and variadic templates
✅ SFINAE and concepts
✅ std::array, std::span (no heap)
✅ Structured bindings
✅ Lambda expressions
✅ [[nodiscard]], [[maybe_unused]]

❌ std::vector, std::string (heap)
❌ Exceptions (throw/catch)
❌ RTTI (dynamic_cast, typeid)
❌ new/delete operators
❌ Virtual functions (prefer CRTP)
```

---

## 🏷️ Naming Conventions

### Files

```cpp
// Headers
snake_case.hpp          // C++ headers
register_map.hpp        // Generated code
peripherals.hpp         // Platform definitions

// Source
snake_case.cpp          // Implementation
startup.cpp             // Generated startup code
```

### Types

```cpp
// Classes and Structs
class GpioPin { };              // PascalCase
struct TaskControlBlock { };     // PascalCase
enum class Level { };            // PascalCase

// Type Aliases
using u32 = uint32_t;            // lowercase
using gpio_config_t = GpioConfig; // snake_case_t
```

### Variables and Functions

```cpp
// Functions
void initialize_system();        // snake_case
auto get_priority() const;       // snake_case

// Variables
int counter = 0;                 // snake_case
const char* device_name;         // snake_case

// Constants
constexpr uint32_t MAX_TASKS = 8;     // UPPER_CASE
constexpr auto BUFFER_SIZE = 256;     // UPPER_CASE

// Member variables
class Foo {
    int value_;              // snake_case with trailing underscore
    static int count;        // snake_case (static members)
};
```

### Namespaces

```cpp
namespace alloy { }              // snake_case
namespace alloy::core { }        // nested snake_case
namespace alloy::hal::stm32 { }  // platform-specific
```

### Templates

```cpp
template<typename T>             // PascalCase
template<size_t Size>            // PascalCase
template<auto Value>             // PascalCase

// Concepts
template<typename T>
concept Peripheral = requires(T t) { };  // PascalCase
```

---

## 🎨 Code Formatting

### Tool: clang-format

**Configuration**: `.clang-format` at repository root

```bash
# Format single file
clang-format -i file.cpp

# Format all files
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Check formatting (CI/CD)
clang-format --dry-run -Werror file.cpp
```

### Key Rules

```cpp
// Indentation: 4 spaces
void foo() {
    if (condition) {
        do_something();
    }
}

// Line length: 100 characters (soft limit)
// Break long lines at logical points

// Braces: K&R style for functions, same line for control flow
void function()          // Opening brace on new line
{
    if (x) {            // Opening brace same line
        // ...
    }
}

// Pointer alignment: right
int* ptr;                // NOT: int *ptr
const char* str;

// Include order
#include "project/header.hpp"     // Project headers first
#include <system/header.hpp>      // System headers second
#include <cstdint>                 // C++ standard library last
```

---

## ⚡ Modern C++ Features

### Use constexpr Everywhere

```cpp
// ✅ Good
constexpr uint32_t calculate_mask(uint8_t bits) {
    return (1u << bits) - 1;
}

// ❌ Bad
uint32_t calculate_mask(uint8_t bits) {
    return (1u << bits) - 1;
}
```

### Use auto Appropriately

```cpp
// ✅ Good - obvious type
auto value = 42u;                        // uint32_t
auto* ptr = get_pointer();               // Pointer type explicit

// ✅ Good - complex type
auto result = peripheral.read_register();

// ❌ Bad - unclear type
auto x = get_something();                // What is x?
```

### Use [[nodiscard]] for Important Returns

```cpp
[[nodiscard]] bool initialize();         // Must check return value
[[nodiscard]] auto read() const -> uint32_t;
```

### Use Concepts for Template Constraints

```cpp
template<typename T>
concept GpioPeripheral = requires(T t) {
    { t.set_high() } -> std::same_as<void>;
    { t.set_low() } -> std::same_as<void>;
    { t.read() } -> std::convertible_to<bool>;
};

template<GpioPeripheral T>
void blink(T& gpio) {
    gpio.set_high();
    gpio.set_low();
}
```

---

## 🔧 Embedded Best Practices

### No Dynamic Allocation

```cpp
// ✅ Good - static/stack allocation
std::array<Task, 8> tasks;
constexpr size_t BUFFER_SIZE = 256;
uint8_t buffer[BUFFER_SIZE];

// ❌ Bad - heap allocation
std::vector<int> data;              // Uses new/delete
auto* ptr = new int[10];            // Explicit heap allocation
```

### Use Fixed-Size Types

```cpp
// ✅ Good
#include <cstdint>
uint32_t register_value;
int16_t temperature;

// ❌ Bad
int value;          // Platform-dependent size
long count;         // Platform-dependent size
```

### Volatile for Hardware Registers

```cpp
// ✅ Good
volatile uint32_t* const GPIOA_ODR = reinterpret_cast<volatile uint32_t*>(0x40020014);

// Access pattern
*GPIOA_ODR = value;               // Write
uint32_t val = *GPIOA_ODR;        // Read

// ❌ Bad - no volatile
uint32_t* GPIOA_ODR = reinterpret_cast<uint32_t*>(0x40020014);
```

### Memory-Mapped I/O

```cpp
// ✅ Good - structured access
struct GPIORegisters {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    // ...
};

auto* const GPIOA = reinterpret_cast<GPIORegisters*>(0x40020000);
GPIOA->MODER = 0x12345678;

// Better - type-safe wrapper
namespace gpio {
    template<uintptr_t BaseAddress>
    class Port {
        volatile uint32_t* const moder_ =
            reinterpret_cast<volatile uint32_t*>(BaseAddress);
    public:
        void set_mode(uint8_t pin, Mode mode) { /* ... */ }
    };
}
```

### Interrupt Handlers

```cpp
// ✅ Good - extern "C" linkage
extern "C" void USART1_IRQHandler() {
    // Handle interrupt
}

// Weak attribute for default handler
extern "C" __attribute__((weak)) void Default_Handler() {
    while (true) {
        __asm__ volatile("nop");
    }
}
```

---

## 🤖 Generated Code

### Startup Files (startup.cpp)

**Generator**: `tools/codegen/cli/generators/generate_startup.py`

**Rules**:
- Auto-generated from CMSIS-SVD
- **DO NOT EDIT** manually
- Regenerate with: `python3 codegen.py generate --startup`

**Format**:
```cpp
/// Auto-generated startup code for DEVICE
/// Generated by Alloy Code Generator from CMSIS-SVD
///
/// DO NOT EDIT - Regenerate from SVD if needed

// Weak handlers (NOT using alias - macOS compatible)
extern "C" __attribute__((weak)) void IRQ_Handler() {
    Default_Handler();
}

// Const correctness
const uint32_t* src = &_sidata;

// Auto pointer qualification
for (auto* ctor = __init_array_start; ctor < __init_array_end; ++ctor) {
    (*ctor)();
}
```

### Register Maps

**Generator**: SVD-based register generation

**Format**:
```cpp
namespace alloy::generated::stm32f4 {
    struct GPIOA_Registers {
        volatile uint32_t MODER;    // Offset: 0x00
        volatile uint32_t OTYPER;   // Offset: 0x04
        // ...
    };
}
```

### Platform HAL

**Generator**: Template-based generation from JSON metadata

**Format**:
```cpp
// Generated from metadata/platform/stm32f4_gpio.json
namespace alloy::hal::stm32f4 {
    template<uint8_t PortNum>
    class Gpio {
        // Type-safe zero-overhead abstraction
    };
}
```

---

## 📝 Documentation

### Header Comments

```cpp
/// @file gpio.hpp
/// @brief GPIO peripheral abstraction for STM32F4
///
/// Provides zero-overhead GPIO operations with compile-time
/// pin validation and type safety.
```

### Class/Function Documentation

```cpp
/// Configure GPIO pin mode
///
/// @param pin Pin number (0-15)
/// @param mode Pin mode (Input/Output/AF/Analog)
/// @returns Result::Ok on success
///
/// @example
/// ```cpp
/// gpio.configure(5, Mode::Output);
/// ```
[[nodiscard]] auto configure(uint8_t pin, Mode mode) -> Result<void>;
```

### Inline Comments

```cpp
// Use for clarification, not for obvious code
uint32_t mask = (1u << bits) - 1;  // Create bitmask

// Avoid stating the obvious
i++;  // Increment i  ❌ Bad
```

---

## 🧪 Testing

### Unit Tests

```cpp
// tests/unit/test_gpio.cpp
#include <catch2/catch.hpp>
#include "hal/gpio.hpp"

TEST_CASE("GPIO pin configuration", "[gpio]") {
    SECTION("Set pin high") {
        // Arrange
        auto gpio = create_test_gpio();

        // Act
        gpio.set_high(5);

        // Assert
        REQUIRE(gpio.read(5) == true);
    }
}
```

### Integration Tests

```cpp
// tests/integration/test_rtos.cpp
TEST_CASE("Task scheduling", "[rtos][integration]") {
    // Test real RTOS behavior
}
```

---

## 🛠️ Tooling

### Clang-Tidy

**Configuration**: Hierarchical `.clang-tidy` files

```yaml
# Root: Strict checks
# src/: Balanced for production
# src/hal/vendors/: Lenient for generated
# tests/: Permissive for tests
```

**Run**:
```bash
# Single file
clang-tidy file.cpp -- -std=c++20

# All startup files
bash tools/codegen/scripts/validate_clang_tidy.sh

# Everything
bash tools/codegen/scripts/validate_all_clang_tidy.sh
```

### Disabled Checks (Embedded-Specific)

```yaml
-cppcoreguidelines-avoid-c-arrays        # Vector tables must be C arrays
-cppcoreguidelines-pro-type-reinterpret-cast  # Hardware register access
-cppcoreguidelines-pro-type-vararg       # printf-style logging
-hicpp-no-assembler                      # Inline assembly required
-bugprone-reserved-identifier            # Linker symbols (_sidata, etc.)
```

### Pre-commit Checks

```bash
# Format check
clang-format --dry-run -Werror src/**/*.{cpp,hpp}

# Tidy check
bash tools/codegen/scripts/validate_clang_tidy.sh

# Build
cmake --build build --target all

# Test
cmake --build build --target test
```

---

## 📦 Project Structure

```
alloy/
├── src/                      # Source code
│   ├── core/                 # Core utilities
│   ├── hal/                  # Hardware abstraction layer
│   │   ├── interface/        # Platform-independent interfaces
│   │   ├── platform/         # Platform-specific (generated)
│   │   └── vendors/          # Vendor-specific (generated)
│   ├── rtos/                 # Real-time OS
│   └── logger/               # Logging system
├── tests/                    # Tests
│   ├── unit/                 # Unit tests
│   ├── integration/          # Integration tests
│   └── compile_tests/        # Compile-time tests
├── tools/                    # Tooling
│   └── codegen/              # Code generator
│       ├── cli/              # Generator implementation
│       ├── templates/        # Jinja2 templates
│       ├── scripts/          # Validation scripts
│       └── docs/             # Generator documentation
├── boards/                   # Board definitions
├── .clang-format             # Format configuration
├── .clang-tidy               # Tidy configuration (root)
└── CODING_STYLE.md           # This file
```

---

## ✅ Checklist for Contributors

Before submitting code:

- [ ] Code follows naming conventions
- [ ] clang-format passes: `clang-format --dry-run -Werror`
- [ ] clang-tidy passes: `bash tools/codegen/scripts/validate_clang_tidy.sh`
- [ ] All tests pass: `cmake --build build --target test`
- [ ] Documentation added for new features
- [ ] No warnings in compilation
- [ ] Tested on target hardware (if applicable)

---

## 🤝 For AI Assistants

When generating code for Alloy:

1. **Always use C++20 features** (constexpr, concepts, auto)
2. **Follow naming**: snake_case for functions/variables, PascalCase for types
3. **No heap allocation**: Use std::array, not std::vector
4. **No exceptions**: Use Result<T> for error handling
5. **Template-heavy**: Prefer compile-time to runtime
6. **Document**: Use /// Doxygen comments
7. **Format**: Run clang-format before committing
8. **Validate**: Run clang-tidy to check compliance

### Code Generation Patterns

```cpp
// ✅ Correct pattern for embedded HAL
template<typename Config>
class Peripheral {
    static constexpr auto BASE = Config::BASE_ADDRESS;

    [[nodiscard]] auto read() const -> uint32_t {
        return *reinterpret_cast<volatile uint32_t*>(BASE);
    }

    void write(uint32_t value) {
        *reinterpret_cast<volatile uint32_t*>(BASE) = value;
    }
};
```

---

## 📚 References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Embedded C++ Guidelines](https://www.autosar.org/)
- [LLVM Coding Standards](https://llvm.org/docs/CodingStandards.html)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

---

**Questions?** Open an issue or discussion on GitHub.

**Updates to this guide?** Submit a PR with rationale.
