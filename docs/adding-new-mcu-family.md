# Adding a New MCU Family to CoreZero

**Complete Guide with Real-World Example (STM32G0)**

This document provides a step-by-step guide for adding support for a new MCU family to the CoreZero framework. It uses the STM32G0 family as a real-world example throughout.

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Step-by-Step Process](#step-by-step-process)
4. [Architecture Layers](#architecture-layers)
5. [Code Generation](#code-generation)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

---

## Overview

Adding a new MCU family involves several layers of the CoreZero architecture:

```
┌─────────────────────────────────────────────────────────┐
│  Application Layer (examples/blink)                      │
│  - Uses board abstraction                                │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│  Board Layer (boards/nucleo_g0b1re/)                     │
│  - board.hpp, board.cpp, board_config.hpp               │
│  - Uses HAL APIs                                         │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│  HAL API Layer (src/hal/api/)                            │
│  - Platform-independent interfaces                       │
│  - gpio_simple.hpp, uart_simple.hpp, etc.               │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│  HAL Platform Layer (src/hal/platform/st/stm32g0/)       │
│  - Platform-specific implementations                     │
│  - gpio.hpp, systick_platform.hpp                       │
└───────────────────┬─────────────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────────────┐
│  Vendor Layer (src/hal/vendors/st/stm32g0/)              │
│  - Auto-generated registers and bitfields (from SVD)     │
│  - Auto-generated hardware policies (from JSON metadata) │
│  - registers/*.hpp, bitfields/*.hpp                     │
│  - *_hardware_policy.hpp                                │
└─────────────────────────────────────────────────────────┘
```

---

## Prerequisites

Before starting, ensure you have:

1. **SVD File**: Obtain the CMSIS-SVD file for your MCU
   - Usually available from vendor's website or ARM CMSIS-SVD repository
   - Example: `STM32G0B1.svd` from ST Microelectronics

2. **Reference Manual**: MCU family reference manual from vendor
   - Contains register descriptions and peripheral details
   - Example: RM0444 (STM32G0 Reference Manual)

3. **Datasheet**: MCU-specific datasheet
   - Pin assignments, electrical characteristics
   - Example: STM32G0B1RET6 datasheet

4. **Development Board** (optional but recommended)
   - For testing the implementation
   - Example: Nucleo-G0B1RE

---

## Step-by-Step Process

### Phase 1: SVD Processing and Code Generation

This phase generates all register definitions and bitfields automatically from the SVD file.

#### Step 1.1: Add SVD File

```bash
# Place SVD file in the appropriate location
cp STM32G0B1.svd tools/codegen/svd/upstream/cmsis-svd-data/data/STMicro/
```

#### Step 1.2: Generate Registers and Bitfields

```bash
cd tools/codegen

# Generate all registers and bitfields from SVD
python3 generate_stm32g0_registers.py
```

**What this generates:**
- `src/hal/vendors/st/stm32g0/registers/` - 33 register definition files
  - `rcc_registers.hpp`, `gpioa_registers.hpp`, `usart1_registers.hpp`, etc.
- `src/hal/vendors/st/stm32g0/bitfields/` - 33 bitfield definition files
  - `rcc_bitfields.hpp`, `gpioa_bitfields.hpp`, `usart1_bitfields.hpp`, etc.

**Example generated register file** (`rcc_registers.hpp`):
```cpp
namespace alloy::hal::st::stm32g0::rcc {

/// RCC Register Structure
struct RCC_Registers {
    volatile uint32_t CR;          ///< Clock control register
    volatile uint32_t ICSCR;       ///< Internal clock sources calibration
    volatile uint32_t CFGR;        ///< Clock configuration register
    volatile uint32_t PLLCFGR;     ///< PLL configuration register
    // ... more registers
};

/// RCC peripheral instance
inline RCC_Registers* RCC() {
    return reinterpret_cast<RCC_Registers*>(0x40021000);
}

}  // namespace alloy::hal::st::stm32g0::rcc
```

**Example generated bitfield file** (`rcc_bitfields.hpp`):
```cpp
namespace alloy::hal::st::stm32g0::rcc {

/// CR - Clock control register
namespace cr {
    /// HSI16 clock enable
    using HSION = BitField<8, 1>;
    constexpr uint32_t HSION_Pos = 8;
    constexpr uint32_t HSION_Msk = HSION::mask;

    /// HSI16 clock ready flag
    using HSIRDY = BitField<10, 1>;
    constexpr uint32_t HSIRDY_Pos = 10;
    constexpr uint32_t HSIRDY_Msk = HSIRDY::mask;
    // ... more bitfields
}

}  // namespace alloy::hal::st::stm32g0::rcc
```

#### Step 1.3: Generate Startup Code

The startup code is also auto-generated from SVD:

**What gets generated:**
- Vector table with correct Cortex-M exception handlers
- Peripheral interrupt handlers
- Reset handler with .data and .bss initialization

**Example** (`src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp`):
```cpp
// Vector table
__attribute__((section(".isr_vector"), used))
void (* const vector_table[])() = {
    // Cortex-M0+ System Exceptions (16 entries)
    reinterpret_cast<void (*)()>(&_estack),  // Stack pointer
    Reset_Handler,                           // Reset
    NMI_Handler,                            // NMI
    HardFault_Handler,                      // HardFault
    // ... reserved slots ...
    SysTick_Handler,                        // SysTick

    // STM32G0B1 Peripheral Interrupts
    WWDG_Handler,       // IRQ 0
    PVD_Handler,        // IRQ 1
    // ... all peripheral IRQs ...
};
```

---

### Phase 2: Hardware Policy Creation

Hardware policies provide platform-specific hardware access using the Policy-Based Design pattern.

#### Step 2.1: Create Peripheral Metadata JSON

Create metadata files describing each peripheral's policy methods.

**Location**: `tools/codegen/cli/generators/metadata/platform/`

**Example**: `stm32g0_gpio.json`

```json
{
  "family": "stm32g0",
  "mcu": "stm32g0b1",
  "vendor": "st",
  "peripheral_name": "GPIO",
  "description": "General Purpose Input/Output",
  "register_type": "GPIOA_Registers",
  "register_namespace": "gpioa",
  "register_include": "hal/vendors/st/stm32g0/registers/gpioa_registers.hpp",
  "bitfield_include": "hal/vendors/st/stm32g0/bitfields/gpioa_bitfields.hpp",
  "platform_namespace": "alloy::hal::st::stm32g0",
  "vendor_namespace": "alloy::hal::st::stm32g0",

  "features": {
    "pull_up": true,
    "pull_down": true,
    "open_drain": true,
    "alternate_function": true,
    "output_speed": true
  },

  "port_bases": {
    "GPIOA": "0x50000000",
    "GPIOB": "0x50000400",
    "GPIOC": "0x50000800"
  },

  "template_params": [
    {
      "name": "BASE_ADDR",
      "type": "uint32_t",
      "description": "Peripheral base address"
    },
    {
      "name": "PERIPH_CLOCK_HZ",
      "type": "uint32_t",
      "description": "Peripheral clock frequency in Hz"
    }
  ],

  "policy_methods": {
    "description": "Hardware Policy methods for GPIO",
    "peripheral_clock_hz": 64000000,
    "mock_hook_prefix": "ALLOY_GPIO_MOCK_HW",

    "hw_accessor": {
      "description": "Get pointer to hardware registers",
      "return_type": "volatile RegisterType*",
      "code": "return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);"
    },

    "set_mode_output": {
      "description": "Configure pin as output",
      "parameters": [
        {"name": "pin_number", "type": "uint8_t", "description": "Pin number (0-15)"}
      ],
      "return_type": "void",
      "code": "hw()->MODER = (hw()->MODER & ~(0x3U << (pin_number * 2))) | (0x1U << (pin_number * 2));",
      "test_hook": "ALLOY_GPIO_TEST_HOOK_MODER"
    },

    "set_output": {
      "description": "Set output pin high",
      "parameters": [
        {"name": "pin_mask", "type": "uint32_t", "description": "Pin mask"}
      ],
      "return_type": "void",
      "code": "hw()->BSRR = pin_mask;",
      "test_hook": "ALLOY_GPIO_TEST_HOOK_BSRR"
    },

    "clear_output": {
      "description": "Set output pin low",
      "parameters": [
        {"name": "pin_mask", "type": "uint32_t", "description": "Pin mask"}
      ],
      "return_type": "void",
      "code": "hw()->BRR = pin_mask;",
      "test_hook": "ALLOY_GPIO_TEST_HOOK_BRR"
    }

    // ... more policy methods ...
  }
}
```

**Key Points for Creating Metadata:**

1. **Template Parameters** (`template_params`): **REQUIRED**
   - Must include `BASE_ADDR` and `PERIPH_CLOCK_HZ` at minimum
   - These become template parameters in the generated hardware policy
   - Allows compile-time configuration of peripheral instances
   - Example: `template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>`

2. **Namespace Configuration**:
   - `register_namespace`: The nested namespace where registers are defined (e.g., "gpioa", "i2c1")
   - `vendor_namespace`: The parent vendor namespace (e.g., "alloy::hal::st::stm32g0")
   - Generator combines these: `using namespace alloy::hal::st::stm32g0::gpioa;`

3. **Register Access Pattern**: Understand your MCU's register structure
   - Write-only registers (BSRR, BRR for GPIO set/reset)
   - Read-modify-write registers (MODER for mode configuration)
   - Read-only registers (IDR for input reading)

4. **Bit Field Layout**: Check reference manual for:
   - 1 bit per pin vs 2 bits per pin vs 4 bits per pin
   - Field positions and masks
   - Reset values

5. **Hardware-Specific Features**: Include what's available:
   - STM32: Alternate functions, output speed, pull resistors
   - SAME70/PIO: Debounce, input filter, multi-driver

#### Step 2.2: Generate Hardware Policies

```bash
cd tools/codegen

# Generate policy for specific peripheral
python3 generate_hardware_policy.py --family stm32g0 --peripheral gpio

# Or generate all policies for the family
python3 generate_hardware_policy.py --family stm32g0 --all
```

**What this generates:**
- `src/hal/vendors/st/stm32g0/gpio_hardware_policy.hpp`
- `src/hal/vendors/st/stm32g0/uart_hardware_policy.hpp`
- etc.

**Example generated policy** (`gpio_hardware_policy.hpp`):
```cpp
template <uint32_t BASE_ADDR, uint32_t PERIPH_CLOCK_HZ>
struct Stm32g0GpioHardwarePolicy {
    using RegisterType = GPIOA_Registers;

    static constexpr uint32_t base_address = BASE_ADDR;
    static constexpr uint32_t peripheral_clock_hz = PERIPH_CLOCK_HZ;

    static inline volatile RegisterType* hw() {
        #ifdef ALLOY_GPIO_MOCK_HW
            return ALLOY_GPIO_MOCK_HW();  // Test hook
        #else
            return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);
        #endif
    }

    static inline void set_mode_output(uint8_t pin_number) {
        hw()->MODER = (hw()->MODER & ~(0x3U << (pin_number * 2))) |
                      (0x1U << (pin_number * 2));
    }

    static inline void set_output(uint32_t pin_mask) {
        hw()->BSRR = pin_mask;
    }

    // ... all other policy methods ...
};
```

---

### Phase 3: Platform Layer Implementation

The platform layer provides MCU-specific implementations using the generated policies.

#### Step 3.1: Create Platform Directory

```bash
mkdir -p src/hal/platform/st/stm32g0
```

#### Step 3.2: Implement SysTick Platform Support

**File**: `src/hal/platform/st/stm32g0/systick_platform.hpp`

```cpp
#pragma once

#include <cstdint>

namespace alloy::hal::st::stm32g0 {

template <uint32_t CLOCK_HZ>
struct SysTick {
    static constexpr uint32_t clock_hz = CLOCK_HZ;

    // Implementation using Cortex-M SysTick registers
    // ...
};

}  // namespace alloy::hal::st::stm32g0
```

#### Step 3.3: Implement GPIO Platform Support (Optional)

You can either:
1. **Use generated hardware policies directly** (recommended for new families)
2. **Create wrapper implementations** (if you need additional abstraction)

For STM32G0, we keep it simple and use the auto-generated policy.

---

### Phase 4: Board Support Package

Create board-specific configuration and initialization.

#### Step 4.1: Create Board Directory

```bash
mkdir -p boards/nucleo_g0b1re
```

#### Step 4.2: Create Board Configuration

**File**: `boards/nucleo_g0b1re/board_config.hpp`

```cpp
#pragma once

#include <cstdint>

namespace nucleo_g0b1re {

/// Clock configuration
struct ClockConfig {
    static constexpr uint32_t hsi_hz = 16000000;        // HSI16 oscillator
    static constexpr uint32_t system_clock_hz = 64000000; // PLL output
    static constexpr uint32_t ahb_hz = 64000000;
    static constexpr uint32_t apb_hz = 64000000;
};

/// LED configuration
struct LedConfig {
    // Green LED on PA5
    static constexpr uint32_t led_green_port = 0x50000000;  // GPIOA
    static constexpr uint8_t led_green_pin = 5;
    static constexpr bool led_green_active_high = true;

    using led_green = GpioPin<led_green_port, led_green_pin>;
};

}  // namespace nucleo_g0b1re
```

#### Step 4.3: Implement Board Initialization

**File**: `boards/nucleo_g0b1re/board.cpp`

```cpp
#include "board.hpp"
#include "hal/api/systick_simple.hpp"
#include "hal/vendors/st/stm32g0/registers/rcc_registers.hpp"
#include "hal/vendors/st/stm32g0/registers/flash_registers.hpp"
#include "hal/vendors/st/stm32g0/bitfields/rcc_bitfields.hpp"
#include "hal/vendors/st/stm32g0/bitfields/flash_bitfields.hpp"

using namespace alloy::hal::st::stm32g0;

namespace board {

static inline void configure_system_clock() {
    using namespace rcc;  // Use RCC bitfields namespace

    // 1. Enable HSI16
    rcc::RCC()->CR |= cr::HSION::mask;
    while (!(rcc::RCC()->CR & cr::HSIRDY::mask));

    // 2. Configure PLL: HSI16 / 1 × 8 / 2 = 64 MHz
    rcc::RCC()->PLLCFGR = pllcfgr::PLLSRC::write(0, 2) |  // HSI16
                          pllcfgr::PLLM::write(0, 0) |    // /1
                          pllcfgr::PLLN::write(0, 8) |    // ×8
                          pllcfgr::PLLR::write(0, 0) |    // /2
                          pllcfgr::PLLREN::mask;          // Enable

    // 3. Enable PLL
    rcc::RCC()->CR |= cr::PLLON::mask;
    while (!(rcc::RCC()->CR & cr::PLLRDY::mask));

    // 4. Configure flash latency (2 wait states for 64 MHz)
    flash::FLASH()->ACR = flash::acr::LATENCY::write(flash::FLASH()->ACR, 2);

    // 5. Switch to PLL
    rcc::RCC()->CFGR = cfgr::SW::write(rcc::RCC()->CFGR, 2);
    while (cfgr::SWS::read(rcc::RCC()->CFGR) != 2);
}

static inline void enable_gpio_clocks() {
    using namespace rcc;

    rcc::RCC()->IOPENR |= iopenr::GPIOAEN::mask |
                          iopenr::GPIOBEN::mask |
                          iopenr::GPIOCEN::mask;
}

void init() {
    configure_system_clock();
    enable_gpio_clocks();
    SysTickTimer::init_ms<BoardSysTick>(1);
    led::init();
    __asm volatile ("cpsie i" ::: "memory");  // Enable interrupts
}

}  // namespace board
```

**Key Points:**
1. ✅ **Uses auto-generated registers** - No magic numbers!
2. ✅ **Uses auto-generated bitfields** - Type-safe bit manipulation
3. ✅ **Self-documenting** - Clear what each operation does
4. ✅ **Zero overhead** - Compiles to same assembly as manual code

---

### Phase 5: CMake Integration

#### Step 5.1: Update Root CMakeLists.txt

```cmake
# Auto-detect platform from board
if(ALLOY_BOARD STREQUAL "nucleo_g0b1re" OR ALLOY_BOARD MATCHES "^stm32g0")
    set(ALLOY_PLATFORM "stm32g0")
endif()

# Board abstraction layer configuration
if(ALLOY_BOARD STREQUAL "nucleo_g0b1re")
    set(BOARD_HEADER_PATH "boards/nucleo_g0b1re/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/nucleo_g0b1re/STM32G0B1RET6.ld")
endif()

# Define board macro
string(TOUPPER "${ALLOY_BOARD}" ALLOY_BOARD_UPPER)
add_compile_definitions(ALLOY_BOARD_${ALLOY_BOARD_UPPER})

# Add example if board is supported
if(ALLOY_BOARD STREQUAL "nucleo_g0b1re")
    add_subdirectory(examples/blink)
endif()
```

#### Step 5.2: Create Example CMakeLists.txt

**File**: `examples/blink/CMakeLists.txt`

```cmake
add_executable(blink
    main.cpp
    ${CMAKE_SOURCE_DIR}/${BOARD_HEADER_PATH}
    ${CMAKE_SOURCE_DIR}/boards/${ALLOY_BOARD}/board.cpp
)

target_link_libraries(blink PRIVATE alloy-hal)

# Add startup code
target_sources(blink PRIVATE ${STARTUP_SOURCE})

# Set linker script
set_target_properties(blink PROPERTIES
    LINK_DEPENDS ${LINKER_SCRIPT}
)
target_link_options(blink PRIVATE -T${LINKER_SCRIPT})

# Generate hex and bin files
add_custom_command(TARGET blink POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:blink> ${CMAKE_CURRENT_BINARY_DIR}/blink.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:blink> ${CMAKE_CURRENT_BINARY_DIR}/blink.bin
)
```

---

### Phase 6: Makefile Integration

#### Step 6.1: Add Build Targets

```makefile
# STM32G0 Nucleo-G0B1RE Board
NUCLEO_G0B1RE_BUILD_DIR := build-nucleo-g0b1re
NUCLEO_G0B1RE_BLINK := blink

nucleo-g0b1re-blink-build:
	@echo "Building blink for Nucleo-G0B1RE..."
	@cmake -S . -B $(NUCLEO_G0B1RE_BUILD_DIR) \
		-DALLOY_BOARD=nucleo_g0b1re \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake
	@cmake --build $(NUCLEO_G0B1RE_BUILD_DIR) --target $(NUCLEO_G0B1RE_BLINK)

nucleo-g0b1re-blink-flash: nucleo-g0b1re-blink-build
	@openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
		-c "program $(NUCLEO_G0B1RE_BUILD_DIR)/examples/blink/blink.elf verify reset exit"
```

---

## Architecture Layers

### Layer 1: Vendor Layer (Auto-Generated)

**Purpose**: Provide low-level hardware access
**Location**: `src/hal/vendors/<vendor>/<family>/`
**Generated from**: SVD files + JSON metadata

**Contents**:
- `registers/*.hpp` - Register structures
- `bitfields/*.hpp` - Bit field definitions
- `*_hardware_policy.hpp` - Hardware policies

**Characteristics**:
- ✅ 100% auto-generated
- ✅ Zero manual coding
- ✅ Type-safe
- ✅ Zero runtime overhead

### Layer 2: Platform Layer (Manual)

**Purpose**: Platform-specific implementations
**Location**: `src/hal/platform/<vendor>/<family>/`

**Contents**:
- `gpio.hpp` - GPIO implementation
- `systick_platform.hpp` - SysTick timer
- etc.

**Characteristics**:
- ⚙️ Uses generated hardware policies
- ⚙️ Platform-specific optimizations
- ⚙️ Template-based (compile-time)

### Layer 3: HAL API Layer (Generic)

**Purpose**: Platform-independent interfaces
**Location**: `src/hal/api/`

**Contents**:
- `gpio_simple.hpp`
- `uart_simple.hpp`
- `systick_simple.hpp`

**Characteristics**:
- 🔧 Generic, reusable APIs
- 🔧 Works across all platforms
- 🔧 Policy-based design

### Layer 4: Board Layer (Manual)

**Purpose**: Board-specific configuration
**Location**: `boards/<board_name>/`

**Contents**:
- `board.hpp` - Board API
- `board.cpp` - Initialization
- `board_config.hpp` - Configuration

**Characteristics**:
- 📋 Board-specific pin mappings
- 📋 Clock configuration
- 📋 Peripheral initialization

---

## Code Generation

### Auto-Generated Files (DO NOT EDIT)

All files with this header are auto-generated:

```cpp
/// Auto-generated from CMSIS-SVD
/// DO NOT EDIT - Regenerate if SVD file changes
```

**Regeneration commands**:

```bash
# Regenerate registers and bitfields
python3 tools/codegen/generate_stm32g0_registers.py

# Regenerate hardware policies
python3 tools/codegen/generate_hardware_policy.py --family stm32g0 --all
```

### Metadata Files (Manual Creation)

Metadata files describe peripheral policies and must be created manually:

**Location**: `tools/codegen/cli/generators/metadata/platform/`

**Naming**: `<family>_<peripheral>.json`
- Examples: `stm32g0_gpio.json`, `stm32g0_uart.json`

**Template Structure**:

```json
{
  "family": "<family>",
  "mcu": "<representative_mcu>",
  "vendor": "<vendor>",
  "peripheral_name": "<PERIPHERAL>",
  "register_type": "<PERIPHERAL>_Registers",
  "register_namespace": "<peripheral_lowercase>",
  "register_include": "hal/vendors/<vendor>/<family>/registers/<peripheral>_registers.hpp",
  "bitfield_include": "hal/vendors/<vendor>/<family>/bitfields/<peripheral>_bitfields.hpp",

  "policy_methods": {
    "hw_accessor": { ... },
    "method_name": {
      "description": "...",
      "parameters": [...],
      "return_type": "...",
      "code": "...",
      "test_hook": "..."
    }
  }
}
```

---

## Best Practices

### 1. Use Auto-Generated Code

✅ **DO**:
```cpp
rcc::RCC()->CR |= cr::HSION::mask;  // Uses generated bitfield
```

❌ **DON'T**:
```cpp
RCC->CR |= (1 << 8);  // Magic number
```

### 2. Keep Board Code Clean

✅ **DO**:
```cpp
static inline void configure_system_clock() {
    using namespace rcc;  // Clear namespace usage

    // Well-commented, self-documenting code
    rcc::RCC()->CR |= cr::HSION::mask;
    while (!(rcc::RCC()->CR & cr::HSIRDY::mask));
}
```

❌ **DON'T**:
```cpp
void configure_system_clock() {
    *(volatile uint32_t*)0x40021000 |= 0x100;  // What is this?
}
```

### 3. Leverage Type Safety

✅ **DO**:
```cpp
pllcfgr::PLLN::write(0, 8)  // Compile-time bounds checking
```

❌ **DON'T**:
```cpp
(8 << 8)  // No bounds checking
```

### 4. Document Hardware-Specific Choices

```cpp
// 2. Configure PLL: HSI16 / 1 × 8 / 2 = 64 MHz
// PLLSRC=2 (HSI16), PLLM=0 (/1), PLLN=8 (×8), PLLR=0 (/2)
rcc::RCC()->PLLCFGR = pllcfgr::PLLSRC::write(0, 2) |
                      pllcfgr::PLLM::write(0, 0) |
                      pllcfgr::PLLN::write(0, 8) |
                      pllcfgr::PLLR::write(0, 0) |
                      pllcfgr::PLLREN::mask;
```

### 5. Zero Overhead Verification

Always verify that generated code has zero overhead:

```bash
# Build and check size
make nucleo-g0b1re-blink-build

# Should show:
#    text       data        bss        dec        hex    filename
#    1028         16       8232       9276       243c   blink
```

Compare with manually written equivalent - should be identical!

---

## Troubleshooting

### Issue 1: Registers Not Found

**Symptom**: `error: 'RCC' was not declared`

**Cause**: Missing include or namespace

**Fix**:
```cpp
#include "hal/vendors/st/stm32g0/registers/rcc_registers.hpp"
using namespace alloy::hal::st::stm32g0::rcc;
```

### Issue 2: Bitfield Compilation Errors

**Symptom**: `error: no member named 'write'`

**Cause**: Incorrect BitField usage

**Fix**:
```cpp
// Correct:
pllcfgr::PLLN::write(0, 8)

// Incorrect:
pllcfgr::PLLN::write_value(0, 8)  // No such method
```

### Issue 3: Hardware Policy Generation Fails

**Symptom**: `AttributeError: 'dict object' has no attribute 'hw_accessor'`

**Cause**: Missing `hw_accessor` in metadata JSON

**Fix**: Ensure all metadata files include:
```json
"policy_methods": {
  "hw_accessor": {
    "description": "Get pointer to hardware registers",
    "return_type": "volatile RegisterType*",
    "code": "return reinterpret_cast<volatile RegisterType*>(BASE_ADDR);"
  }
}
```

### Issue 4: Linker Errors

**Symptom**: `undefined reference to vector_table`

**Cause**: Missing startup code

**Fix**: Add to CMakeLists.txt:
```cmake
target_sources(blink PRIVATE ${STARTUP_SOURCE})
```

---

## Checklist for New Family

Use this checklist when adding a new family:

- [ ] Obtain SVD file from vendor
- [ ] Place SVD in `tools/codegen/svd/upstream/cmsis-svd-data/data/<Vendor>/`
- [ ] Generate registers and bitfields
  - [ ] Run SVD generator
  - [ ] Verify output in `src/hal/vendors/<vendor>/<family>/`
  - [ ] Check register structures are correct
  - [ ] Check bitfields match reference manual
- [ ] Create peripheral metadata
  - [ ] Create `<family>_gpio.json`
  - [ ] Create `<family>_uart.json`
  - [ ] Create metadata for other needed peripherals
- [ ] Generate hardware policies
  - [ ] Run policy generator for all peripherals
  - [ ] Verify generated code compiles
- [ ] Create platform layer
  - [ ] Create directory `src/hal/platform/<vendor>/<family>/`
  - [ ] Implement `systick_platform.hpp`
  - [ ] Implement other platform-specific code as needed
- [ ] Create board support
  - [ ] Create directory `boards/<board_name>/`
  - [ ] Create `board_config.hpp`
  - [ ] Create `board.hpp`
  - [ ] Create `board.cpp`
  - [ ] Create linker script
- [ ] Update build system
  - [ ] Update root `CMakeLists.txt`
  - [ ] Update `Makefile`
  - [ ] Add flash targets
- [ ] Test
  - [ ] Build blink example
  - [ ] Flash to hardware
  - [ ] Verify LED blinks
  - [ ] Check binary size (should be minimal)

---

## Summary

Adding a new MCU family to CoreZero involves:

1. **Auto-generate** registers, bitfields, and startup code from SVD
2. **Create** JSON metadata for hardware policies
3. **Generate** hardware policies from metadata
4. **Implement** platform layer using generated code
5. **Create** board support package
6. **Integrate** with build system

**Key Principle**: Maximize auto-generation, minimize manual coding!

- 95% of vendor layer = auto-generated
- 80% of platform layer = uses auto-generated code
- 100% type-safe, zero runtime overhead

**Result**: Clean, maintainable, portable code that's easy to understand and extend!

---

*Last updated: 2025-11-14*
*Based on: STM32G0 family integration*
