# Generated Peripheral Definitions

## Overview

The Alloy code generator automatically creates peripheral headers from MCU database JSON files. This eliminates the need to hardcode memory addresses and register definitions for each MCU.

## How It Works

```
SVD File → Parser → JSON Database → Generator → peripherals.hpp
                                              → startup.cpp
```

### 1. Parse SVD to JSON

```bash
python3 svd_parser.py \
    --input STM32F103.svd \
    --output database/families/stm32f1xx.json
```

The JSON contains everything:
```json
{
  "peripherals": {
    "GPIO": {
      "instances": [
        {"name": "GPIOA", "base": "0x40010800"},
        {"name": "GPIOB", "base": "0x40010C00"}
      ],
      "registers": {
        "CRL": {"offset": "0x00", "size": 32},
        "CRH": {"offset": "0x04", "size": 32},
        "IDR": {"offset": "0x08", "size": 32}
      }
    }
  }
}
```

### 2. Generate Headers

```bash
python3 generator.py \
    --mcu STM32F103C8 \
    --database database/families/stm32f1xx.json \
    --output build/generated/STM32F103C8
```

Generates:
- `startup.cpp` - Startup code with vector table
- `peripherals.hpp` - All peripheral definitions

### 3. Use in C++ Code

```cpp
#ifdef ALLOY_USE_GENERATED_PERIPHERALS
#include "peripherals.hpp"

namespace alloy::hal::stm32f1 {
    // Use generated definitions
    using GpioPort = alloy::generated::stm32f103c8::gpio::Registers;
}
#else
// Fallback to hardcoded definitions
#endif
```

## Generated Header Structure

```cpp
namespace alloy::generated::stm32f103c8 {

/// Memory map
namespace memory {
    constexpr uint32_t FLASH_BASE = 0x08000000;
    constexpr uint32_t FLASH_SIZE = 64 * 1024;
    constexpr uint32_t RAM_BASE   = 0x20000000;
    constexpr uint32_t RAM_SIZE   = 20 * 1024;
}

// GPIO Peripheral
namespace gpio {
    /// Base addresses
    constexpr uint32_t GPIOA_BASE = 0x40010800;
    constexpr uint32_t GPIOB_BASE = 0x40010C00;
    constexpr uint32_t GPIOC_BASE = 0x40011000;

    /// GPIO Register structure
    struct Registers {
        volatile uint32_t CRL;   ///< Port config register low
        volatile uint32_t CRH;   ///< Port config register high
        volatile uint32_t IDR;   ///< Input data register
        volatile uint32_t ODR;   ///< Output data register
        volatile uint32_t BSRR;  ///< Bit set/reset register
        volatile uint32_t BRR;   ///< Bit reset register
        volatile uint32_t LCKR;  ///< Configuration lock register
    };

    /// Peripheral instances
    inline Registers* GPIOA = reinterpret_cast<Registers*>(GPIOA_BASE);
    inline Registers* GPIOB = reinterpret_cast<Registers*>(GPIOB_BASE);
    inline Registers* GPIOC = reinterpret_cast<Registers*>(GPIOC_BASE);
}

// USART Peripheral
namespace usart {
    constexpr uint32_t USART1_BASE = 0x40013800;
    constexpr uint32_t USART2_BASE = 0x40004400;

    struct Registers {
        volatile uint32_t SR;    ///< Status register
        volatile uint32_t DR;    ///< Data register
        volatile uint32_t BRR;   ///< Baud rate register
        volatile uint32_t CR1;   ///< Control register 1
        volatile uint32_t CR2;   ///< Control register 2
        volatile uint32_t CR3;   ///< Control register 3
        volatile uint32_t GTPR;  ///< Guard time and prescaler
    };

    inline Registers* USART1 = reinterpret_cast<Registers*>(USART1_BASE);
    inline Registers* USART2 = reinterpret_cast<Registers*>(USART2_BASE);
    inline Registers* USART3 = reinterpret_cast<Registers*>(USART3_BASE);
}

} // namespace alloy::generated::stm32f103c8
```

## CMake Integration

The board configuration automatically generates headers:

```cmake
# cmake/boards/bluepill.cmake

include(codegen)

if(ALLOY_CODEGEN_AVAILABLE)
    alloy_generate_code(
        MCU STM32F103C8
        FAMILY stm32f1xx
    )

    # ALLOY_GENERATED_DIR is now available
    include_directories(${ALLOY_GENERATED_DIR})
endif()
```

The HAL automatically uses generated definitions if available:

```cmake
# src/hal/stm32f1/CMakeLists.txt

if(ALLOY_GENERATED_DIR AND EXISTS "${ALLOY_GENERATED_DIR}/peripherals.hpp")
    target_include_directories(alloy_hal_stm32f1 PUBLIC ${ALLOY_GENERATED_DIR})
    target_compile_definitions(alloy_hal_stm32f1 PUBLIC ALLOY_USE_GENERATED_PERIPHERALS)
    message(STATUS "  Using generated peripheral definitions")
else()
    message(STATUS "  Using hardcoded peripheral definitions (fallback)")
endif()
```

