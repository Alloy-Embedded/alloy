# Spec: ARM Cortex-M7 Specific Features

## Overview
Features specific to ARM Cortex-M7:
- **FPU**: Single/double precision floating point
- **Cache**: Instruction Cache (I-Cache) + Data Cache (D-Cache)
- **MPU**: Memory Protection Unit (16 regions)
- **TCM**: Tightly Coupled Memory

## Key Files

### `src/startup/arm_cortex_m7/fpu_m7.hpp`
Identical to M4F but with double precision support.

### `src/startup/arm_cortex_m7/cache_m7.hpp`
**Purpose**: Enable and manage I-Cache + D-Cache

```cpp
#pragma once

#include "startup/arm_cortex_m/core_common.hpp"
#include "core/types.hpp"

namespace alloy::startup::arm::cortex_m7 {

using namespace alloy::core;

// Cache Control Register (CCSIDR, CCSELR, CSSELR)
struct Cache_Registers {
    volatile u32 ICIALLU;  // 0xE000EF50 - I-Cache Invalidate All
    volatile u32 RESERVED0;
    volatile u32 ICIMVAU;  // 0xE000EF58 - I-Cache Invalidate by MVA
    volatile u32 DCIMVAC;  // 0xE000EF5C - D-Cache Invalidate by MVA
    volatile u32 DCISW;    // 0xE000EF60 - D-Cache Invalidate by Set/Way
    volatile u32 DCCMVAU;  // 0xE000EF64 - D-Cache Clean by MVA
    volatile u32 DCCMVAC;  // 0xE000EF68 - D-Cache Clean by MVA
    volatile u32 DCCSW;    // 0xE000EF6C - D-Cache Clean by Set/Way
    volatile u32 DCCIMVAC; // 0xE000EF70 - D-Cache Clean+Invalidate by MVA
    volatile u32 DCCISW;   // 0xE000EF74 - D-Cache Clean+Invalidate by Set/Way
};

inline Cache_Registers* const CACHE = reinterpret_cast<Cache_Registers*>(0xE000EF50);

// CCR register bits (in SCB)
namespace ccr_bits {
    constexpr u32 IC = (1U << 17);   // Instruction cache enable
    constexpr u32 DC = (1U << 16);   // Data cache enable
}

/**
 * Invalidate entire Instruction Cache
 */
inline void invalidate_icache() {
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
    dsb();
    isb();
    CACHE->ICIALLU = 0;  // Write any value to invalidate
    dsb();
    isb();
    #endif
}

/**
 * Enable Instruction Cache
 * Call AFTER clock configuration for best performance
 */
inline void enable_icache() {
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
    invalidate_icache();
    SCB->CCR |= ccr_bits::IC;
    dsb();
    isb();
    #endif
}

/**
 * Disable Instruction Cache
 */
inline void disable_icache() {
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
    dsb();
    isb();
    SCB->CCR &= ~ccr_bits::IC;
    invalidate_icache();
    #endif
}

/**
 * Invalidate entire Data Cache
 */
inline void invalidate_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    // Invalidate all D-Cache by set/way
    for (u32 set = 0; set < 4; set++) {
        for (u32 way = 0; way < 4; way++) {
            CACHE->DCISW = ((way & 0x3) << 30) | ((set & 0x7F) << 5);
        }
    }
    dsb();
    #endif
}

/**
 * Enable Data Cache
 * Call AFTER clock configuration
 * WARNING: May cause issues with DMA if not managed properly
 */
inline void enable_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    invalidate_dcache();
    SCB->CCR |= ccr_bits::DC;
    dsb();
    isb();
    #endif
}

/**
 * Disable Data Cache
 */
inline void disable_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    dsb();
    SCB->CCR &= ~ccr_bits::DC;
    invalidate_dcache();
    #endif
}

/**
 * Clean Data Cache (write back dirty lines)
 * Use before DMA reads from memory
 */
inline void clean_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    for (u32 set = 0; set < 4; set++) {
        for (u32 way = 0; way < 4; way++) {
            CACHE->DCCSW = ((way & 0x3) << 30) | ((set & 0x7F) << 5);
        }
    }
    dsb();
    #endif
}

} // namespace alloy::startup::arm::cortex_m7
```

### `src/startup/arm_cortex_m7/cortex_m7_init.hpp`
```cpp
#pragma once

#include "startup/arm_cortex_m7/fpu_m7.hpp"
#include "startup/arm_cortex_m7/cache_m7.hpp"

namespace alloy::startup::arm::cortex_m7 {

/**
 * Initialize Cortex-M7 features
 * Call from SystemInit() AFTER clock configuration
 */
inline void init() {
    // Enable FPU (double precision supported)
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1) && defined(__FPU_USED) && (__FPU_USED == 1)
    enable_fpu();
    configure_lazy_stacking(true);
    #endif

    // Enable caches for performance
    // Note: Enable AFTER clock configuration for optimal performance
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
    enable_icache();
    #endif

    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    enable_dcache();
    #endif
}

} // namespace alloy::startup::arm::cortex_m7
```

## Compiler Flags

```cmake
# M7 with single precision FPU
-mcpu=cortex-m7 -mfpu=fpv5-sp-d16 -mfloat-abi=hard

# M7 with double precision FPU
-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard

add_compile_definitions(__FPU_PRESENT=1 __FPU_USED=1 __ICACHE_PRESENT=1 __DCACHE_PRESENT=1)
```

## Performance Impact
- **I-Cache enabled**: 2-3x faster code execution
- **D-Cache enabled**: 2-5x faster data access
- **FPU enabled**: 10-100x faster floating point

## Testing
- ✅ Verify cache enabled (check CCR register)
- ✅ Benchmark with/without cache
- ✅ Test DMA + D-Cache interaction

## References
- ARM Cortex-M7 Processor Technical Reference Manual
- "Level 1 Cache" chapter
