# CMake Integration Guide

The Alloy code generator integrates seamlessly with CMake to automatically generate startup code and peripheral definitions during the build configuration phase.

## Quick Start

In your board configuration file:

```cmake
# Include code generation module
include(codegen)

# Generate code for your MCU
alloy_generate_code(
    MCU STM32F103C8
    FAMILY stm32f1xx
)

# Use generated files
include_directories(${ALLOY_GENERATED_DIR})
add_executable(myapp main.cpp ${ALLOY_GENERATED_SOURCES})
```

That's it! Code is generated automatically when you run `cmake`.

## API Reference

### `alloy_generate_code()`

Generates code from MCU database.

**Signature:**
```cmake
alloy_generate_code(
    MCU <mcu_name>              # Required
    [DATABASE <path>]           # Optional
    [OUTPUT_DIR <path>]         # Optional
    [FAMILY <family_name>]      # Optional
)
```

**Parameters:**

| Parameter | Required | Description | Example |
|-----------|----------|-------------|---------|
| `MCU` | Yes | Target MCU name (must match database) | `STM32F103C8` |
| `DATABASE` | No | Path to database JSON file | `database/families/stm32f1xx.json` |
| `FAMILY` | No | Family name for auto-detection | `stm32f1xx` |
| `OUTPUT_DIR` | No | Output directory for generated files | `${CMAKE_BINARY_DIR}/gen` |

**Auto-detection:**

If `DATABASE` is not specified, the module attempts to auto-detect the database file based on MCU name:

- `STM32F103C8` → `database/families/stm32f1xx.json`
- `STM32F407VG` → `database/families/stm32f4xx.json`
- `NRF52840` → `database/families/nrf52.json`
- `RL78G14` → `database/families/rl78.json`

**Default output directory:**
```
${CMAKE_BINARY_DIR}/generated/${MCU_NAME}/
```

### Exported Variables

After calling `alloy_generate_code()`, the following variables are available:

| Variable | Type | Description | Example |
|----------|------|-------------|---------|
| `ALLOY_GENERATED_DIR` | Path | Directory containing generated files | `build/generated/STM32F103C8` |
| `ALLOY_GENERATED_SOURCES` | List | Generated `.cpp` and `.c` files | `startup.cpp` |
| `ALLOY_GENERATED_HEADERS` | List | Generated `.h` and `.hpp` files | (future) |

**Example usage:**
```cmake
alloy_generate_code(MCU STM32F103C8)

message(STATUS "Generated directory: ${ALLOY_GENERATED_DIR}")
message(STATUS "Generated sources: ${ALLOY_GENERATED_SOURCES}")

add_executable(myapp
    main.cpp
    ${ALLOY_GENERATED_SOURCES}
)

target_include_directories(myapp PRIVATE ${ALLOY_GENERATED_DIR})
```

### `alloy_validate_database()`

Validates a database JSON file.

**Signature:**
```cmake
alloy_validate_database(<database_file>)
```

**Example:**
```cmake
alloy_validate_database(database/families/stm32f1xx.json)
```

This runs `validate_database.py` and fails the build if validation fails.

## Build Behavior

### First Build

On the first build (or when no marker file exists):

```
-- Code generation: ENABLED
--   Generator: /path/to/generator.py
--   Databases: /path/to/database/families
-- Generating code for STM32F103C8 (marker file missing)...
-- Code generation complete: build/generated/STM32F103C8
-- Generated sources:
--   - build/generated/STM32F103C8/startup.cpp
```

### Subsequent Builds

Code is **cached** and only regenerated when:

1. **Database file is updated** (newer timestamp)
2. **Generator script is updated** (newer timestamp)
3. **Marker file is deleted** (`.generated`)

When cached:
```
-- Using cached generated code for STM32F103C8
```

When regenerated:
```
-- Generating code for STM32F103C8 (database updated)...
```

### Clean Build

To force regeneration:

```bash
rm -rf build/generated
cmake -B build
```

Or:
```bash
cmake -B build --fresh
```

## Integration Patterns

### Pattern 1: Board File Integration

**Best for:** Standard boards with fixed MCUs

