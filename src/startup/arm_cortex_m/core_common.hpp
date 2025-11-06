// Alloy Framework - ARM Cortex-M Core Common Definitions
//
// Provides common register definitions and functions for all ARM Cortex-M cores
// (M0, M0+, M3, M4, M7, M23, M33, M55, etc.)
//
// This file contains:
// - System Control Block (SCB) registers
// - Nested Vectored Interrupt Controller (NVIC) registers
// - SysTick Timer registers
// - Memory barrier and instruction synchronization functions
// - Wait-for-event/interrupt functions
//
// All definitions are compatible with CMSIS but use modern C++ idioms

#pragma once

#include <cstdint>

namespace alloy::arm::cortex_m {

// ============================================================================
// System Control Block (SCB) Registers
// ============================================================================
// Base address: 0xE000ED00

struct SCB_Registers {
    volatile uint32_t CPUID;     // Offset: 0x000 (R/ )  CPUID Base Register
    volatile uint32_t ICSR;      // Offset: 0x004 (R/W)  Interrupt Control and State Register
    volatile uint32_t VTOR;      // Offset: 0x008 (R/W)  Vector Table Offset Register
    volatile uint32_t AIRCR;     // Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register
    volatile uint32_t SCR;       // Offset: 0x010 (R/W)  System Control Register
    volatile uint32_t CCR;       // Offset: 0x014 (R/W)  Configuration Control Register
    volatile uint8_t  SHP[12];   // Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15)
    volatile uint32_t SHCSR;     // Offset: 0x024 (R/W)  System Handler Control and State Register
    volatile uint32_t CFSR;      // Offset: 0x028 (R/W)  Configurable Fault Status Register
    volatile uint32_t HFSR;      // Offset: 0x02C (R/W)  HardFault Status Register
    volatile uint32_t DFSR;      // Offset: 0x030 (R/W)  Debug Fault Status Register
    volatile uint32_t MMFAR;     // Offset: 0x034 (R/W)  MemManage Fault Address Register
    volatile uint32_t BFAR;      // Offset: 0x038 (R/W)  BusFault Address Register
    volatile uint32_t AFSR;      // Offset: 0x03C (R/W)  Auxiliary Fault Status Register
    volatile uint32_t PFR[2];    // Offset: 0x040 (R/ )  Processor Feature Register
    volatile uint32_t DFR;       // Offset: 0x048 (R/ )  Debug Feature Register
    volatile uint32_t ADR;       // Offset: 0x04C (R/ )  Auxiliary Feature Register
    volatile uint32_t MMFR[4];   // Offset: 0x050 (R/ )  Memory Model Feature Register
    volatile uint32_t ISAR[5];   // Offset: 0x060 (R/ )  Instruction Set Attributes Register
    uint32_t RESERVED0[5];
    volatile uint32_t CPACR;     // Offset: 0x088 (R/W)  Coprocessor Access Control Register
};

constexpr SCB_Registers* SCB = reinterpret_cast<SCB_Registers*>(0xE000ED00);

// CPACR (Coprocessor Access Control Register) bit definitions
namespace cpacr {
    constexpr uint32_t CP10_Pos = 20;                              // CP10 coprocessor access bits position
    constexpr uint32_t CP10_Msk = (3UL << CP10_Pos);               // CP10 coprocessor access bits mask
    constexpr uint32_t CP11_Pos = 22;                              // CP11 coprocessor access bits position
    constexpr uint32_t CP11_Msk = (3UL << CP11_Pos);               // CP11 coprocessor access bits mask

