# Architectural Consolidation Design

## Overview

This document outlines the architectural decisions for consolidating the CoreZero/Alloy framework into a coherent, maintainable structure while preserving all existing functionality.

## 1. Directory Structure Unification

### Current State (Problematic)

```
src/hal/
├── vendors/          # Generated code: registers, bitfields, policies
│   ├── st/
│   │   ├── stm32f4/
│   │   ├── stm32f7/
│   │   └── stm32g0/
│   └── arm/
│       └── same70/
└── platform/         # Hand-written implementations
    ├── st/
    │   ├── stm32f4/
    │   ├── stm32f7/
    │   └── stm32g0/
    └── same70/
```

**Problems:**
- Unclear boundary between "vendor" and "platform"
- Generated and hand-written code intermixed
- Developers don't know where to add new code
- Difficult to apply code generation updates

### Proposed Structure (Solution)

```
src/hal/
└── vendors/          # Single source of truth for platform code
    ├── st/           # Vendor name
    │   ├── common/   # Shared across all STM32
    │   │   ├── cortex_m_common.hpp
    │   │   ├── stm32_clock_common.hpp
    │   │   └── stm32_gpio_common.hpp
    │   ├── stm32f4/  # MCU family
    │   │   ├── generated/          # 🔵 MARKER: Auto-generated
    │   │   │   ├── registers/
    │   │   │   │   ├── rcc_registers.hpp
    │   │   │   │   └── gpio_registers.hpp
    │   │   │   └── bitfields/
    │   │   │       ├── rcc_bitfields.hpp
    │   │   │       └── gpio_bitfields.hpp
    │   │   ├── stm32f401/          # Specific MCU
    │   │   │   ├── peripherals.hpp
    │   │   │   └── startup.cpp
    │   │   ├── clock_platform.hpp  # Hand-written
    │   │   ├── gpio.hpp            # Hand-written
    │   │   └── uart.hpp            # Hand-written
    │   ├── stm32f7/
    │   └── stm32g0/
    └── arm/
        └── same70/
            ├── generated/
            ├── clock_platform.hpp
            └── gpio.hpp
```

**Benefits:**
- ✅ Clear separation: `/generated/` subdirectories mark auto-generated code
- ✅ Single location for each platform
- ✅ Shared code in `/common/` reduces duplication
- ✅ Code generation can safely overwrite `/generated/` directories
- ✅ Developers know to edit files outside `/generated/`

### Migration Strategy

**Step 1: Identify Files**
```bash
# List all files in platform/ and vendors/
find src/hal/platform -type f > platform_files.txt
find src/hal/vendors -type f > vendor_files.txt

# Classify by generation status
rg "AUTO-GENERATED|DO NOT EDIT" src/hal/platform >> generated_files.txt
rg "AUTO-GENERATED|DO NOT EDIT" src/hal/vendors >> generated_files.txt
```

**Step 2: Create New Structure**
```bash
# Create new hierarchy
mkdir -p src/hal/vendors/st/{common,stm32f4,stm32f7,stm32g0}
mkdir -p src/hal/vendors/st/stm32f4/generated/{registers,bitfields}
mkdir -p src/hal/vendors/st/stm32f7/generated/{registers,bitfields}
mkdir -p src/hal/vendors/st/stm32g0/generated/{registers,bitfields}
```

**Step 3: Move Files**
```bash
# Move generated files to /generated/ subdirs
mv src/hal/vendors/st/stm32f4/registers/* src/hal/vendors/st/stm32f4/generated/registers/
mv src/hal/vendors/st/stm32f4/bitfields/* src/hal/vendors/st/stm32f4/generated/bitfields/

# Move hand-written files from platform/ to vendors/
mv src/hal/platform/st/stm32f4/clock_platform.hpp src/hal/vendors/st/stm32f4/
mv src/hal/platform/st/stm32f4/gpio.hpp src/hal/vendors/st/stm32f4/
```

**Step 4: Update Includes**
```bash
# Update all #include statements
rg -l '#include "hal/platform/' | xargs sed -i 's|hal/platform/|hal/vendors/|g'
```

