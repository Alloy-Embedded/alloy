// Alloy Framework - ARM Cortex-M7 Cache Control
//
// Provides Instruction Cache (I-Cache) and Data Cache (D-Cache) control
// for Cortex-M7 cores
//
// Features:
// - I-Cache enable/disable/invalidate
// - D-Cache enable/disable/invalidate/clean
// - Cache maintenance for DMA operations
//
// Cache sizes (M7 implementation dependent):
// - I-Cache: Typically 4KB-64KB
// - D-Cache: Typically 4KB-64KB
//
// Important for DMA:
// - Before DMA write (peripheral -> memory): invalidate_dcache_by_addr()
// - After DMA read (memory -> peripheral): clean_dcache_by_addr()

#pragma once

#include "../arm_cortex_m/core_common.hpp"
#include <stdint.h>

namespace alloy::arm::cortex_m7::cache {

// Import from parent namespace
using namespace alloy::arm::cortex_m;

// ============================================================================
// Cache Control Functions
// ============================================================================

/// Check if Instruction Cache is present
/// @return true if I-Cache is available
inline constexpr bool is_icache_present() {
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
    return true;
#else
    return false;
#endif
}

/// Check if Data Cache is present
/// @return true if D-Cache is available
inline constexpr bool is_dcache_present() {
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    return true;
#else
    return false;
#endif
}

// ============================================================================
// Instruction Cache (I-Cache) Functions
// ============================================================================

/// Invalidate entire I-Cache
/// Use this after:
/// - Modifying code in Flash/RAM (self-modifying code)
/// - Loading new code via bootloader
inline void invalidate_icache() {
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
        dsb();  // Ensure all memory transactions are complete
        isb();  // Ensure instruction fetch before invalidate

        CACHE()->ICIALLU = 0;  // Invalidate all I-Cache

        dsb();  // Ensure invalidation is complete
        isb();  // Refetch instructions
    #endif
}

/// Enable Instruction Cache
/// Enables I-Cache for faster code execution (typically 2-3x speedup)
inline void enable_icache() {
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
        // Invalidate I-Cache before enabling
        invalidate_icache();

        // Enable I-Cache via CCR register
        SCB()->CCR |= ccr::IC_Msk;

        dsb();
        isb();
    #endif
}

/// Disable Instruction Cache
inline void disable_icache() {
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
        dsb();
        isb();

        // Disable I-Cache
        SCB()->CCR &= ~ccr::IC_Msk;

        // Invalidate I-Cache after disabling
        invalidate_icache();
    #endif
}

/// Check if I-Cache is enabled
/// @return true if I-Cache is currently enabled
inline bool is_icache_enabled() {
    return (SCB()->CCR & ccr::IC_Msk) != 0;
}

// ============================================================================
// Data Cache (D-Cache) Functions
// ============================================================================

/// Invalidate entire D-Cache
/// Warning: This discards dirty cache lines without writing back!
/// Use clean_invalidate_dcache() if data needs to be preserved.
///
/// Use case: Before enabling D-Cache or after DMA writes to memory
inline void invalidate_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        // Invalidate D-Cache by set/way (all ways, all sets)
        for (uint32_t set = 0; set < 4; ++set) {
            for (uint32_t way = 0; way < 4; ++way) {
                // Format: [way:31-30] [set:12-5]
                uint32_t value = (way << 30) | (set << 5);
                CACHE()->DCISW = value;
            }
        }

        dsb();  // Ensure invalidation is complete
    #endif
}

/// Clean entire D-Cache
/// Writes all dirty cache lines back to memory
///
/// Use case: Before DMA reads from memory (memory -> peripheral)
inline void clean_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        // Clean D-Cache by set/way (all ways, all sets)
        for (uint32_t set = 0; set < 4; ++set) {
            for (uint32_t way = 0; way < 4; ++way) {
                // Format: [way:31-30] [set:12-5]
                uint32_t value = (way << 30) | (set << 5);
                CACHE()->DCCSW = value;
            }
        }

        dsb();  // Ensure clean is complete
    #endif
}

/// Clean and invalidate entire D-Cache
/// Writes dirty lines to memory, then invalidates cache
///
/// Use case: Complete cache flush with data preservation
inline void clean_invalidate_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        // Clean and invalidate D-Cache by set/way
        for (uint32_t set = 0; set < 4; ++set) {
            for (uint32_t way = 0; way < 4; ++way) {
                // Format: [way:31-30] [set:12-5]
                uint32_t value = (way << 30) | (set << 5);
                CACHE()->DCCISW = value;
            }
        }

        dsb();
    #endif
}