    // Coprocessor access modes
    constexpr uint32_t Access_Denied      = 0UL;  // Access denied (reset value)
    constexpr uint32_t Access_Privileged  = 1UL;  // Privileged access only
    constexpr uint32_t Access_Reserved    = 2UL;  // Reserved
    constexpr uint32_t Access_Full        = 3UL;  // Full access
}

// CCR (Configuration Control Register) bit definitions
namespace ccr {
    constexpr uint32_t IC_Pos  = 17;              // Instruction cache enable bit position
    constexpr uint32_t IC_Msk  = (1UL << IC_Pos); // Instruction cache enable bit mask
    constexpr uint32_t DC_Pos  = 16;              // Data cache enable bit position
    constexpr uint32_t DC_Msk  = (1UL << DC_Pos); // Data cache enable bit mask
    constexpr uint32_t BP_Pos  = 18;              // Branch prediction enable bit position
    constexpr uint32_t BP_Msk  = (1UL << BP_Pos); // Branch prediction enable bit mask
}

// AIRCR (Application Interrupt and Reset Control Register) bit definitions
namespace aircr {
    constexpr uint32_t VECTKEY_Pos = 16;                     // VECTKEY position
    constexpr uint32_t VECTKEY     = (0x05FAUL << 16);       // VECTKEY value for writes
    constexpr uint32_t PRIGROUP_Pos = 8;                     // Priority grouping position
    constexpr uint32_t PRIGROUP_Msk = (7UL << PRIGROUP_Pos); // Priority grouping mask
}

// ============================================================================
// Cache Maintenance Operations (M7 only)
// ============================================================================
// Base address: 0xE000EF50

struct Cache_Registers {
    volatile uint32_t ICIALLU;   // Offset: 0x000 ( /W)  I-Cache Invalidate All to PoU
    uint32_t RESERVED0;
    volatile uint32_t ICIMVAU;   // Offset: 0x008 ( /W)  I-Cache Invalidate by MVA to PoU
    volatile uint32_t DCIMVAC;   // Offset: 0x00C ( /W)  D-Cache Invalidate by MVA to PoC
    volatile uint32_t DCISW;     // Offset: 0x010 ( /W)  D-Cache Invalidate by Set-way
    volatile uint32_t DCCMVAU;   // Offset: 0x014 ( /W)  D-Cache Clean by MVA to PoU
    volatile uint32_t DCCMVAC;   // Offset: 0x018 ( /W)  D-Cache Clean by MVA to PoC
    volatile uint32_t DCCSW;     // Offset: 0x01C ( /W)  D-Cache Clean by Set-way
    volatile uint32_t DCCIMVAC;  // Offset: 0x020 ( /W)  D-Cache Clean and Invalidate by MVA to PoC
    volatile uint32_t DCCISW;    // Offset: 0x024 ( /W)  D-Cache Clean and Invalidate by Set-way
};

constexpr Cache_Registers* CACHE = reinterpret_cast<Cache_Registers*>(0xE000EF50);

// ============================================================================
// Nested Vectored Interrupt Controller (NVIC) Registers
// ============================================================================
// Base address: 0xE000E100

struct NVIC_Registers {
    volatile uint32_t ISER[8];   // Offset: 0x000 (R/W)  Interrupt Set Enable Register
    uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];   // Offset: 0x080 (R/W)  Interrupt Clear Enable Register
    uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];   // Offset: 0x100 (R/W)  Interrupt Set Pending Register
    uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];   // Offset: 0x180 (R/W)  Interrupt Clear Pending Register
    uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];   // Offset: 0x200 (R/ )  Interrupt Active Bit Register
    uint32_t RESERVED4[56];
    volatile uint8_t  IP[240];   // Offset: 0x300 (R/W)  Interrupt Priority Register (8Bit wide)
    uint32_t RESERVED5[644];
    volatile uint32_t STIR;      // Offset: 0xE00 ( /W)  Software Trigger Interrupt Register
};

constexpr NVIC_Registers* NVIC = reinterpret_cast<NVIC_Registers*>(0xE000E100);

// ============================================================================
// SysTick Timer Registers
// ============================================================================
// Base address: 0xE000E010

struct SysTick_Registers {
    volatile uint32_t CTRL;      // Offset: 0x000 (R/W)  SysTick Control and Status Register
    volatile uint32_t LOAD;      // Offset: 0x004 (R/W)  SysTick Reload Value Register
    volatile uint32_t VAL;       // Offset: 0x008 (R/W)  SysTick Current Value Register
    volatile uint32_t CALIB;     // Offset: 0x00C (R/ )  SysTick Calibration Register
};

constexpr SysTick_Registers* SYSTICK = reinterpret_cast<SysTick_Registers*>(0xE000E010);