**Step 5: Remove Old Directories**
```bash
# After verification, remove old platform/
rm -rf src/hal/platform/
```

**Step 6: Update CMake**
```cmake
# Update CMakeLists.txt to reference new structure
set(HAL_VENDOR_DIR ${CMAKE_SOURCE_DIR}/src/hal/vendors/${VENDOR_NAME})
set(HAL_MCU_DIR ${HAL_VENDOR_DIR}/${MCU_FAMILY})

# Add generated files explicitly (no GLOB)
set(HAL_GENERATED_SOURCES
    ${HAL_MCU_DIR}/generated/registers/rcc_registers.hpp
    ${HAL_MCU_DIR}/generated/registers/gpio_registers.hpp
    # ... explicit list
)
```

**Validation:**
- ✅ All examples must build
- ✅ All examples must run on hardware
- ✅ No new warnings or errors
- ✅ Binary size unchanged (±1%)

---

## 2. Naming Standardization

### Decision: Use "Alloy" as Canonical Name

**Rationale:**
- Shorter (5 vs 8 characters)
- Unique (CoreZero has many GitHub results)
- Already used in CMake targets and namespaces
- "Alloy" metaphor: mixture of metals → mixture of HAL abstractions

### Changes Required

**1. Project Root**
```bash
# Rename repository (GitHub/GitLab)
corezero → alloy

# Update README title
s/CoreZero/Alloy/g in README.md
```

**2. CMake Targets**
```cmake
# Already uses "alloy" - no change needed
project(alloy VERSION 0.1.0 LANGUAGES CXX C ASM)
add_library(alloy_hal ...)
```

**3. Namespaces**
```cpp
// Current: Already uses "alloy" - good!
namespace alloy::hal::gpio { ... }
namespace alloy::rtos { ... }
```

**4. Documentation**
```bash
# Update all docs
rg -l 'CoreZero' docs/ | xargs sed -i 's/CoreZero/Alloy/g'
```

**5. File Headers**
```cpp
// Before:
/**
 * @file gpio.hpp
 * @brief CoreZero GPIO abstraction
 */

// After:
/**
 * @file gpio.hpp
 * @brief Alloy GPIO abstraction
 */
```

**6. Macro Prefixes**
```cpp
// Before:
#define COREZERO_VERSION_MAJOR 0
#ifndef COREZERO_HAL_GPIO_HPP

// After:
#define ALLOY_VERSION_MAJOR 0
#ifndef ALLOY_HAL_GPIO_HPP
```

**Migration Script:**
```bash
#!/bin/bash
# rename_to_alloy.sh

# Update source files
find src -type f \( -name "*.hpp" -o -name "*.cpp" \) -exec sed -i \
    -e 's/CoreZero/Alloy/g' \
    -e 's/COREZERO_/ALLOY_/g' \
    {} \;

# Update documentation
find docs -type f -name "*.md" -exec sed -i 's/CoreZero/Alloy/g' {} \;

# Update README
sed -i 's/CoreZero/Alloy/g' README.md

# Update examples
find examples -type f -name "*.cpp" -exec sed -i 's/CoreZero/Alloy/g' {} \;

echo "✅ Renamed CoreZero → Alloy"
```

**Validation:**
- ✅ All files compile
- ✅ No "CoreZero" in source (except historical comments)
- ✅ Documentation consistent

---

## 3. Board Abstraction Fix

### Current Problem

Boards use `#ifdef` ladders in supposedly portable code:

```cpp
// boards/nucleo_f401re/board.cpp - PROBLEMATIC
void init() {
    #ifdef STM32F4
        stm32f4::Clock::init();
        stm32f4::GPIO::enable_port_clock(GPIOA);
    #endif

    #ifdef SAME70
        same70::Clock::init();
        same70::PIO::enable_clock(PIOA);
    #endif
}
```

**Problems:**
- Board code is NOT portable
- Adding new platform requires editing all boards
- Breaks compile-time abstraction promise

### Solution: Policy-Based Board Layer

Each board defines platform-specific types, implementation is generic:

```cpp
// boards/nucleo_f401re/board_config.hpp
namespace board {
    // Platform-specific type aliases
    using ClockPlatform = stm32f4::Clock<84000000>;
    using GpioPlatform = stm32f4::GPIO;
    using UartPlatform = stm32f4::UART;

    // SysTick configuration
    using BoardSysTick = SysTick<ClockPlatform::system_clock_hz>;

    // Pin definitions
    using LedPin = GpioPin<GpioPlatform, GPIOA, 5>;
    using UartTxPin = GpioPin<GpioPlatform, GPIOA, 2>;
}
```

```cpp
// boards/nucleo_f401re/board.cpp - GENERIC, NO #ifdef!
namespace board {

void init() {
    // Generic code - uses policy types
    ClockPlatform::init();

    // Configure LED pin
    LedPin::configure(OutputMode::PushPull, Speed::Low);

    // Configure UART
    UartTxPin::configure_alternate(7);  // AF7 = USART2
}

} // namespace board
```

**Benefits:**
- ✅ Board .cpp files have ZERO platform-specific code
- ✅ All platform details in board_config.hpp type aliases
- ✅ Adding new platform = update board_config.hpp only
- ✅ Compile-time type checking

### Migration Path

**Step 1: Extract Platform Types**

For each board, create `board_config.hpp`:

```cpp
// Template for board_config.hpp
#pragma once

#include "hal/vendors/<vendor>/<family>/clock.hpp"
#include "hal/vendors/<vendor>/<family>/gpio.hpp"
#include "hal/core/types.hpp"

namespace board {

// Clock configuration
using ClockPlatform = <vendor>::<family>::Clock<CLOCK_HZ>;

// Peripheral policies
using GpioPlatform = <vendor>::<family>::GPIO;
using UartPlatform = <vendor>::<family>::UART;
using I2cPlatform = <vendor>::<family>::I2C;
using SpiPlatform = <vendor>::<family>::SPI;

// Timing
using BoardSysTick = SysTick<ClockPlatform::system_clock_hz>;

// Pin definitions
using LedPin = GpioPin<GpioPlatform, PORT, PIN>;
// ... more pins

} // namespace board
```

**Step 2: Refactor board.cpp**

Remove all `#ifdef` blocks:

```cpp
// Before:
void init() {
    #ifdef STM32F4
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    #endif
}

// After:
void init() {
    GpioPlatform::enable_port_clock(GPIOA);
}
```

**Step 3: Validate**

```bash
# Build all boards
for board in nucleo_f401re nucleo_f722ze nucleo_g071rb nucleo_g0b1re same70_xplained; do
    cmake -B build-${board} -DBOARD=${board}
    cmake --build build-${board}
done
```

---

## 4. Code Generation Consolidation

### Current State (Sprawl)

```
tools/codegen/cli/generators/
├── generate_stm32_registers.py      # 450 lines
├── generate_stm32f4_registers.py    # 380 lines
├── generate_stm32f7_registers.py    # 380 lines
├── generate_stm32g0_registers.py    # 380 lines
├── generate_same70_registers.py     # 420 lines
├── generate_gpio_policies.py        # 320 lines
├── generate_clock_configs.py        # 280 lines
├── generate_uart_configs.py         # 260 lines
├── svd_parser.py                    # 580 lines (core)
└── templates/                       # 15+ Jinja2 templates
```

**Problems:**
- 95% code duplication
- Inconsistent output formats
- Hard to maintain
- Different bugs in each generator

### Proposed Structure (Unified)

```
tools/codegen/
├── codegen.py                       # Main entry point (200 lines)
├── core/
│   ├── svd_parser.py               # SVD → AST (600 lines)
│   ├── generator_base.py           # Common generation logic (400 lines)
│   └── output_writer.py            # File writing utilities (200 lines)
├── generators/
│   ├── register_generator.py       # Registers (300 lines)
│   ├── bitfield_generator.py       # Bitfields (250 lines)
│   ├── peripheral_generator.py     # Peripherals (300 lines)
│   └── policy_generator.py         # Hardware policies (350 lines)
└── templates/                      # 8 templates (consolidated)
    ├── register.hpp.j2
    ├── bitfield.hpp.j2
    ├── peripheral.hpp.j2
    └── policy.hpp.j2
```

