# Unified Directory Structure Specification

## MODIFIED Requirements

### Requirement: Single Platform Directory Hierarchy

The HAL SHALL use a single, unified directory structure under `src/hal/vendors/` for all platform-specific code, eliminating the dual `platform/` and `vendors/` structure.

**Rationale**: Eliminates confusion about where to place platform-specific code and provides clear separation between generated and hand-written code.

#### Scenario: Generated code in /generated/ subdirectory

- **GIVEN** code generation system produces registers and bitfields
- **WHEN** code is generated for a platform
- **THEN** output SHALL be placed in `vendors/<vendor>/<family>/generated/`
- **AND** generated directory SHALL contain `registers/` and `bitfields/` subdirectories
- **AND** `.generated` marker file SHALL exist to indicate auto-generated content

```
src/hal/vendors/st/stm32f4/
├── generated/              # Auto-generated code
│   ├── .generated          # Marker file
│   ├── registers/
│   │   ├── rcc_registers.hpp
│   │   └── gpio_registers.hpp
│   └── bitfields/
│       ├── rcc_bitfields.hpp
│       └── gpio_bitfields.hpp
├── clock_platform.hpp      # Hand-written
└── gpio.hpp                # Hand-written
```

#### Scenario: Hand-written platform code in family root

- **GIVEN** platform-specific implementation is needed
- **WHEN** developer creates new platform code
- **THEN** code SHALL be placed in `vendors/<vendor>/<family>/`
- **AND** code SHALL NOT be placed in `/generated/` subdirectory
- **AND** code SHALL use generated code from `/generated/` via includes

```cpp
// src/hal/vendors/st/stm32f4/clock_platform.hpp
#pragma once

#include "hal/vendors/st/stm32f4/generated/registers/rcc_registers.hpp"
#include "hal/core/types.hpp"

namespace stm32f4 {

template <u32 CLOCK_HZ>
struct Clock {
    static constexpr u32 system_clock_hz = CLOCK_HZ;

    static void init() {
        // Implementation using generated RCC registers
        using RCC = rcc_registers::RCC;
        RCC::CR::write(0x00000001);  // Enable HSI
    }
};

} // namespace stm32f4
```

#### Scenario: Shared code in /common/ directory

- **GIVEN** multiple MCU families share common code (e.g., all STM32 use Cortex-M)
- **WHEN** shared code is created
- **THEN** code SHALL be placed in `vendors/<vendor>/common/`
- **AND** family-specific code SHALL include from `../common/`

```
src/hal/vendors/st/
├── common/
│   ├── cortex_m_common.hpp     # Shared: NVIC, SysTick base
│   ├── stm32_clock_common.hpp  # Shared: PLL calculations
│   └── stm32_gpio_common.hpp   # Shared: GPIO mode enums
├── stm32f4/
│   ├── clock_platform.hpp      # Includes ../common/stm32_clock_common.hpp
│   └── gpio.hpp                # Includes ../common/stm32_gpio_common.hpp
└── stm32f7/
    ├── clock_platform.hpp
    └── gpio.hpp
```

#### Scenario: Platform directory removal

- **GIVEN** all code has been migrated from `src/hal/platform/` to `src/hal/vendors/`
- **WHEN** migration is complete and validated
- **THEN** `src/hal/platform/` directory SHALL be deleted
- **AND** no references to `hal/platform/` SHALL exist in includes
- **AND** CMake SHALL NOT reference `platform/` directory

---

### Requirement: Clear Directory Naming Convention

Directory names SHALL follow consistent convention: `vendors/<vendor_name>/<mcu_family>/<optional_specific_mcu>/`.

**Rationale**: Provides predictable structure for navigation and code generation.

#### Scenario: Vendor directory naming

- **GIVEN** new vendor is added to HAL
- **WHEN** vendor directory is created
- **THEN** directory name SHALL match vendor name in lowercase
- **AND** name SHALL be unambiguous (e.g., `st`, `arm`, `espressif`, `raspberrypi`)

```
src/hal/vendors/
├── st/           # STMicroelectronics
├── arm/          # ARM (for SAME70, etc.)
├── espressif/    # Espressif (ESP32)
└── raspberrypi/  # Raspberry Pi (RP2040)
```

#### Scenario: MCU family directory naming

- **GIVEN** new MCU family is added
- **WHEN** family directory is created
- **THEN** directory name SHALL match official family name
- **AND** name SHALL include series identifier (e.g., `stm32f4`, `stm32g0`, `same70`)

