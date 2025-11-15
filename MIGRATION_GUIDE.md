# Migration Guide - Alloy Framework

This guide helps you migrate existing code to the latest version of Alloy Framework.

## Table of Contents

- [Overview](#overview)
- [Project Rename: CoreZero → Alloy](#project-rename-corezero--alloy)
- [Directory Structure Changes](#directory-structure-changes)
- [API Changes](#api-changes)
- [Build System Changes](#build-system-changes)
- [Code Generation Changes](#code-generation-changes)
- [Testing Changes](#testing-changes)
- [Migration Checklist](#migration-checklist)

---

## Overview

The Alloy Framework (formerly CoreZero) has undergone significant consolidation and modernization. This guide covers all breaking changes and provides step-by-step migration instructions.

**Major Changes**:
- ✅ Project renamed from CoreZero to Alloy
- ✅ Directory structure consolidated (platform/ → vendors/)
- ✅ API standardization with C++20 concepts
- ✅ Unified code generation system
- ✅ Comprehensive test infrastructure
- ✅ CI/CD integration

---

## Project Rename: CoreZero → Alloy

### What Changed

The project has been renamed from **CoreZero** to **Alloy Framework**.

### Migration Steps

#### 1. Update Project References

**Before**:
```cmake
project(corezero VERSION 1.0.0)
```

**After**:
```cmake
project(alloy VERSION 1.0.0)
```

#### 2. Update Namespaces

Namespaces already use `alloy::` - **no changes needed**.

```cpp
// Already correct - no migration needed
using namespace alloy::core;
using namespace alloy::hal;
```

#### 3. Update Macros

**Before** (if you used any):
```cpp
#define COREZERO_VERSION "1.0.0"
#ifdef COREZERO_DEBUG
```

**After**:
```cpp
#define ALLOY_VERSION "1.0.0"
#ifdef ALLOY_DEBUG
```

#### 4. Update Documentation

Replace all references to "CoreZero" with "Alloy" in:
- README files
- Code comments
- Documentation
- Build scripts

---

## Directory Structure Changes

### What Changed

Generated files moved from root directories to `/generated/` subdirectories.

**Before**:
```
src/hal/platform/st/stm32f4/
├── registers/
│   ├── gpio_registers.hpp
│   └── rcc_registers.hpp
└── bitfields/
    ├── gpio_bitfields.hpp
    └── rcc_bitfields.hpp
```

**After**:
```
src/hal/vendors/st/stm32f4/
├── generated/
│   ├── registers/
│   │   ├── gpio_registers.hpp
│   │   └── rcc_registers.hpp
│   └── bitfields/
│       ├── gpio_bitfields.hpp
│       └── rcc_bitfields.hpp
├── gpio.hpp          (hand-written)
└── clock_platform.hpp (hand-written)
```

### Migration Steps

#### 1. Update Include Paths

**Before**:
```cpp
#include "hal/platform/st/stm32f4/registers/gpio_registers.hpp"
#include "hal/platform/st/stm32f4/bitfields/gpio_bitfields.hpp"
```

**After**:
```cpp
#include "hal/vendors/st/stm32f4/generated/registers/gpio_registers.hpp"
#include "hal/vendors/st/stm32f4/generated/bitfields/gpio_bitfields.hpp"
```

#### 2. Update CMake Paths

**Before**:
```cmake
include_directories(${CMAKE_SOURCE_DIR}/src/hal/platform)
```

**After**:
```cmake
include_directories(${CMAKE_SOURCE_DIR}/src/hal/vendors)
```

#### 3. Search and Replace

Run this command to update all includes:

```bash
find src -name "*.cpp" -o -name "*.hpp" | \
  xargs sed -i 's|hal/platform/|hal/vendors/|g'

find src -name "*.cpp" -o -name "*.hpp" | \
  xargs sed -i 's|/registers/|/generated/registers/|g'

find src -name "*.cpp" -o -name "*.hpp" | \
  xargs sed -i 's|/bitfields/|/generated/bitfields/|g'
```

---

## API Changes

### Result<T, E> Type

**No breaking changes** - API is backward compatible.

### GPIO API

#### Added Methods

**New**: `write(bool)` method added for consistency

**Before**:
```cpp
led.set();    // Turn on
led.clear();  // Turn off
```

**After** (both work):
```cpp
// Old API (still works)
led.set();
led.clear();

// New API (preferred)
led.write(true);   // Turn on
led.write(false);  // Turn off
```

#### Return Type Changes

**Before**:
```cpp
auto result = led.read();  // Result<uint32_t, ErrorCode>
if (result.is_ok()) {
    uint32_t value = result.unwrap();
}
```

**After**:
```cpp
auto result = led.read();  // Result<bool, ErrorCode>
if (result.is_ok()) {
    bool value = result.unwrap();  // true or false
}
```

### Clock API

#### New Methods

Added peripheral clock enable methods:

```cpp
// New in Alloy Framework
auto result = ClockPlatform::enable_uart_clock(USART1_BASE);
auto result = ClockPlatform::enable_spi_clock(SPI1_BASE);
auto result = ClockPlatform::enable_i2c_clock(I2C1_BASE);
```

#### Return Type Changes

**Before**:
```cpp
void enable_gpio_clocks();  // No return value
```

**After**:
```cpp
auto result = ClockPlatform::enable_gpio_clocks();  // Returns Result<void, ErrorCode>
if (result.is_err()) {
    // Handle error
}
```

---

## Build System Changes

### Platform Selection

**Before**:
```bash
cmake -DALLOY_PLATFORM=host -B build
```

**After** (use board instead):
```bash
# For embedded
cmake -DALLOY_BOARD=nucleo_f401re -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake -B build

# For host testing
cmake -DALLOY_PLATFORM=linux -B build
```

### Supported Platforms

Platform names changed to be more specific:

**Before**:
- `host` (generic)
- `stm32` (generic)

**After**:
- `linux` (host platform)
- `stm32f4`, `stm32f7`, `stm32g0` (specific families)

### CMake Variables

**Updated variables**:

| Old | New | Purpose |
|-----|-----|---------|
| `COREZERO_BUILD_TESTS` | `ALLOY_BUILD_TESTS` | Enable tests |
| `COREZERO_PLATFORM` | `ALLOY_PLATFORM` | Target platform |
| `COREZERO_BOARD` | `ALLOY_BOARD` | Target board |

---

## Code Generation Changes

### Unified Generator

**Before**: Multiple separate scripts
```bash
tools/generate_stm32f4_registers.py
tools/generate_stm32f7_registers.py
tools/generate_same70_registers.py
```

**After**: Single unified generator
```bash
cd tools/codegen
python3 codegen.py generate stm32f4
python3 codegen.py generate stm32f7
python3 codegen.py generate-complete  # Generate all
```

### Generator Commands

**New command structure**:

```bash
# Generate specific platform
python3 codegen.py generate <platform>

# Generate all platforms
python3 codegen.py generate-complete

# Check status
python3 codegen.py status

# Clean generated files
python3 codegen.py clean <platform>

# List supported vendors
python3 codegen.py vendors
```

### Metadata Files

Generated files now have metadata headers:

```cpp
/**
 * @file gpio_registers.hpp
 * @brief Auto-generated GPIO register definitions for STM32F4
 *
 * DO NOT MODIFY - Generated from STM32F401.svd
 * Generated: 2025-11-15 10:30:45
 * Generator: Alloy Code Generator v2.0
 */
```

---

## Testing Changes

### Test Framework

**Changed from Google Test to Catch2 v3**.

**Before**:
```cpp
#include <gtest/gtest.h>

TEST(GpioTest, SetPin) {
    EXPECT_TRUE(pin.set().is_ok());
}
```

**After**:
```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("GPIO: Set pin", "[gpio]") {
    REQUIRE(pin.set().is_ok());
}
```

### Test Organization

**New structure**:

```
tests/
├── unit/           # Unit tests (49 tests)
├── integration/    # Integration tests (20 tests)
├── regression/     # Regression tests (31 tests)
└── hardware/       # Hardware validation tests (3 tests)
```

### Running Tests

**Before**:
```bash
./build/tests/run_tests
```

**After**:
```bash
cmake -B build-host -DALLOY_PLATFORM=linux -DALLOY_BUILD_TESTS=ON
cmake --build build-host
cd build-host && ctest --output-on-failure
```

---

## Migration Checklist

Use this checklist to ensure complete migration:

### Code Changes

- [ ] Update all `#include "hal/platform/` → `#include "hal/vendors/`
- [ ] Update generated file includes to use `/generated/` subdirectory
- [ ] Replace `COREZERO_*` macros with `ALLOY_*`
- [ ] Update GPIO `read()` to expect `Result<bool, ErrorCode>`
- [ ] Update Clock API calls to handle `Result<void, ErrorCode>`
- [ ] Add error handling for new Clock peripheral enable methods

### Build System

- [ ] Update CMake project name to `alloy`
- [ ] Replace `COREZERO_*` variables with `ALLOY_*`
- [ ] Update platform selection (`host` → `linux`)
- [ ] Update toolchain file paths if customized

### Documentation

- [ ] Replace "CoreZero" with "Alloy" in READMEs
- [ ] Update code comments referencing old structure
- [ ] Update build instructions

### Tests

- [ ] Migrate Google Test → Catch2 v3
- [ ] Update test includes
- [ ] Reorganize tests into unit/integration/regression
- [ ] Update test execution commands

### Code Generation

- [ ] Switch to unified `codegen.py` script
- [ ] Update code generation commands
- [ ] Verify generated file locations

### CI/CD

- [ ] Update workflow files with new paths
- [ ] Update artifact names
- [ ] Update build matrices

---

## Automated Migration Script

We provide a script to automate common migration tasks:

```bash
#!/bin/bash
# migrate.sh - Alloy Framework Migration Script

echo "Migrating to Alloy Framework..."

# 1. Update includes
find src -name "*.cpp" -o -name "*.hpp" | \
  xargs sed -i.bak 's|hal/platform/|hal/vendors/|g'

find src -name "*.cpp" -o -name "*.hpp" | \
  xargs sed -i.bak 's|/registers/|/generated/registers/|g'

find src -name "*.cpp" -o -name "*.hpp" | \
  xargs sed -i.bak 's|/bitfields/|/generated/bitfields/|g'

# 2. Update macros
find src -name "*.cpp" -o -name "*.hpp" -o -name "CMakeLists.txt" | \
  xargs sed -i.bak 's|COREZERO_|ALLOY_|g'

# 3. Update project name in CMakeLists.txt
sed -i.bak 's|project(corezero|project(alloy|g' CMakeLists.txt

echo "Migration complete! Backup files created with .bak extension"
echo "Review changes and test before removing .bak files"
```

---

## Need Help?

If you encounter issues during migration:

1. **Check Documentation**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
2. **Review Examples**: [examples/](examples/)
3. **Ask Questions**: [GitHub Issues](https://github.com/your-org/corezero/issues)

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| v1.0.0 | 2025-11-15 | Initial Alloy Framework release |
| (prev) | 2025-11-01 | Last CoreZero version |

---

**Last Updated**: 2025-11-15
**Maintainer**: Alloy Framework Team