**`cmake/boards/bluepill.cmake`:**
```cmake
set(ALLOY_MCU "STM32F103C8" CACHE STRING "MCU model")
set(ALLOY_ARCH "arm-cortex-m3" CACHE STRING "Architecture")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake")

# Code generation
include(codegen)

if(ALLOY_CODEGEN_AVAILABLE)
    alloy_generate_code(
        MCU ${ALLOY_MCU}
        FAMILY stm32f1xx
    )
    include_directories(${ALLOY_GENERATED_DIR})
endif()
```

### Pattern 2: Application-Level Integration

**Best for:** Custom applications with specific MCU choice

**`CMakeLists.txt`:**
```cmake
cmake_minimum_required(VERSION 3.25)
project(my_firmware)

set(ALLOY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../alloy")
list(APPEND CMAKE_MODULE_PATH ${ALLOY_ROOT}/cmake)

include(codegen)

# User selects MCU via command line: -DMCU_TARGET=STM32F103C8
if(NOT MCU_TARGET)
    set(MCU_TARGET "STM32F103C8" CACHE STRING "Target MCU")
endif()

alloy_generate_code(MCU ${MCU_TARGET})

add_executable(firmware
    src/main.cpp
    ${ALLOY_GENERATED_SOURCES}
)

target_include_directories(firmware PRIVATE ${ALLOY_GENERATED_DIR})
```

**Build:**
```bash
cmake -B build -DMCU_TARGET=STM32F407VG
```

### Pattern 3: Multiple MCU Targets

**Best for:** Projects supporting multiple hardware variants

**`CMakeLists.txt`:**
```cmake
# Define MCU variants
set(MCU_VARIANTS
    "STM32F103C8:stm32f1xx"
    "STM32F407VG:stm32f4xx"
    "NRF52840:nrf52"
)

if(NOT VARIANT)
    set(VARIANT "STM32F103C8:stm32f1xx")
endif()

# Parse variant
string(REPLACE ":" ";" VARIANT_LIST ${VARIANT})
list(GET VARIANT_LIST 0 MCU_NAME)
list(GET VARIANT_LIST 1 MCU_FAMILY)

include(codegen)
alloy_generate_code(
    MCU ${MCU_NAME}
    FAMILY ${MCU_FAMILY}
)

# Build for specific variant
add_executable(firmware_${MCU_NAME}
    src/main.cpp
    ${ALLOY_GENERATED_SOURCES}
)
```

**Build:**
```bash
cmake -B build_bluepill -DVARIANT="STM32F103C8:stm32f1xx"
cmake -B build_disco -DVARIANT="STM32F407VG:stm32f4xx"
```

## Dependency Tracking

The CMake integration automatically tracks dependencies:

```
Generated Code
    └── Depends on: Database JSON
    └── Depends on: generator.py
    └── Depends on: Templates
```

**Marker file (`.generated`)** contains:
```
Generated: /path/to/CMakeLists.txt
MCU: STM32F103C8
Database: /path/to/stm32f1xx.json
Timestamp: 2025-10-30T21:15:03
```

CMake compares timestamps to decide if regeneration is needed.

## Error Handling

### Python Not Found

**Error:**
```
Code generation: DISABLED (Python3 not found)
```

**Solution:**
```bash
# macOS
brew install python3

# Ubuntu/Debian
sudo apt install python3

# Windows
# Download from python.org
```

### Database Not Found

**Error:**
```
alloy_generate_code: Database file not found: /path/to/db.json
  MCU: STM32F103C8
```

**Solutions:**
1. Specify correct path:
   ```cmake
   alloy_generate_code(
       MCU STM32F103C8
       DATABASE ${CMAKE_SOURCE_DIR}/tools/codegen/database/families/stm32f1xx.json
   )
   ```

2. Use FAMILY parameter:
   ```cmake
   alloy_generate_code(
       MCU STM32F103C8
       FAMILY stm32f1xx  # Auto-finds database/families/stm32f1xx.json
   )
   ```

### MCU Not in Database

**Error:**
```
alloy_generate_code: MCU 'CUSTOM_MCU' not found in database
  Available MCUs: STM32F103C8, STM32F103CB
```

**Solution:** Check MCU name spelling or parse SVD to add MCU:
```bash
python3 tools/codegen/svd_parser.py \
    --input my_mcu.svd \
    --output database/families/my_family.json \
    --merge
```

### Generation Failed

**Error:**
```
Code generation failed for STM32F103C8!
Error: (details)
```