## Benefits

### ✅ Scalability
- **Before**: Manually write addresses for each MCU (hours of work)
- **After**: Parse SVD once, support entire family (minutes)

### ✅ Accuracy
- **Before**: Typos in hardcoded addresses
- **After**: Parsed directly from official vendor SVD files

### ✅ Maintainability
- **Before**: Update addresses manually when vendor changes specs
- **After**: Re-parse SVD file

### ✅ Coverage
- **Before**: Only implement needed peripherals
- **After**: ALL peripherals available from SVD (24 types for STM32F1)

## Example: Adding STM32F103CB Support

The STM32F103CB is like the C8 but with 128KB flash instead of 64KB.

### 1. Parse SVD (if needed)

```bash
python3 svd_parser.py \
    --input upstream/cmsis-svd-data/data/STMicro/STM32F103xx.svd \
    --output database/families/stm32f1xx.json \
    --merge  # Add to existing database
```

### 2. That's it!

The code generator automatically handles it because it reads from the JSON:

```bash
# Build for C8 (64KB)
cmake -B build -DALLOY_BOARD=bluepill -DALLOY_MCU=STM32F103C8

# Build for CB (128KB) - just change MCU name!
cmake -B build -DALLOY_BOARD=bluepill -DALLOY_MCU=STM32F103CB
```

**No code changes needed** - the peripherals.hpp is generated with correct addresses for whichever MCU you select!

## Adding a New STM32F1 MCU

Let's say we want to add STM32F103RE (512KB flash, 64KB RAM):

### Step 1: Parse its SVD

```bash
python3 svd_parser.py \
    --input upstream/cmsis-svd-data/data/STMicro/STM32F103xx.svd \
    --output database/families/stm32f1xx.json \
    --merge
```

This adds STM32F103RE to the JSON with its specific memory map.

### Step 2: Create board file (optional)

```cmake
# cmake/boards/nucleo_f103.cmake
set(ALLOY_MCU "STM32F103RE")
set(ALLOY_ARCH "arm-cortex-m3")
set(ALLOY_FLASH_SIZE "512KB")
set(ALLOY_RAM_SIZE "64KB")

include(codegen)
alloy_generate_code(MCU STM32F103RE FAMILY stm32f1xx)
```

### Step 3: Build!

```bash
cmake -B build -DALLOY_BOARD=nucleo_f103
cmake --build build
```

**Total time: ~5 minutes**

Compare this to manually implementing:
- GPIO registers for 7 ports
- USART registers for 5 UARTs
- All other peripheral addresses
- Interrupt vector table (71 vectors)

**Manual time: Hours to days**

## Template Customization

The peripheral header template is in:
```
tools/codegen/templates/peripherals/stm32_peripherals.hpp.j2
```

You can customize it to generate:
- Bit field definitions
- Helper macros
- Typed peripheral handles
- Register-specific documentation

See `docs/TEMPLATES.md` for details.

## Fallback Mode

If generated headers are not available (e.g., building without code generation), the HAL falls back to hardcoded definitions:

```cpp
#ifdef ALLOY_USE_GENERATED_PERIPHERALS
    // Use generated: Scales to all MCUs
    using GpioPort = alloy::generated::stm32f103c8::gpio::Registers;
#else
    // Fallback: Works but doesn't scale
    struct GpioPort {
        volatile uint32_t CRL;
        volatile uint32_t CRH;
        // ...
    };
#endif
```

This ensures the code always compiles, even without the generator.

## Future Enhancements

Potential template additions:

1. **Bit field definitions**
```cpp
namespace gpio::odr_bits {
    constexpr uint32_t ODR0 = (1U << 0);
    constexpr uint32_t ODR1 = (1U << 1);
    // ...
}
```

2. **Type-safe peripheral handles**
```cpp
template<typename PeriphT>
class PeripheralHandle {
    PeriphT* regs_;
public:
    auto& operator*() { return *regs_; }
    auto* operator->() { return regs_; }
};
```

3. **Clock enable helpers**
```cpp
namespace gpio {
    inline void enable_clock(Port port) {
        RCC->APB2ENR |= (1U << (2 + static_cast<uint8_t>(port)));
    }
}
```

## Summary

**Before (Hardcoded):**
- ❌ Manual work for each MCU
- ❌ Error-prone (typos in addresses)
- ❌ Hard to maintain
- ❌ Doesn't scale

**After (Generated):**
- ✅ Automatic from SVD files
- ✅ Accurate (from vendor specs)
- ✅ Easy to maintain (re-parse SVD)
- ✅ Scales to hundreds of MCUs

**The code generator + JSON database eliminates 90% of the manual work when adding new MCUs!**
