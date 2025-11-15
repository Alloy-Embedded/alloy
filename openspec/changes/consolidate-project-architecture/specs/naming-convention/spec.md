# Naming Convention Standardization Specification

## MODIFIED Requirements

### Requirement: Canonical Project Name

The project SHALL use "Alloy" as the canonical name throughout all code, documentation, and build system.

**Rationale**: Eliminates confusion from dual naming (CoreZero/Alloy) and provides shorter, more unique identifier.

#### Scenario: Source code uses Alloy

- **GIVEN** any source file in the project
- **WHEN** file is examined
- **THEN** project references SHALL use "Alloy"
- **AND** file headers SHALL reference "Alloy Framework"
- **AND** no references to "CoreZero" SHALL exist (except historical comments)

```cpp
/**
 * @file gpio.hpp
 * @brief Alloy Framework - GPIO HAL abstraction
 * @copyright Copyright (c) 2025 Alloy Framework Contributors
 */

namespace alloy::hal::gpio {
    // Implementation
}
```

#### Scenario: Macro prefixes use ALLOY_

- **GIVEN** header guards or feature macros
- **WHEN** macro is defined
- **THEN** prefix SHALL be `ALLOY_`
- **AND** prefix SHALL NOT be `COREZERO_`

```cpp
// CORRECT
#ifndef ALLOY_HAL_GPIO_HPP
#define ALLOY_HAL_GPIO_HPP

#define ALLOY_VERSION_MAJOR 1
#define ALLOY_VERSION_MINOR 0

#ifdef ALLOY_RTOS_ENABLED
    // RTOS code
#endif

#endif // ALLOY_HAL_GPIO_HPP

// INCORRECT
#ifndef COREZERO_HAL_GPIO_HPP  // ✗ Wrong prefix
```

#### Scenario: CMake project name

- **GIVEN** root CMakeLists.txt
- **WHEN** project() is defined
- **THEN** project name SHALL be "alloy"
- **AND** targets SHALL use `alloy_` prefix

```cmake
project(alloy VERSION 1.0.0 LANGUAGES CXX C ASM)

add_library(alloy_hal STATIC ${HAL_SOURCES})
add_library(alloy_rtos STATIC ${RTOS_SOURCES})
add_executable(alloy_example_blink ${EXAMPLE_SOURCES})
```

#### Scenario: Documentation references

- **GIVEN** README, guides, or documentation
- **WHEN** project is mentioned
- **THEN** name SHALL be "Alloy"
- **AND** framework SHALL be "Alloy Framework"

```markdown
# Alloy Framework

Alloy is a modern C++20 framework for bare-metal embedded systems...
```

---

### Requirement: Namespace Consistency

All code SHALL use `alloy` as the root namespace.

**Rationale**: Already implemented correctly; this requirement codifies existing practice.

#### Scenario: HAL uses alloy::hal namespace

- **GIVEN** HAL component code
- **WHEN** namespace is defined
- **THEN** namespace SHALL be `alloy::hal::<component>`

```cpp
namespace alloy::hal::gpio {
    // GPIO code
}

namespace alloy::hal::uart {
    // UART code
}

namespace alloy::hal::i2c {
    // I2C code
}
```

#### Scenario: RTOS uses alloy::rtos namespace

- **GIVEN** RTOS code
- **WHEN** namespace is defined
- **THEN** namespace SHALL be `alloy::rtos`

```cpp
namespace alloy::rtos {
    class Task { /* ... */ };
    class Mutex { /* ... */ };
    class Queue { /* ... */ };
}
```

#### Scenario: Core types use alloy::core namespace

- **GIVEN** core types and utilities
- **WHEN** namespace is defined
- **THEN** namespace SHALL be `alloy::core`

```cpp
namespace alloy::core {
    template <typename T, typename E>
    class Result { /* ... */ };

    using u8 = uint8_t;
    using u32 = uint32_t;
}
```

---

## REMOVED Requirements

### Requirement: CoreZero naming (REMOVED)

All references to "CoreZero" have been removed and replaced with "Alloy".

**Migration**: Automated sed script replaced all instances:
- `s/CoreZero/Alloy/g`
- `s/COREZERO_/ALLOY_/g`