**Code Reduction:**
- Before: ~3,450 lines across 8+ scripts
- After: ~2,200 lines in unified system
- **Savings: 36% reduction + consistency**

### Unified Generation Interface

```bash
# Single command to generate all
python tools/codegen/codegen.py \
    --svd data/stm32f4.svd \
    --vendor st \
    --family stm32f4 \
    --output src/hal/vendors/st/stm32f4/generated/

# Generates:
# - registers/*.hpp
# - bitfields/*.hpp
# - peripherals.hpp
# - gpio_hardware_policy.hpp
# - uart_hardware_policy.hpp
```

---

## 5. CMake Build System Modernization

### Problem: GLOB Anti-Pattern

```cmake
# PROBLEMATIC - current approach
file(GLOB_RECURSE HAL_SOURCES "src/hal/*.cpp")
file(GLOB_RECURSE VENDOR_SOURCES "src/hal/vendors/${VENDOR}/*.cpp")
```

**Issues:**
- CMake doesn't detect new files (must re-run cmake)
- Slow (scans filesystem every build)
- Includes unwanted files
- No control over link order

### Solution: Explicit Source Lists

```cmake
# src/hal/CMakeLists.txt - EXPLICIT LISTS

set(HAL_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/result.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/interrupt.cpp
)

set(HAL_VENDOR_SOURCES_STM32F4
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f4/clock_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f4/gpio_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/st/stm32f4/uart_platform.cpp
    # ... explicit list, easier to review in PRs
)

# Conditional platform sources
if(MCU_FAMILY STREQUAL "stm32f4")
    set(HAL_PLATFORM_SOURCES ${HAL_VENDOR_SOURCES_STM32F4})
elseif(MCU_FAMILY STREQUAL "stm32f7")
    set(HAL_PLATFORM_SOURCES ${HAL_VENDOR_SOURCES_STM32F7})
endif()

add_library(alloy_hal STATIC
    ${HAL_CORE_SOURCES}
    ${HAL_PLATFORM_SOURCES}
)
```

**Benefits:**
- ✅ Fast incremental builds
- ✅ Explicit dependencies
- ✅ Better PR reviews (see what files added)
- ✅ Controlled link order

### Validation Targets

```cmake
# Add validation target
add_custom_target(validate-build-system
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/validate_sources.cmake
    COMMENT "Validating CMake source lists..."
)

# cmake/validate_sources.cmake
# Check that all .cpp files are listed
file(GLOB_RECURSE ALL_CPP_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
foreach(cpp_file ${ALL_CPP_FILES})
    # Check if file is in HAL_SOURCES or HAL_PLATFORM_SOURCES
    # Error if orphaned
endforeach()
```

---

## 6. API Standardization with C++20 Concepts

### Problem: Inconsistent Interfaces

Different platforms have different Clock APIs:

```cpp
// STM32F4
namespace stm32f4 {
    struct Clock {
        static void init();  // No parameters
        static constexpr u32 system_clock_hz = 84000000;
    };
}

// STM32F7
namespace stm32f7 {
    template <u32 CLOCK_HZ>
    struct Clock {
        static void configure();  // Different name!
        static constexpr u32 frequency = CLOCK_HZ;  // Different member!
    };
}

// SAME70
namespace same70 {
    struct Clock {
        static Result<void, Error> initialize();  // Returns Result!
        static u32 get_frequency();  // Runtime function!
    };
}
```

**Problem:** Can't write generic code across platforms.

### Solution: ClockPlatform Concept

```cpp
// src/hal/core/concepts.hpp
template <typename T>
concept ClockPlatform = requires {
    // Must have compile-time frequency
    { T::system_clock_hz } -> std::convertible_to<u32>;

    // Must have initialization function
    { T::init() } -> std::same_as<void>;

    // Optional: Returns Result<void, Error> for error handling
    // OR just void for infallible init
};

// Validate all platforms
static_assert(ClockPlatform<stm32f4::Clock<84000000>>);
static_assert(ClockPlatform<stm32f7::Clock<216000000>>);
static_assert(ClockPlatform<same70::Clock<300000000>>);
```