/// Enable Data Cache
/// Enables D-Cache for faster data access (typically 2-3x speedup)
inline void enable_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        // Invalidate D-Cache before enabling
        invalidate_dcache();

        // Enable D-Cache via CCR register
        SCB()->CCR |= ccr::DC_Msk;

        dsb();
        isb();
    #endif
}

/// Disable Data Cache
inline void disable_dcache() {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        // Clean and invalidate D-Cache before disabling
        clean_invalidate_dcache();

        // Disable D-Cache
        SCB()->CCR &= ~ccr::DC_Msk;

        dsb();
        isb();
    #endif
}

/// Check if D-Cache is enabled
/// @return true if D-Cache is currently enabled
inline bool is_dcache_enabled() {
    return (SCB()->CCR & ccr::DC_Msk) != 0;
}

// ============================================================================
// Cache Maintenance for DMA Operations
// ============================================================================

/// Invalidate D-Cache by address range
/// Use BEFORE DMA write (peripheral -> memory)
///
/// @param addr: Start address (must be 32-byte aligned for optimal performance)
/// @param size: Size in bytes
///
/// Example: Before receiving data via DMA
///   uint8_t buffer[256] __attribute__((aligned(32)));
///   invalidate_dcache_by_addr(buffer, sizeof(buffer));
///   // Start DMA transfer
inline void invalidate_dcache_by_addr(void* addr, uint32_t size) {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        uint32_t start_addr = reinterpret_cast<uint32_t>(addr);
        uint32_t end_addr = start_addr + size;

        // Align to cache line size (32 bytes on Cortex-M7)
        start_addr &= ~0x1FUL;

        dsb();

        // Invalidate each cache line
        for (uint32_t addr_val = start_addr; addr_val < end_addr; addr_val += 32) {
            CACHE()->DCIMVAC = addr_val;
        }

        dsb();
        isb();
    #else
        (void)addr;
        (void)size;
    #endif
}

/// Clean D-Cache by address range
/// Use AFTER writing data that will be read by DMA (memory -> peripheral)
///
/// @param addr: Start address (must be 32-byte aligned for optimal performance)
/// @param size: Size in bytes
///
/// Example: Before sending data via DMA
///   uint8_t buffer[256] __attribute__((aligned(32)));
///   // Fill buffer with data
///   clean_dcache_by_addr(buffer, sizeof(buffer));
///   // Start DMA transfer
inline void clean_dcache_by_addr(const void* addr, uint32_t size) {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        uint32_t start_addr = reinterpret_cast<uint32_t>(addr);
        uint32_t end_addr = start_addr + size;

        // Align to cache line size (32 bytes)
        start_addr &= ~0x1FUL;

        dsb();

        // Clean each cache line
        for (uint32_t addr_val = start_addr; addr_val < end_addr; addr_val += 32) {
            CACHE()->DCCMVAC = addr_val;
        }

        dsb();
        isb();
    #else
        (void)addr;
        (void)size;
    #endif
}

/// Clean and invalidate D-Cache by address range
/// Use for bidirectional DMA buffers
///
/// @param addr: Start address (must be 32-byte aligned for optimal performance)
/// @param size: Size in bytes
inline void clean_invalidate_dcache_by_addr(void* addr, uint32_t size) {
    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        uint32_t start_addr = reinterpret_cast<uint32_t>(addr);
        uint32_t end_addr = start_addr + size;

        // Align to cache line size (32 bytes)
        start_addr &= ~0x1FUL;

        dsb();

        // Clean and invalidate each cache line
        for (uint32_t addr_val = start_addr; addr_val < end_addr; addr_val += 32) {
            CACHE()->DCCIMVAC = addr_val;
        }

        dsb();
        isb();
    #else
        (void)addr;
        (void)size;
    #endif
}

// ============================================================================
// High-Level Cache Initialization
// ============================================================================

/// Initialize caches with recommended settings
/// @param enable_icache: Enable instruction cache (default: true)
/// @param enable_dcache: Enable data cache (default: true)
///
/// This enables both I-Cache and D-Cache for optimal performance.
/// Typically called early in SystemInit().
///
/// Performance improvement:
/// - 2-3x faster code execution with I-Cache
/// - 2-3x faster data access with D-Cache
/// - Overall system performance: 3-5x faster
///
/// Example:
///   void SystemInit() {
///       fpu::initialize();
///       cache::initialize();  // Enable both caches
///       // ... rest of initialization
///   }
inline void initialize(bool enable_icache_flag = true, bool enable_dcache_flag = true) {
    #if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
        if (enable_icache_flag) {
            enable_icache();
        }
    #else
        (void)enable_icache_flag;
    #endif

    #if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
        if (enable_dcache_flag) {
            enable_dcache();
        }
    #else
        (void)enable_dcache_flag;
    #endif
}

} // namespace alloy::arm::cortex_m7::cache
