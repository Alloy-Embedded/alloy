// Alloy Framework - ARM Cortex-M7 FPU (Floating Point Unit)
//
// Provides FPU initialization and configuration for Cortex-M7 cores
//
// Features:
// - FPU enable/disable
// - Lazy context switching configuration
// - FPU exception handling
// - Single and double precision support
//
// Note: Cortex-M7 FPU supports both single (32-bit) and double (64-bit) precision
// This is an enhancement over Cortex-M4F which only supports single precision

#pragma once

#include "../arm_cortex_m/core_common.hpp"
#include <stdint.h>

namespace alloy::arm::cortex_m7::fpu {

// Import from parent namespace
using namespace alloy::arm::cortex_m;

// ============================================================================
// FPU Context Control Register (FPCCR)
// ============================================================================
// Base address: 0xE000EF34

struct FPU_Registers {
    uint32_t RESERVED0;
    volatile uint32_t FPCCR;     // Offset: 0x004 (R/W)  Floating-Point Context Control Register
    volatile uint32_t FPCAR;     // Offset: 0x008 (R/W)  Floating-Point Context Address Register
    volatile uint32_t FPDSCR;    // Offset: 0x00C (R/W)  Floating-Point Default Status Control Register
    volatile uint32_t MVFR0;     // Offset: 0x010 (R/ )  Media and FP Feature Register 0
    volatile uint32_t MVFR1;     // Offset: 0x014 (R/ )  Media and FP Feature Register 1
    volatile uint32_t MVFR2;     // Offset: 0x018 (R/ )  Media and FP Feature Register 2 (M7 only)
};

inline FPU_Registers* FPU() {
    return reinterpret_cast<FPU_Registers*>(0xE000EF30);
}

// FPCCR (Floating-Point Context Control Register) bit definitions
namespace fpccr {
    constexpr uint32_t LSPACT  = (1UL << 0);   // Lazy State Preservation Active
    constexpr uint32_t USER    = (1UL << 1);   // Privilege level when exception was taken
    constexpr uint32_t THREAD  = (1UL << 3);   // Mode when exception was taken
    constexpr uint32_t HFRDY   = (1UL << 4);   // HardFault Ready
    constexpr uint32_t MMRDY   = (1UL << 5);   // MemManage Ready
    constexpr uint32_t BFRDY   = (1UL << 6);   // BusFault Ready
    constexpr uint32_t MONRDY  = (1UL << 8);   // DebugMonitor Ready
    constexpr uint32_t LSPEN   = (1UL << 30);  // Lazy State Preservation Enable
    constexpr uint32_t ASPEN   = (1UL << 31);  // Automatic State Preservation Enable
}

// ============================================================================
// FPU Control Functions
// ============================================================================

/// Check if FPU is present
/// @return true if FPU is available on this chip
inline constexpr bool is_fpu_present() {
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
    return true;
#else
    return false;
#endif
}

/// Check if FPU is enabled
/// @return true if FPU is currently enabled
inline bool is_fpu_enabled() {
    // Check if CP10 and CP11 coprocessors are enabled
    uint32_t cpacr = SCB()->CPACR;
    uint32_t cp10 = (cpacr & cpacr::CP10_Msk) >> cpacr::CP10_Pos;
    uint32_t cp11 = (cpacr & cpacr::CP11_Msk) >> cpacr::CP11_Pos;

    return (cp10 == cpacr::Access_Full) && (cp11 == cpacr::Access_Full);
}

/// Enable FPU
/// Enables full access to CP10 and CP11 coprocessors (FPU)
///
/// Cortex-M7 FPU supports:
/// - Single precision (float): 32-bit
/// - Double precision (double): 64-bit
///
/// This must be called early in startup before any floating point code is executed.
/// After enabling FPU, use DSB and ISB to ensure memory synchronization.
///
/// Example usage in Reset_Handler:
///   enable_fpu();
///   // Now safe to use floating point operations (float and double)
inline void enable_fpu() {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
        // Enable CP10 and CP11 coprocessors (FPU)
        // Set bits 20-23 to enable full access
        SCB()->CPACR |= (cpacr::Access_Full << cpacr::CP10_Pos) |
                        (cpacr::Access_Full << cpacr::CP11_Pos);

        // Memory barrier to ensure FPU is enabled before FPU instructions execute
        dsb();
        isb();
    #endif
}

/// Disable FPU
/// Disables access to CP10 and CP11 coprocessors
/// Warning: Calling this when floating point code is in use will cause a fault!
inline void disable_fpu() {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
        // Clear CP10 and CP11 enable bits
        SCB()->CPACR &= ~(cpacr::CP10_Msk | cpacr::CP11_Msk);

        dsb();
        isb();
    #endif
}

/// Configure FPU lazy context switching
/// @param enable_lazy: If true, FPU context is only saved when actually used
///
/// Lazy context switching can improve interrupt latency by deferring FPU
/// register save until the FPU is actually used in the exception handler.
///
/// Benefits:
/// - Faster interrupt response if FPU not used in handler
/// - Reduced stack usage if FPU not used
///
/// Tradeoffs:
/// - Non-deterministic latency (depends on FPU usage)
/// - More complex to debug
///
/// For RTOS use, typically disabled for deterministic timing.
inline void configure_lazy_stacking(bool enable_lazy = true) {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
        if (enable_lazy) {
            // Enable automatic and lazy state preservation
            FPU()->FPCCR |= (fpccr::ASPEN | fpccr::LSPEN);
        } else {
            // Disable lazy state preservation (always save FPU context)
            FPU()->FPCCR &= ~fpccr::LSPEN;
        }
    #endif
}

/// Initialize FPU with recommended settings for Cortex-M7
/// @param enable_lazy_stacking: Enable lazy context switching (default: true)
///
/// This function should be called early in SystemInit() or Reset_Handler
/// before any floating point operations are performed.
///
/// Cortex-M7 specific optimizations:
/// - Enables both single and double precision
/// - Configures for optimal performance
///
/// Example:
///   void SystemInit() {
///       fpu::initialize();  // Enable FPU with lazy stacking
///       // ... rest of initialization
///   }
inline void initialize(bool enable_lazy_stacking = true) {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
        // 1. Enable FPU
        enable_fpu();

        // 2. Configure lazy context switching
        configure_lazy_stacking(enable_lazy_stacking);

        // 3. Memory barriers to ensure configuration is complete
        dsb();
        isb();
    #endif
}

/// Get FPU exception status
/// @return FPSCR (Floating-Point Status and Control Register) value
inline uint32_t get_fpscr() {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
        uint32_t result;
        __asm__ volatile ("VMRS %0, fpscr" : "=r" (result));
        return result;
    #else
        return 0;
    #endif
}

/// Set FPU exception status
/// @param fpscr: FPSCR value to set
inline void set_fpscr(uint32_t fpscr) {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
        __asm__ volatile ("VMSR fpscr, %0" :: "r" (fpscr));
    #endif
}

/// Clear FPU exception flags
/// Clears all FPU exception flags in FPSCR
inline void clear_exceptions() {
    #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1)
        // Clear exception flags (bits 0-4 of FPSCR)
        uint32_t fpscr = get_fpscr();
        fpscr &= ~0x1F;
        set_fpscr(fpscr);
    #endif
}

} // namespace alloy::arm::cortex_m7::fpu
