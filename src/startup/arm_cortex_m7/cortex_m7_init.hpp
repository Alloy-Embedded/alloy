// Alloy Framework - ARM Cortex-M7 Initialization
//
// Provides initialization for Cortex-M7 cores
//
// Features initialized:
// - FPU (single and double precision if present)
// - Instruction Cache (I-Cache)
// - Data Cache (D-Cache)
// - Memory barriers for proper synchronization
//
// Usage in SystemInit():
//   #include "startup/arm_cortex_m7/cortex_m7_init.hpp"
//
//   void SystemInit() {
//       alloy::arm::cortex_m7::initialize();
//       // ... vendor-specific initialization
//   }

#pragma once

#include "fpu_m7.hpp"
#include "cache_m7.hpp"
#include "../arm_cortex_m/core_common.hpp"

namespace alloy::arm::cortex_m7 {

// Import from parent namespace
using namespace alloy::arm::cortex_m;

/// Initialize Cortex-M7 core features
/// @param enable_fpu: Enable FPU if present (default: true)
/// @param enable_lazy_fpu_stacking: Enable lazy FPU context switching (default: true)
/// @param enable_icache: Enable instruction cache (default: true)
/// @param enable_dcache: Enable data cache (default: true)
///
/// This function initializes:
/// 1. FPU (if present) with optional lazy stacking - supports single and double precision
/// 2. Instruction Cache (I-Cache) for faster code execution
/// 3. Data Cache (D-Cache) for faster data access
/// 4. Memory barriers for proper synchronization
///
/// Performance improvement:
/// - 10-100x faster floating point (hardware FPU vs software emulation)
/// - 2-3x faster code execution (I-Cache enabled)
/// - 2-3x faster data access (D-Cache enabled)
/// - Overall: 3-10x system performance improvement
///
/// Should be called early in SystemInit() before any floating point operations.
///
/// Example:
///   void SystemInit() {
///       cortex_m7::initialize();  // Enable all features
///       // Now safe to use float, double, and benefit from caching
///   }
inline void initialize(
    bool enable_fpu_flag = true,
    bool enable_lazy_fpu_stacking = true,
    bool enable_icache = true,
    bool enable_dcache = true
) {
    // 1. Enable FPU if present and requested
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
        if (enable_fpu_flag) {
            fpu::initialize(enable_lazy_fpu_stacking);
        }
    #else
        (void)enable_fpu_flag;
        (void)enable_lazy_fpu_stacking;
    #endif

    // 2. Enable caches if present and requested
    cache::initialize(enable_icache, enable_dcache);

    // 3. Memory barriers to ensure all configuration is complete
    dsb();
    isb();
}

/// Initialize Cortex-M7 with FPU but without caches
/// Use this for debugging cache-related issues
inline void initialize_without_cache() {
    initialize(true, true, false, false);
}

/// Initialize Cortex-M7 with caches but without FPU
/// Use this if you want caching but no floating point
inline void initialize_without_fpu() {
    initialize(false, false, true, true);
}

/// Initialize Cortex-M7 with minimal features (no FPU, no cache)
/// Use this for debugging or minimal power consumption
inline void initialize_minimal() {
    initialize(false, false, false, false);
}

} // namespace alloy::arm::cortex_m7