```
src/hal/vendors/st/
├── stm32f0/
├── stm32f1/
├── stm32f4/
├── stm32f7/
├── stm32g0/
└── stm32h7/
```

#### Scenario: Specific MCU variant directories

- **GIVEN** MCU family has significant variants (e.g., STM32F401 vs STM32F407)
- **WHEN** variant-specific code is needed
- **THEN** variant directory SHALL be created under family
- **AND** variant SHALL contain startup code, linker scripts, peripheral definitions

```
src/hal/vendors/st/stm32f4/
├── generated/
├── stm32f401/
│   ├── peripherals.hpp
│   └── startup.cpp
├── stm32f407/
│   ├── peripherals.hpp
│   └── startup.cpp
├── clock_platform.hpp  # Shared by all STM32F4
└── gpio.hpp            # Shared by all STM32F4
```

---

## REMOVED Requirements

### Requirement: Separate platform/ directory (REMOVED)

**Rationale**: Dual directory structure is confusing and unnecessary. Consolidating into single `vendors/` hierarchy.

**Migration**: All files from `src/hal/platform/` have been moved to `src/hal/vendors/` following the new structure.

---

## ADDED Requirements

### Requirement: Generated Code Marker Files

All auto-generated directories SHALL contain a `.generated` marker file to indicate content should not be manually edited.

**Rationale**: Prevents developers from accidentally modifying auto-generated code that will be overwritten.

#### Scenario: Marker file creation

- **GIVEN** code generation runs for a platform
- **WHEN** output is written to `/generated/` directory
- **THEN** `.generated` file SHALL be created in the directory
- **AND** file SHALL contain warning message and timestamp

```
# src/hal/vendors/st/stm32f4/generated/.generated
# AUTO-GENERATED CODE - DO NOT EDIT
# Generated: 2025-11-14 23:45:00
# Generator: codegen.py v1.0.0
# Source: data/stm32f4.svd
#
# This directory contains auto-generated code.
# Any manual changes will be overwritten on next generation.
# To customize behavior, edit templates or generator configuration.
```

#### Scenario: Build validation of generated code

- **GIVEN** build system processes HAL files
- **WHEN** CMake configuration runs
- **THEN** build SHALL verify `.generated` marker files exist
- **AND** build SHALL warn if hand-written files exist in `/generated/` directories
- **AND** build SHALL fail if `/generated/` directory is missing marker

```cmake
# CMake validation
if(EXISTS "${HAL_MCU_DIR}/generated")
    if(NOT EXISTS "${HAL_MCU_DIR}/generated/.generated")
        message(FATAL_ERROR
            "Missing .generated marker in ${HAL_MCU_DIR}/generated/\n"
            "This directory should only contain auto-generated code.")
    endif()
endif()
```

---

### Requirement: Include Path Consistency

All include paths SHALL use vendor hierarchy: `#include "hal/vendors/<vendor>/<family>/..."`.

**Rationale**: Explicit, predictable include paths that match directory structure.

#### Scenario: Including platform headers

- **GIVEN** board code needs platform-specific types
- **WHEN** header is included
- **THEN** include path SHALL start with `hal/vendors/`
- **AND** path SHALL include vendor and family
- **AND** path SHALL NOT use `hal/platform/`

```cpp
// CORRECT
#include "hal/vendors/st/stm32f4/clock_platform.hpp"
#include "hal/vendors/st/stm32f4/gpio.hpp"
#include "hal/vendors/st/stm32f4/generated/registers/rcc_registers.hpp"

// INCORRECT - old platform/ path
#include "hal/platform/st/stm32f4/clock_platform.hpp"
```

#### Scenario: Including generated code

- **GIVEN** hand-written platform code needs generated registers
- **WHEN** generated header is included
- **THEN** include path SHALL include `/generated/` in path
- **AND** path SHALL be relative to source root

```cpp
// src/hal/vendors/st/stm32f4/clock_platform.hpp

#include "hal/vendors/st/stm32f4/generated/registers/rcc_registers.hpp"
#include "hal/vendors/st/stm32f4/generated/bitfields/rcc_bitfields.hpp"
```

#### Scenario: Including common code

- **GIVEN** family-specific code needs shared vendor code
- **WHEN** common header is included
- **THEN** path SHALL reference `../common/` relative directory
- **OR** use full path `hal/vendors/<vendor>/common/`

```cpp
// src/hal/vendors/st/stm32f4/clock_platform.hpp

// Option 1: Relative include
#include "../common/stm32_clock_common.hpp"

// Option 2: Full path (preferred for clarity)
#include "hal/vendors/st/common/stm32_clock_common.hpp"
```