**Solutions:**
1. Check database validity:
   ```bash
   python3 tools/codegen/validate_database.py database/families/stm32f1xx.json
   ```

2. Run generator manually:
   ```bash
   python3 tools/codegen/generator.py \
       --mcu STM32F103C8 \
       --database database/families/stm32f1xx.json \
       --output /tmp/test \
       --verbose
   ```

3. Check generator output for specific errors

## Advanced Configuration

### Custom Output Directory

```cmake
alloy_generate_code(
    MCU STM32F103C8
    FAMILY stm32f1xx
    OUTPUT_DIR ${CMAKE_SOURCE_DIR}/generated_code
)
```

### Conditional Generation

```cmake
option(ENABLE_CODEGEN "Enable code generation" ON)

if(ENABLE_CODEGEN AND ALLOY_CODEGEN_AVAILABLE)
    alloy_generate_code(MCU ${TARGET_MCU})
else()
    # Use pre-generated code
    set(ALLOY_GENERATED_DIR ${CMAKE_SOURCE_DIR}/pregenerated)
endif()
```

### Pre-commit Hook

Generate code automatically before committing:

**`.git/hooks/pre-commit`:**
```bash
#!/bin/bash
cd tools/codegen
python3 generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx.json \
    --output ../../src/generated
```

### CI/CD Integration

**GitHub Actions:**
```yaml
- name: Generate startup code
  run: |
    cmake -B build
    # Code is generated during configure
```

**GitLab CI:**
```yaml
build:
  script:
    - cmake -B build
    - cmake --build build
```

Code generation happens automatically during `cmake -B build`.

## Best Practices

### 1. Version Control Generated Code?

**Recommendation:** No, add to `.gitignore`:

```gitignore
build/
**/generated/
.generated
```

**Why:** Generated code can always be recreated from databases.

**Exception:** If you want reproducible builds without Python dependency, commit generated code.

### 2. Cache Generated Code

✅ **Do:** Let CMake cache generated code between builds
```cmake
# Default behavior - uses marker file
alloy_generate_code(MCU STM32F103C8)
```

❌ **Don't:** Force regeneration every build
```cmake
# Bad: Always regenerates
file(REMOVE ${ALLOY_GENERATED_DIR}/.generated)
alloy_generate_code(MCU STM32F103C8)
```

### 3. Parallel Builds

Generated code is created during CMake configuration, not during build, so:

✅ **Safe for parallel builds:**
```bash
cmake -B build && cmake --build build -j8
```

### 4. Cross-Compilation

Set toolchain before including codegen:
```cmake
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchains/arm-none-eabi.cmake)
include(codegen)
alloy_generate_code(MCU STM32F103C8)
```

## Troubleshooting

### Stale Generated Code

**Symptom:** Changes to database don't appear in generated code.

**Solution:**
```bash
rm -rf build/generated
cmake -B build
```

### Permission Denied

**Symptom:** Can't write to output directory.

**Solution:** Check permissions or use different output dir:
```cmake
alloy_generate_code(
    MCU STM32F103C8
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/gen  # Use build directory
)
```

### Python Import Errors

**Symptom:**
```
ImportError: No module named 'jinja2'
```

**Solution:**
```bash
pip3 install jinja2
```

## Example: Complete CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_firmware CXX C ASM)

# Alloy configuration
set(ALLOY_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/alloy)
set(ALLOY_BOARD "bluepill" CACHE STRING "Target board")

list(APPEND CMAKE_MODULE_PATH ${ALLOY_ROOT}/cmake)

# Load board configuration (which calls alloy_generate_code)
if(EXISTS ${ALLOY_ROOT}/cmake/boards/${ALLOY_BOARD}.cmake)
    include(${ALLOY_ROOT}/cmake/boards/${ALLOY_BOARD}.cmake)
else()
    message(FATAL_ERROR "Board '${ALLOY_BOARD}' not found")
endif()

# Your application
add_executable(firmware
    src/main.cpp
    src/app.cpp
    ${ALLOY_GENERATED_SOURCES}  # Add generated startup code
)

target_include_directories(firmware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ALLOY_GENERATED_DIR}       # Include generated headers
)

target_link_libraries(firmware PRIVATE
    alloy::core
    alloy::hal
)
```

**Build:**
```bash
cmake -B build -DALLOY_BOARD=bluepill
cmake --build build
```

Code is generated automatically during configuration!