// SysTick CTRL bit definitions
namespace systick_ctrl {
    constexpr uint32_t ENABLE    = (1UL << 0);   // Counter enable
    constexpr uint32_t TICKINT   = (1UL << 1);   // Enable SysTick exception
    constexpr uint32_t CLKSOURCE = (1UL << 2);   // Clock source selection (0=external, 1=processor)
    constexpr uint32_t COUNTFLAG = (1UL << 16);  // Counter flag
}

// ============================================================================
// Memory Barrier and Synchronization Functions
// ============================================================================

/// Data Memory Barrier
/// Ensures that all explicit memory accesses that appear in program order
/// before the DMB instruction are observed before any explicit memory accesses
/// that appear in program order after the DMB instruction
[[gnu::always_inline]] inline void dmb() {
    __asm__ volatile ("dmb" ::: "memory");
}

/// Data Synchronization Barrier
/// Ensures that all explicit memory accesses that appear in program order
/// before the DSB instruction are observed before any instruction that appears
/// in program order after the DSB instruction
[[gnu::always_inline]] inline void dsb() {
    __asm__ volatile ("dsb" ::: "memory");
}

/// Instruction Synchronization Barrier
/// Flushes the pipeline and ensures that all instructions following the ISB
/// are fetched from cache or memory, after the ISB has been completed
[[gnu::always_inline]] inline void isb() {
    __asm__ volatile ("isb" ::: "memory");
}

/// No Operation
/// Does nothing, takes one cycle
[[gnu::always_inline]] inline void nop() {
    __asm__ volatile ("nop");
}

/// Wait For Interrupt
/// Suspends execution until one of the following events occurs:
/// - An IRQ interrupt
/// - An FIQ interrupt
/// - A Debug Entry request
[[gnu::always_inline]] inline void wfi() {
    __asm__ volatile ("wfi");
}

/// Wait For Event
/// Suspends execution until one of the following events occurs:
/// - An event signaled by another processor using SEV instruction
/// - An interrupt (even if disabled)
/// - A debug event
[[gnu::always_inline]] inline void wfe() {
    __asm__ volatile ("wfe");
}

/// Send Event
/// Causes an event to be signaled to all processors in a multiprocessor system
[[gnu::always_inline]] inline void sev() {
    __asm__ volatile ("sev");
}

/// Enable IRQ interrupts
/// Clears PRIMASK bit (enables interrupts with configurable priority)
[[gnu::always_inline]] inline void enable_irq() {
    __asm__ volatile ("cpsie i" ::: "memory");
}

/// Disable IRQ interrupts
/// Sets PRIMASK bit (disables interrupts with configurable priority)
[[gnu::always_inline]] inline void disable_irq() {
    __asm__ volatile ("cpsid i" ::: "memory");
}

/// Enable fault exceptions (M3/M4/M7 only)
/// Clears FAULTMASK bit
[[gnu::always_inline]] inline void enable_fault_irq() {
    __asm__ volatile ("cpsie f" ::: "memory");
}

/// Disable fault exceptions (M3/M4/M7 only)
/// Sets FAULTMASK bit (disables all exceptions except NMI and HardFault)
[[gnu::always_inline]] inline void disable_fault_irq() {
    __asm__ volatile ("cpsid f" ::: "memory");
}

/// Get PRIMASK value
/// Returns current PRIMASK value
[[gnu::always_inline]] inline uint32_t get_primask() {
    uint32_t result;
    __asm__ volatile ("MRS %0, primask" : "=r" (result));
    return result;
}

/// Set PRIMASK value
/// Sets PRIMASK to specified value
[[gnu::always_inline]] inline void set_primask(uint32_t priMask) {
    __asm__ volatile ("MSR primask, %0" :: "r" (priMask) : "memory");
}

// ============================================================================
// System Control Functions
// ============================================================================

/// System Reset
/// Triggers a system reset
[[gnu::always_inline, noreturn]] inline void system_reset() {
    dsb();  // Ensure all outstanding memory accesses included buffered writes are completed

    SCB->AIRCR = aircr::VECTKEY | (SCB->AIRCR & aircr::PRIGROUP_Msk) | (1UL << 2);  // SYSRESETREQ

    dsb();  // Ensure completion of memory access

    while(true) {
        nop();  // Wait for reset
    }
}

} // namespace alloy::arm::cortex_m
