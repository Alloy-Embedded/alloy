# Porting a New Platform (MCU Family) to Alloy

**Difficulty:** ⭐⭐⭐⭐☆ (Advanced)
**Time Required:** 4-8 hours
**Prerequisites:** Deep C++ knowledge, MCU architecture understanding, CMake experience

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Architecture Overview](#architecture-overview)
4. [Step-by-Step Guide](#step-by-step-guide)
5. [Example: Adding STM32G0](#example-adding-stm32g0)
6. [Testing Your Platform](#testing-your-platform)
7. [Concept Validation](#concept-validation)
8. [Troubleshooting](#troubleshooting)

---

## Overview

Adding a new platform (MCU family) to Alloy involves creating the entire hardware abstraction layer for that family. This is a multi-step process that includes:

1. **Code Generation** - Generate registers, bitfields, and startup code from SVD
2. **Hardware Policies** - Create low-level register access policies
3. **Platform Implementations** - Implement Clock, GPIO, UART, etc.
4. **Concept Validation** - Ensure all implementations satisfy C++20 concepts
5. **CMake Integration** - Add platform to build system
6. **Testing** - Validate with real hardware

**Estimated effort:**
- Code generation: 1-2 hours
- Hardware policies: 2-3 hours (if generating), 1 day (if hand-written)
- Platform implementations: 2-4 hours per peripheral
- Testing & debugging: 2-4 hours

---

## Prerequisites

### Required Knowledge

- **C++20**: Concepts, templates, constexpr
- **MCU Architecture**: Register layout, clock trees, peripheral buses
- **CMake**: Advanced features, toolchain files
- **SVD Files**: CMSIS-SVD format understanding

### Required Files

1. **CMSIS-SVD File** - XML description of MCU peripherals
   - Download from vendor or [cmsis-svd repository](https://github.com/posborne/cmsis-svd)
   - Example: `STM32G0B1.svd`

2. **MCU Datasheet** - Official vendor documentation
   - Memory map
   - Clock tree diagram
   - Peripheral descriptions
   - Electrical characteristics

3. **Reference Manual** - Detailed register descriptions
   - Register addresses
   - Bitfield definitions
   - Reset values

### Tools

- **Python 3.10+** - For code generator
- **CMake 3.25+** - Build system
- **ARM Toolchain** - arm-none-eabi-gcc 11+
- **SVD Tool** (optional) - For SVD validation

---

## Architecture Overview

### Layer Structure for New Platform

```
src/hal/vendors/{vendor}/{family}/
├── generated/              # Auto-generated from SVD
│   ├── .generated          # Marker file
│   ├── registers/
│   │   ├── gpio_registers.hpp
│   │   ├── rcc_registers.hpp
│   │   └── ...
│   └── bitfields/
│       ├── gpio_bitfields.hpp
│       ├── rcc_bitfields.hpp
│       └── ...
│
├── {mcu_model}/            # MCU-specific
│   ├── peripherals.hpp     # Peripheral base addresses
│   └── startup.cpp         # Startup code + vector table
│
├── gpio_hardware_policy.hpp    # GPIO register access
├── uart_hardware_policy.hpp    # UART register access
├── spi_hardware_policy.hpp     # SPI register access
├── i2c_hardware_policy.hpp     # I2C register access
│
├── gpio.hpp                # GPIO platform implementation
├── clock_platform.hpp      # Clock platform implementation
└── systick_platform.hpp    # SysTick platform implementation
```

### Required Implementations

To satisfy Alloy's concepts, you must implement:

| Component | Concept | Priority | Difficulty |
|-----------|---------|----------|------------|
| Clock | `ClockPlatform` | **Required** | Medium |
| GPIO | `GpioPin` | **Required** | Easy |
| SysTick | - | **Required** | Easy |
| UART | `UartPeripheral` | Recommended | Medium |
| SPI | `SpiPeripheral` | Recommended | Medium |
| I2C | - | Optional | Medium |
| ADC | `AdcPeripheral` | Optional | Medium |
| PWM | `PwmPeripheral` | Optional | Hard |
| DMA | `DmaCapable` | Optional | Hard |

---

## Step-by-Step Guide

### Step 1: Obtain CMSIS-SVD File

**Option A: Download from vendor**
```bash
# STMicroelectronics
wget https://www.st.com/resource/en/svd/stm32g0_svd.zip
unzip stm32g0_svd.zip

# Microchip (Atmel)
# Download from Microchip Packs Repository
```

**Option B: Extract from CMSIS Pack**
```bash
# Example: STM32G0 CMSIS Pack
wget http://www.keil.com/pack/Keil.STM32G0xx_DFP.1.4.0.pack
unzip Keil.STM32G0xx_DFP.1.4.0.pack
# SVD files in: CMSIS/SVD/
```

**Validate SVD file:**
```bash
python3 tools/codegen/svd_parser.py --validate STM32G0B1.svd
```

### Step 2: Generate Code from SVD

```bash
cd tools/codegen

# Parse SVD to JSON database
python3 svd_parser.py \
    --input ../../svd/ST/STM32G0B1.svd \
    --output database/families/stm32g0.json

# Generate all code
python3 codegen.py generate \
    --family stm32g0 \
    --vendor st \
    --output ../../src/hal/vendors/st/stm32g0/generated/
```

This generates:
- `registers/` - Register struct definitions
- `bitfields/` - Bitfield masks and accessors
- `.generated` - Marker file

### Step 3: Create Directory Structure

```bash
cd src/hal/vendors/

# Create vendor directory (if new vendor)
mkdir -p {vendor}/{family}

# Example: STM32G0
mkdir -p st/stm32g0
cd st/stm32g0

# Create MCU-specific directory
mkdir -p stm32g0b1

# Generated code already created in step 2
ls generated/
# registers/  bitfields/  .generated
```

### Step 4: Create Peripheral Addresses

**File:** `src/hal/vendors/st/stm32g0/stm32g0b1/peripherals.hpp`

```cpp
/**
 * @file peripherals.hpp
 * @brief Peripheral base addresses for STM32G0B1
 *
 * Source: STM32G0B1 Reference Manual (RM0444)
 *         Section 2.2: Memory map
 */

#pragma once

#include "core/types.hpp"

namespace alloy::hal::st::stm32g0::peripherals {

using namespace alloy::core;

// ============================================================================
// APB Peripherals
// ============================================================================

// APB1 Peripherals (0x4000_0000 - 0x4000_7FFF)
constexpr u32 TIM2   = 0x40000000;
constexpr u32 TIM3   = 0x40000400;
constexpr u32 TIM6   = 0x40001000;
constexpr u32 TIM7   = 0x40001400;
constexpr u32 TIM14  = 0x40002000;

constexpr u32 RTC    = 0x40002800;
constexpr u32 WWDG   = 0x40002C00;
constexpr u32 IWDG   = 0x40003000;

constexpr u32 SPI2   = 0x40003800;
constexpr u32 SPI3   = 0x40003C00;

constexpr u32 USART2 = 0x40004400;
constexpr u32 USART3 = 0x40004800;
constexpr u32 USART4 = 0x40004C00;

constexpr u32 I2C1   = 0x40005400;
constexpr u32 I2C2   = 0x40005800;

constexpr u32 PWR    = 0x40007000;
constexpr u32 DAC1   = 0x40007400;
constexpr u32 LPTIM1 = 0x40007C00;

// APB2 Peripherals (0x4001_0000 - 0x4001_5FFF)
constexpr u32 SYSCFG = 0x40010000;
constexpr u32 TIM1   = 0x40012C00;
constexpr u32 SPI1   = 0x40013000;
constexpr u32 USART1 = 0x40013800;
constexpr u32 TIM15  = 0x40014000;
constexpr u32 TIM16  = 0x40014400;
constexpr u32 TIM17  = 0x40014800;
constexpr u32 ADC1   = 0x40012400;

// ============================================================================
// AHB Peripherals
// ============================================================================

// AHB Peripherals (0x4002_0000 - 0x4802_4FFF)
constexpr u32 DMA1   = 0x40020000;
constexpr u32 DMA2   = 0x40020400;
constexpr u32 DMAMUX = 0x40020800;

constexpr u32 RCC    = 0x40021000;
constexpr u32 FLASH  = 0x40022000;
constexpr u32 CRC    = 0x40023000;

// GPIO Ports
constexpr u32 GPIOA  = 0x50000000;
constexpr u32 GPIOB  = 0x50000400;
constexpr u32 GPIOC  = 0x50000800;
constexpr u32 GPIOD  = 0x50000C00;
constexpr u32 GPIOE  = 0x50001000;
constexpr u32 GPIOF  = 0x50001400;

} // namespace alloy::hal::st::stm32g0::peripherals
```

**Source for addresses:** MCU reference manual, section "Memory map"

### Step 5: Create Startup Code

**File:** `src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp`

This can be auto-generated or hand-written. Example:

```cpp
/**
 * @file startup.cpp
 * @brief Startup code for STM32G0B1
 *
 * Handles:
 * - Vector table
 * - Reset handler
 * - .data/.bss initialization
 * - C++ static constructors
 */

#include <cstdint>

// Linker script symbols
extern uint32_t _estack;      // Initial stack pointer
extern uint32_t _sdata;       // Start of .data in RAM
extern uint32_t _edata;       // End of .data in RAM
extern uint32_t _sidata;      // Start of .data in FLASH
extern uint32_t _sbss;        // Start of .bss
extern uint32_t _ebss;        // End of .bss
extern uint32_t __preinit_array_start;
extern uint32_t __preinit_array_end;
extern uint32_t __init_array_start;
extern uint32_t __init_array_end;

// Application entry point
extern "C" int main();

// ============================================================================
// Default Handlers
// ============================================================================

extern "C" {

// Weak default handler
[[gnu::weak]] void Default_Handler() {
    while (1) {
        __asm__ volatile("nop");
    }
}

// Weak aliases for all interrupt handlers
void NMI_Handler() __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler() __attribute__((weak, alias("Default_Handler")));
void SVC_Handler() __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler() __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler() __attribute__((weak, alias("Default_Handler")));

// STM32G0 specific interrupts (from reference manual)
void WWDG_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RTC_TAMP_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void EXTI0_1_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void EXTI2_3_IRQHandler() __attribute__((weak, alias("Default_Handler")));
void EXTI4_15_IRQHandler() __attribute__((weak, alias("Default_Handler")));
// ... more interrupts (see reference manual for complete list)

} // extern "C"

// ============================================================================
// Reset Handler
// ============================================================================

extern "C" [[noreturn]] void Reset_Handler() {
    // 1. Copy .data section from FLASH to RAM
    uint32_t* src = &_sidata;
    uint32_t* dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    // 2. Zero-initialize .bss section
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    // 3. Call preinit array (rarely used)
    uint32_t* preinit = &__preinit_array_start;
    while (preinit < &__preinit_array_end) {
        ((void(*)())*preinit++)();
    }

    // 4. Call C++ static constructors
    uint32_t* init = &__init_array_start;
    while (init < &__init_array_end) {
        ((void(*)())*init++)();
    }

    // 5. Call main
    main();

    // 6. If main returns, hang
    while (1) {
        __asm__ volatile("nop");
    }
}

// ============================================================================
// Vector Table
// ============================================================================

typedef void (*vector_table_entry_t)();

extern "C" {
    // Vector table placed in .isr_vector section (goes to start of FLASH)
    __attribute__((section(".isr_vector")))
    const vector_table_entry_t vectors[] = {
        (vector_table_entry_t)&_estack,  // Initial stack pointer
        Reset_Handler,                    // Reset handler

        // Core exceptions
        NMI_Handler,
        HardFault_Handler,
        0, 0, 0, 0, 0, 0, 0,             // Reserved
        SVC_Handler,
        0, 0,                             // Reserved
        PendSV_Handler,
        SysTick_Handler,

        // STM32G0 interrupts (positions from reference manual)
        WWDG_IRQHandler,           // IRQ 0
        PVD_IRQHandler,            // IRQ 1
        RTC_TAMP_IRQHandler,       // IRQ 2
        FLASH_IRQHandler,          // IRQ 3
        RCC_IRQHandler,            // IRQ 4
        EXTI0_1_IRQHandler,        // IRQ 5
        EXTI2_3_IRQHandler,        // IRQ 6
        EXTI4_15_IRQHandler,       // IRQ 7
        // ... continue for all interrupts
    };
}
```

**Key points:**
- Vector table positions from reference manual "Interrupt vector table"
- Reset handler initializes .data and .bss
- C++ constructor support via __init_array
- All handlers have weak aliases (can be overridden)

### Step 6: Create Hardware Policies

Hardware policies provide low-level register access. Example for GPIO:

**File:** `src/hal/vendors/st/stm32g0/gpio_hardware_policy.hpp`

```cpp
#pragma once

#include "core/types.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/gpioa_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/gpioa_bitfields.hpp"

namespace alloy::hal::st::stm32g0 {

template <uint32_t GPIO_BASE, uint32_t PERIPHERAL_CLOCK_HZ>
struct Stm32g0GPIOHardwarePolicy {

    // Register offsets (from reference manual)
    static constexpr uint32_t MODER_OFFSET   = 0x00;
    static constexpr uint32_t OTYPER_OFFSET  = 0x04;
    static constexpr uint32_t OSPEEDR_OFFSET = 0x08;
    static constexpr uint32_t PUPDR_OFFSET   = 0x0C;
    static constexpr uint32_t IDR_OFFSET     = 0x10;
    static constexpr uint32_t ODR_OFFSET     = 0x14;
    static constexpr uint32_t BSRR_OFFSET    = 0x18;
    static constexpr uint32_t LCKR_OFFSET    = 0x1C;
    static constexpr uint32_t AFRL_OFFSET    = 0x20;
    static constexpr uint32_t AFRH_OFFSET    = 0x24;
    static constexpr uint32_t BRR_OFFSET     = 0x28;

    // Set pin output HIGH
    static void set_output(uint32_t pin_mask) {
        volatile uint32_t* bsrr = reinterpret_cast<volatile uint32_t*>(GPIO_BASE + BSRR_OFFSET);
        *bsrr = pin_mask;  // Write 1 to BSx bits
    }

    // Set pin output LOW
    static void clear_output(uint32_t pin_mask) {
        volatile uint32_t* bsrr = reinterpret_cast<volatile uint32_t*>(GPIO_BASE + BSRR_OFFSET);
        *bsrr = (pin_mask << 16);  // Write 1 to BRx bits
    }

    // Toggle pin output
    static void toggle_output(uint32_t pin_mask) {
        volatile uint32_t* odr = reinterpret_cast<volatile uint32_t*>(GPIO_BASE + ODR_OFFSET);
        *odr ^= pin_mask;
    }

    // Read pin input
    static bool read_input(uint32_t pin_mask) {
        volatile uint32_t* idr = reinterpret_cast<volatile uint32_t*>(GPIO_BASE + IDR_OFFSET);
        return (*idr & pin_mask) != 0;
    }

    // Configure pin mode (2 bits per pin)
    static void set_mode_output(uint8_t pin_num) {
        volatile uint32_t* moder = reinterpret_cast<volatile uint32_t*>(GPIO_BASE + MODER_OFFSET);
        *moder = (*moder & ~(0x3 << (pin_num * 2))) | (0x1 << (pin_num * 2));
    }

    static void set_mode_input(uint8_t pin_num) {
        volatile uint32_t* moder = reinterpret_cast<volatile uint32_t*>(GPIO_BASE + MODER_OFFSET);
        *moder &= ~(0x3 << (pin_num * 2));  // 00 = input
    }

    static void set_mode_alternate(uint8_t pin_num) {
        volatile uint32_t* moder = reinterpret_cast<volatile uint32_t*>(GPIO_BASE + MODER_OFFSET);
        *moder = (*moder & ~(0x3 << (pin_num * 2))) | (0x2 << (pin_num * 2));
    }

    // ... more policy methods (pull-up/down, output type, speed, etc.)
};

} // namespace alloy::hal::st::stm32g0
```

### Step 7: Create Platform Implementations

Now implement the user-facing APIs that satisfy concepts.

**File:** `src/hal/vendors/st/stm32g0/clock_platform.hpp`

```cpp
#pragma once

#include "core/result.hpp"
#include "core/error.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/rcc_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/registers/flash_registers.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/rcc_bitfields.hpp"
#include "hal/vendors/st/stm32g0/generated/bitfields/flash_bitfields.hpp"
#include <cstdint>

#if __cplusplus >= 202002L
#include "hal/core/concepts.hpp"
#endif

namespace alloy::hal::st::stm32g0 {

using namespace alloy::core;

template <typename Config>
class Stm32g0Clock {
public:
    static Result<void, ErrorCode> initialize() {
        // Configure PLL, set flash latency, switch system clock
        // ... implementation
        return Ok();
    }

    static Result<void, ErrorCode> enable_gpio_clocks() {
        // Enable all GPIO port clocks
        // ... implementation
        return Ok();
    }

    static Result<void, ErrorCode> enable_uart_clock(uint32_t uart_base) {
        // Enable UART clock based on base address
        // ... implementation
        return Ok();
    }

    static Result<void, ErrorCode> enable_spi_clock(uint32_t spi_base) {
        // ... implementation
        return Ok();
    }

    static Result<void, ErrorCode> enable_i2c_clock(uint32_t i2c_base) {
        // ... implementation
        return Ok();
    }

    static constexpr uint32_t get_system_clock_hz() {
        return Config::system_clock_hz;
    }
};

// Concept validation
#if __cplusplus >= 202002L
struct ExampleG0ClockConfig {
    static constexpr uint32_t system_clock_hz = 64'000'000;
    static constexpr uint32_t pll_m = 0;
    static constexpr uint32_t pll_n = 8;
    static constexpr uint32_t pll_r = 0;
    static constexpr uint32_t flash_latency = 2;
};

static_assert(alloy::hal::concepts::ClockPlatform<Stm32g0Clock<ExampleG0ClockConfig>>,
              "Stm32g0Clock must satisfy ClockPlatform concept");
#endif

} // namespace alloy::hal::st::stm32g0
```

**See full example:** `src/hal/vendors/st/stm32g0/clock_platform.hpp`

### Step 8: Create CMake Platform File

**File:** `cmake/platforms/stm32g0.cmake`

```cmake
# Platform: STM32G0 (ST Microelectronics)
# Core:     ARM Cortex-M0+
# Vendor:   ST

set(ALLOY_PLATFORM "stm32g0")
set(ALLOY_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32g0")

# Collect platform headers (non-recursive GLOB to exclude startup.cpp)
file(GLOB ALLOY_PLATFORM_HEADERS
    "${ALLOY_PLATFORM_DIR}/*.hpp"
    "${ALLOY_PLATFORM_DIR}/generated/**/*.hpp"
)

# CPU flags
set(CPU_FLAGS
    -mcpu=cortex-m0plus
    -mthumb
    -mfloat-abi=soft
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CPU_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CPU_FLAGS}")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} ${CPU_FLAGS} -T${LINKER_SCRIPT} -Wl,-Map=${CMAKE_PROJECT_NAME}.map"
)

message(STATUS "Platform: ${ALLOY_PLATFORM}")
message(STATUS "Platform directory: ${ALLOY_PLATFORM_DIR}")
```

### Step 9: Register Platform in Board-to-Platform Mapping

**File:** `cmake/platform_selection.cmake`

```cmake
function(alloy_board_to_platform board_name out_platform)
    if(board_name MATCHES "^nucleo_g0")
        set(${out_platform} "stm32g0" PARENT_SCOPE)
    elseif(board_name MATCHES "^nucleo_f4")
        set(${out_platform} "stm32f4" PARENT_SCOPE)
    # ... more platforms
    endif()
endfunction()
```

---

## Example: Adding STM32G0

Let's walk through the real STM32G0 port that was added to Alloy.

### Files Created

```
src/hal/vendors/st/stm32g0/
├── generated/                       # From SVD
│   ├── .generated
│   ├── registers/  (33 files)
│   └── bitfields/  (33 files)
│
├── stm32g0b1/
│   ├── peripherals.hpp              # ✅ Created
│   └── startup.cpp                  # ✅ Created
│
├── gpio_hardware_policy.hpp         # ✅ Created (or generated)
├── uart_hardware_policy.hpp         # ✅ Created
├── spi_hardware_policy.hpp          # ✅ Created
├── gpio.hpp                         # ✅ Created
├── clock_platform.hpp               # ✅ Created
└── systick_platform.hpp             # ✅ Created

cmake/platforms/stm32g0.cmake        # ✅ Created
boards/nucleo_g0b1re/                # ✅ Created (test board)
```

### Time Investment

- **Code generation**: 30 minutes (mostly automated)
- **peripherals.hpp**: 15 minutes (copy from reference manual)
- **startup.cpp**: 45 minutes (generated, then reviewed)
- **Hardware policies**: 2 hours (GPIO, UART, SPI)
- **Platform implementations**: 3 hours (Clock, GPIO)
- **CMake integration**: 30 minutes
- **Testing**: 2 hours (board bring-up, debugging)

**Total**: ~8.5 hours for complete platform support

---

## Testing Your Platform

### Test 1: Build Blink Example

```bash
cmake -B build-test \
    -DALLOY_BOARD=your_test_board \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake

cmake --build build-test --target blink
```

**Expected:**
```
[100%] Built target blink
Memory region         Used Size  Region Size  %age Used
           FLASH:        2648 B       512 KB      0.51%
             RAM:        8232 B       144 KB      5.58%
```

### Test 2: Concept Validation

Concepts should validate at compile-time:

```cpp
// In clock_platform.hpp
static_assert(alloy::hal::concepts::ClockPlatform<Stm32g0Clock<ExampleConfig>>,
              "Stm32g0Clock must satisfy ClockPlatform concept");

// In gpio.hpp
static_assert(alloy::hal::concepts::GpioPin<GpioPin<0x50000000, 5>>,
              "STM32G0 GpioPin must satisfy GpioPin concept");
```

If concept not satisfied, you'll get clear error message at compile-time.

### Test 3: Hardware Validation

```bash
# Flash to real hardware
openocd -f interface/stlink.cfg -f target/stm32g0x.cfg \
    -c "program build-test/examples/blink/blink.elf verify reset exit"
```

**Verify:**
- ✓ LED blinks at correct rate
- ✓ System clock is correct frequency (measure with oscilloscope)
- ✓ No hard faults or crashes

---

## Concept Validation

### Required Concepts

All platforms must satisfy:

1. **ClockPlatform** - Clock configuration and peripheral enables
2. **GpioPin** - GPIO pin control

### Recommended Concepts

Implement if you want full HAL support:

3. **UartPeripheral** - UART communication
4. **SpiPeripheral** - SPI communication

### How to Validate

Add `static_assert` at end of each implementation file:

```cpp
#if __cplusplus >= 202002L
#include "hal/core/concepts.hpp"

// Example config for testing
struct ExampleConfig { /* ... */ };

// Validate concept
static_assert(alloy::hal::concepts::ClockPlatform<YourClock<ExampleConfig>>,
              "YourClock must satisfy ClockPlatform concept - missing required methods");
#endif
```

**Compilation will fail** if concept requirements not met, with helpful error message.

---

## Troubleshooting

### Problem: "undefined reference to `Reset_Handler`"

**Cause:** Startup code not being linked

**Solution:** Ensure startup.cpp is included in platform sources:
```cmake
# In cmake/platforms/your_platform.cmake
set(ALLOY_STARTUP_SOURCE "${ALLOY_PLATFORM_DIR}/${ALLOY_MCU}/startup.cpp")
```

### Problem: Hard fault on startup

**Common causes:**
1. **Wrong vector table** - Check IRQ numbers match reference manual
2. **Stack overflow** - Increase stack size in linker script
3. **Invalid clock config** - Verify PLL calculations

**Debug:**
```cpp
// Add at start of Reset_Handler
volatile int* debug_marker = (volatile int*)0x20000000;
*debug_marker = 0xDEADBEEF;  // Write to RAM
```

If this crashes, problem is before Reset_Handler (vector table issue).

### Problem: Concept validation fails

**Example error:**
```
error: static assertion failed: "Clock must satisfy ClockPlatform concept"
note: constraints not satisfied because 'enable_uart_clock' is missing
```

**Solution:** Implement missing method:
```cpp
static Result<void, ErrorCode> enable_uart_clock(uint32_t uart_base) {
    // ... implementation
    return Ok();
}
```

---

## Best Practices

### 1. Auto-Generate When Possible

```bash
# Generate hardware policies from SVD
python3 tools/codegen/generators/hardware_policy.py \
    --svd STM32G0B1.svd \
    --peripheral GPIO \
    --output src/hal/vendors/st/stm32g0/
```

### 2. Document Register Addresses

```cpp
// ✅ Good - Include reference manual section
// From RM0444 Section 2.2.2: RCC base = 0x4002_1000
constexpr u32 RCC = 0x40021000;

// ❌ Bad - No context
constexpr u32 RCC = 0x40021000;
```

### 3. Validate All Bitfield Operations

```cpp
// ✅ Good - Use generated bitfields
using namespace rcc::cr;
RCC->CR |= HSEON::mask;  // Type-safe, validated

// ❌ Bad - Magic numbers
RCC->CR |= (1 << 16);  // What does bit 16 mean?
```

### 4. Add Platform-Specific Tests

```cpp
// Test clock frequency
static_assert(BoardClockConfig::system_clock_hz == 64'000'000,
              "System clock must be 64 MHz for STM32G0B1");

// Test PLL configuration
static constexpr uint32_t vco_freq = /* calculation */;
static_assert(vco_freq >= 64'000'000 && vco_freq <= 344'000'000,
              "VCO frequency out of range for STM32G0");
```

---

## Next Steps

After platform is working:

1. **Add more boards** - See [PORTING_NEW_BOARD.md](PORTING_NEW_BOARD.md)
2. **Implement more peripherals** - UART, SPI, I2C, ADC, PWM
3. **Add examples** - Blink, UART echo, sensor reading
4. **Create PR** - Share your platform with the community!

---

**Questions?** Check [ARCHITECTURE.md](ARCHITECTURE.md) or open an issue on GitHub.