### Standardized APIs

**All platforms must implement:**

```cpp
// Clock Platform
template <u32 FREQ_HZ>
struct ClockPlatform {
    static constexpr u32 system_clock_hz = FREQ_HZ;
    static void init();  // Initialize clocks
};

// GPIO Platform
template <typename Policy>
struct GpioPlatform {
    template <u32 PORT, u8 PIN>
    static void configure_output(OutputMode mode, Speed speed);

    template <u32 PORT, u8 PIN>
    static void set();

    template <u32 PORT, u8 PIN>
    static void clear();
};

// UART Platform
template <typename Policy>
struct UartPlatform {
    static Result<void, Error> init(u32 baud_rate);
    static Result<void, Error> write(const u8* data, usize len);
    static Result<usize, Error> read(u8* buffer, usize len, u32 timeout_ms);
};
```

**Concept Validation:**

```cpp
template <typename T>
concept GpioPlatform = requires(T gpio) {
    { T::template configure_output<GPIOA, 5>(OutputMode::PushPull, Speed::Low) };
    { T::template set<GPIOA, 5>() };
    { T::template clear<GPIOA, 5>() };
};
```

**Usage in Board Code:**

```cpp
// Board uses concept-validated types
using ClockImpl = stm32f4::Clock<84000000>;
static_assert(ClockPlatform<ClockImpl>, "Invalid clock platform!");

void init() {
    ClockImpl::init();  // Guaranteed to exist by concept
}
```

---

## 7. Testing Strategy

### Test Hierarchy

```
tests/
├── unit/                      # Unit tests (host)
│   ├── test_result.cpp
│   ├── test_gpio_api.cpp
│   └── test_clock_api.cpp
├── integration/               # Integration tests (hardware)
│   ├── test_gpio_timing.cpp
│   ├── test_uart_loopback.cpp
│   └── test_rtos_scheduler.cpp
└── regression/                # Regression suite
    ├── test_binary_size.cpp
    └── test_compile_time.cpp
```

### Validation Points

After each consolidation phase:

1. **Compile Test**: All boards must build
2. **Size Test**: Binary size within ±1% of baseline
3. **Runtime Test**: All examples run on hardware
4. **Regression Test**: Performance unchanged

```cmake
# Automated validation
add_test(NAME compile_all_boards
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/test_all_boards.sh
)

add_test(NAME binary_size_regression
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/check_binary_size.sh
)
```

---

## 8. Migration Timeline

### Week 1: Directory Consolidation
- **Mon-Tue**: Merge `platform/` into `vendors/`
- **Wed**: Update CMake includes
- **Thu**: Validate all boards build
- **Fri**: Test on hardware

### Week 2: Naming & Board Abstraction
- **Mon-Tue**: Rename CoreZero → Alloy
- **Wed-Thu**: Refactor board layer (remove #ifdef)
- **Fri**: Validation

### Week 3: Code Generation
- **Mon-Wed**: Consolidate generators
- **Thu**: Re-generate all platforms
- **Fri**: Validation

### Week 4: API Standardization
- **Mon-Tue**: Define concepts
- **Wed-Thu**: Standardize APIs
- **Fri**: Full regression test

---

## 9. Rollback Plan

Each phase is atomic and reversible:

```bash
# Create checkpoint before each phase
git tag checkpoint-phase-1-start
git tag checkpoint-phase-1-complete

# If issues arise, rollback
git reset --hard checkpoint-phase-1-start
```

**Validation Gate:** Each phase must pass ALL tests before proceeding to next phase.

---

## 10. Success Metrics

### Code Quality Metrics
- **Directory Depth**: Max 4 levels (currently 6)
- **Code Duplication**: <5% across families (currently ~35%)
- **Build Time**: No regression
- **Binary Size**: ±1% tolerance

### Documentation Metrics
- **README Accuracy**: 100% match with reality
- **API Coverage**: All public APIs documented
- **Tutorial Completion**: New developer onboards in <1 hour

### Maintainability Metrics
- **PRs to Add Platform**: <50 lines changed
- **Code Generation Time**: <5 seconds per platform
- **Build Configuration**: <10 CMake lines per board
