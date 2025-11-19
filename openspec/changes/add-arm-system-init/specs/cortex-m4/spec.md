# Spec: ARM Cortex-M4 Specific Features

## Overview
Features specific to ARM Cortex-M4 and M4F (with FPU):
- **FPU**: Single precision floating point (optional on M4F)
- **DSP**: DSP instructions (SIMD)
- **MPU**: Memory Protection Unit (8 regions)

## Files

### `src/startup/arm_cortex_m4/fpu_m4.hpp`
**Purpose**: Enable and configure Cortex-M4F FPU (single precision)

```cpp
#pragma once

#include "startup/arm_cortex_m/core_common.hpp"
#include "core/types.hpp"

namespace alloy::startup::arm::cortex_m4 {

using namespace alloy::core;

/**
 * Enable FPU for Cortex-M4F
 *
 * Requirements:
 * - __FPU_PRESENT must be 1 (compile-time check)
 * - Must be called BEFORE using floating point
 *
 * Configuration:
 * - CP10 and CP11: Full access (privileged and unprivileged)
 * - Lazy stacking: Enabled (FPU context saved only when needed)
 *
 * Performance impact:
 * - FPU disabled: float/double operations use software emulation (10-100x slower)
 * - FPU enabled: hardware acceleration (~1-4 cycles per operation)
 */
inline void enable_fpu() {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1) && defined(__FPU_USED) && (__FPU_USED == 1)

    // Enable CP10 and CP11 coprocessors (FPU)
    // CPACR: Coprocessor Access Control Register
    SCB->CPACR |= scb_cpacr::FPU_FULL_ACCESS;

    // Data Synchronization Barrier
    // Ensures CP10/CP11 access is enabled before continuing
    dsb();

    // Instruction Synchronization Barrier
    // Ensures pipeline is flushed and new FPU instructions can be fetched
    isb();

    #endif
}

/**
 * Disable FPU for Cortex-M4F
 * Useful for power saving when FPU not needed
 */
inline void disable_fpu() {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)

    // Disable CP10 and CP11
    SCB->CPACR &= ~scb_cpacr::FPU_FULL_ACCESS;

    dsb();
    isb();

    #endif
}

/**
 * Check if FPU is enabled
 * @return true if FPU is enabled
 */
inline bool is_fpu_enabled() {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
    return (SCB->CPACR & scb_cpacr::FPU_FULL_ACCESS) == scb_cpacr::FPU_FULL_ACCESS;
    #else
    return false;
    #endif
}

/**
 * FPU Context Control Register
 * Located at 0xE000EF34
 */
struct FPCCR_Registers {
    volatile u32 FPCCR;  // FP Context Control
    volatile u32 FPCAR;  // FP Context Address
    volatile u32 FPDSCR; // FP Default Status Control
};

inline FPCCR_Registers* const FPCCR = reinterpret_cast<FPCCR_Registers*>(0xE000EF34);

// FPCCR bits
namespace fpccr_bits {
    constexpr u32 LSPACT = (1U << 0);   // Lazy state preservation active
    constexpr u32 USER = (1U << 1);     // FPU used by unprivileged code
    constexpr u32 THREAD = (1U << 3);   // FPU used in thread mode
    constexpr u32 HFRDY = (1U << 4);    // HardFault ready
    constexpr u32 MMRDY = (1U << 5);    // MemManage ready
    constexpr u32 BFRDY = (1U << 6);    // BusFault ready
    constexpr u32 MONRDY = (1U << 8);   // DebugMonitor ready
    constexpr u32 LSPEN = (1U << 30);   // Lazy state preservation enable
    constexpr u32 ASPEN = (1U << 31);   // Automatic state preservation enable
}

/**
 * Configure FPU lazy stacking
 * Lazy stacking saves FPU context only when needed, reducing interrupt latency
 *
 * @param enable true to enable lazy stacking
 */
inline void configure_lazy_stacking(bool enable) {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)

    if (enable) {
        FPCCR->FPCCR |= fpccr_bits::LSPEN | fpccr_bits::ASPEN;
    } else {
        FPCCR->FPCCR &= ~(fpccr_bits::LSPEN | fpccr_bits::ASPEN);
    }

    #endif
}

} // namespace alloy::startup::arm::cortex_m4
```

---

### `src/startup/arm_cortex_m4/cortex_m4_init.hpp`
**Purpose**: Cortex-M4 initialization (FPU + DSP)

```cpp
#pragma once

#include "startup/arm_cortex_m4/fpu_m4.hpp"
#include "startup/arm_cortex_m/core_common.hpp"

namespace alloy::startup::arm::cortex_m4 {

/**
 * Initialize Cortex-M4 specific features
 *
 * Features enabled:
 * - FPU (if present and used)
 * - Lazy FPU stacking (if FPU present)
 * - DSP instructions (always available on M4)
 *
 * Call this from SystemInit() BEFORE any floating point operations
 */
inline void init() {
    // Enable FPU (if available)
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1) && defined(__FPU_USED) && (__FPU_USED == 1)
    enable_fpu();
    configure_lazy_stacking(true);  // Enable lazy stacking for lower interrupt latency
    #endif

    // DSP instructions are always available on M4, no initialization needed
    // They're enabled automatically by the CPU
}

} // namespace alloy::startup::arm::cortex_m4
```

## Compiler Flags Required

### For M4 without FPU
```cmake
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-m4")
```

### For M4F with FPU (Hard Float ABI)
```cmake
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
add_compile_definitions(__FPU_PRESENT=1 __FPU_USED=1)
```

### For M4F with FPU (Soft Float ABI - not recommended)
```cmake
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp")
add_compile_definitions(__FPU_PRESENT=1 __FPU_USED=1)
```

## Usage Example

```cpp
// In board/stm32f407vg/startup.cpp
#include "startup/arm_cortex_m4/cortex_m4_init.hpp"

extern "C" void SystemInit() {
    // Initialize M4-specific features (FPU, etc)
    alloy::startup::arm::cortex_m4::init();

    // Now safe to use floating point
    float x = 3.14f;
    float y = x * 2.0f;  // Uses hardware FPU
}
```

## Performance Comparison

| Operation | Software Emulation | Hardware FPU | Speedup |
|-----------|-------------------|--------------|---------|
| `float add` | ~40 cycles | 1 cycle | 40x |
| `float mul` | ~50 cycles | 1 cycle | 50x |
| `float div` | ~100 cycles | 14 cycles | 7x |
| `float sqrt` | ~200 cycles | 14 cycles | 14x |

## Testing
- ✅ Compile with FPU enabled/disabled
- ✅ Verify FPU operations use hardware (check disassembly)
- ✅ Benchmark floating point performance
- ✅ Test lazy stacking reduces interrupt latency

## Dependencies
- `startup/arm_cortex_m/core_common.hpp`
- `core/types.hpp`

## References
- ARM Cortex-M4 Processor Technical Reference Manual
- ARM Cortex-M4 Devices Generic User Guide
- "Single Precision Floating Point Unit" chapter
