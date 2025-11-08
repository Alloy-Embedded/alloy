// Alloy Framework - ARM Cortex-M4 Initialization
//
// Provides initialization for Cortex-M4 and Cortex-M4F cores
//
// Features initialized:
// - FPU (if present and __FPU_PRESENT == 1)
// - Memory barriers for proper synchronization
//
// Usage in SystemInit():
//   #include "startup/arm_cortex_m4/cortex_m4_init.hpp"
//
//   void SystemInit() {
//       alloy::arm::cortex_m4::initialize();
//       // ... vendor-specific initialization
//   }

#pragma once

#include "../arm_cortex_m/core_common.hpp"
#include "fpu_m4.hpp"

namespace alloy::arm::cortex_m4 {

/// Initialize Cortex-M4 core features
/// @param enable_fpu: Enable FPU if present (default: true)
/// @param enable_lazy_fpu_stacking: Enable lazy FPU context switching (default: true)
///
/// This function initializes:
/// 1. FPU (if present) with optional lazy stacking
/// 2. Memory barriers for proper synchronization
///
/// Should be called early in SystemInit() before any floating point operations.
inline void initialize(bool enable_fpu_flag = true, bool enable_lazy_fpu_stacking = true) {
// 1. Enable FPU if present and requested
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
    if (enable_fpu_flag) {
        fpu::initialize(enable_lazy_fpu_stacking);
    }
#else
    (void)enable_fpu_flag;  // Suppress unused parameter warning
    (void)enable_lazy_fpu_stacking;
#endif

    // 2. Memory barriers to ensure all configuration is complete
    dsb();
    isb();
}

/// Initialize Cortex-M4 with FPU disabled
/// Use this if you want to initialize the core but explicitly disable FPU
inline void initialize_without_fpu() {
    initialize(false, false);
}

}  // namespace alloy::arm::cortex_m4
