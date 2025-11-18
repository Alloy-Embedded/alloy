# Spec: ARM Cortex-M Core Common

## Overview
Common functionality available on **ALL** ARM Cortex-M cores (M0/M0+/M1/M3/M4/M7/M23/M33/M55).
This layer provides abstractions for NVIC, SysTick, SCB, and other universal ARM peripherals.

## Files

### `src/startup/arm_cortex_m/core_common.hpp`
**Purpose**: Define ARM Cortex-M core registers and basic initialization

```cpp
#pragma once

#include "core/types.hpp"
#include <cstdint>

namespace alloy::startup::arm {

using namespace alloy::core;

//=============================================================================
// System Control Block (SCB) - Available on ALL Cortex-M
//=============================================================================

struct SCB_Registers {
    volatile u32 CPUID;      // 0xE000ED00 - CPU ID Base Register
    volatile u32 ICSR;       // 0xE000ED04 - Interrupt Control and State
    volatile u32 VTOR;       // 0xE000ED08 - Vector Table Offset
    volatile u32 AIRCR;      // 0xE000ED0C - Application Interrupt and Reset Control
    volatile u32 SCR;        // 0xE000ED10 - System Control Register
    volatile u32 CCR;        // 0xE000ED14 - Configuration and Control
    volatile u32 SHPR[3];    // 0xE000ED18-24 - System Handler Priority
    volatile u32 SHCSR;      // 0xE000ED24 - System Handler Control and State
    volatile u32 CFSR;       // 0xE000ED28 - Configurable Fault Status
    volatile u32 HFSR;       // 0xE000ED2C - HardFault Status
    volatile u32 DFSR;       // 0xE000ED30 - Debug Fault Status
    volatile u32 MMFAR;      // 0xE000ED34 - MemManage Fault Address
    volatile u32 BFAR;       // 0xE000ED38 - BusFault Address
    volatile u32 AFSR;       // 0xE000ED3C - Auxiliary Fault Status
    volatile u32 PFR[2];     // 0xE000ED40-44 - Processor Feature
    volatile u32 DFR;        // 0xE000ED48 - Debug Feature
    volatile u32 ADR;        // 0xE000ED4C - Auxiliary Feature
    volatile u32 MMFR[4];    // 0xE000ED50-5C - Memory Model Feature
    volatile u32 ISAR[5];    // 0xE000ED60-70 - Instruction Set Attributes
    volatile u32 RESERVED[5];
    volatile u32 CPACR;      // 0xE000ED88 - Coprocessor Access Control (M3+)
};

inline SCB_Registers* const SCB = reinterpret_cast<SCB_Registers*>(0xE000ED00);

// SCB CPACR bits (for FPU enablement)
namespace scb_cpacr {
    constexpr u32 CP10_FULL_ACCESS = (3U << 20);  // Full access to CP10 (FPU)
    constexpr u32 CP11_FULL_ACCESS = (3U << 22);  // Full access to CP11 (FPU)
    constexpr u32 FPU_FULL_ACCESS = CP10_FULL_ACCESS | CP11_FULL_ACCESS;
}

//=============================================================================
// Nested Vectored Interrupt Controller (NVIC) - Available on ALL Cortex-M
//=============================================================================

struct NVIC_Registers {
    volatile u32 ISER[8];    // 0xE000E100-11C - Interrupt Set-Enable
    volatile u32 RESERVED0[24];
    volatile u32 ICER[8];    // 0xE000E180-19C - Interrupt Clear-Enable
    volatile u32 RESERVED1[24];
    volatile u32 ISPR[8];    // 0xE000E200-21C - Interrupt Set-Pending
    volatile u32 RESERVED2[24];
    volatile u32 ICPR[8];    // 0xE000E280-29C - Interrupt Clear-Pending
    volatile u32 RESERVED3[24];
    volatile u32 IABR[8];    // 0xE000E300-31C - Interrupt Active Bit
    volatile u32 RESERVED4[56];
    volatile u8  IPR[240];   // 0xE000E400-4EF - Interrupt Priority
};

inline NVIC_Registers* const NVIC = reinterpret_cast<NVIC_Registers*>(0xE000E100);

//=============================================================================
// SysTick Timer - Available on ALL Cortex-M
//=============================================================================

struct SysTick_Registers {
    volatile u32 CTRL;       // 0xE000E010 - Control and Status
    volatile u32 LOAD;       // 0xE000E014 - Reload Value
    volatile u32 VAL;        // 0xE000E018 - Current Value
    volatile u32 CALIB;      // 0xE000E01C - Calibration Value
};

inline SysTick_Registers* const SysTick = reinterpret_cast<SysTick_Registers*>(0xE000E010);

// SysTick CTRL bits
namespace systick_ctrl {
    constexpr u32 ENABLE = (1U << 0);      // Counter enable
    constexpr u32 TICKINT = (1U << 1);     // Enable interrupt
    constexpr u32 CLKSOURCE = (1U << 2);   // Clock source (1=processor, 0=external)
    constexpr u32 COUNTFLAG = (1U << 16);  // Count flag
}

//=============================================================================
// Core Debug - Available on ALL Cortex-M (if debug present)
//=============================================================================

struct CoreDebug_Registers {
    volatile u32 DHCSR;      // 0xE000EDF0 - Debug Halting Control and Status
    volatile u32 DCRSR;      // 0xE000EDF4 - Debug Core Register Selector
    volatile u32 DCRDR;      // 0xE000EDF8 - Debug Core Register Data
    volatile u32 DEMCR;      // 0xE000EDFC - Debug Exception and Monitor Control
};

inline CoreDebug_Registers* const CoreDebug = reinterpret_cast<CoreDebug_Registers*>(0xE000EDF0);

//=============================================================================
// Common Functions - Available on ALL Cortex-M
//=============================================================================

/**
 * Data Synchronization Barrier
 * Ensures all memory accesses complete before proceeding
 */
inline void dsb() {
    __asm__ volatile ("dsb" ::: "memory");
}

/**
 * Instruction Synchronization Barrier
 * Flushes pipeline to ensure instruction fetch uses new settings
 */
inline void isb() {
    __asm__ volatile ("isb" ::: "memory");
}

/**
 * Data Memory Barrier
 * Ensures ordering of data accesses
 */
inline void dmb() {
    __asm__ volatile ("dmb" ::: "memory");
}

/**
 * No Operation
 */
inline void nop() {
    __asm__ volatile ("nop");
}

/**
 * Wait For Interrupt
 * Puts processor in low-power state until interrupt
 */
inline void wfi() {
    __asm__ volatile ("wfi");
}

/**
 * Wait For Event
 * Puts processor in low-power state until event
 */
inline void wfe() {
    __asm__ volatile ("wfe");
}

/**
 * Send Event
 * Wakes up processors waiting on WFE
 */
inline void sev() {
    __asm__ volatile ("sev");
}

} // namespace alloy::startup::arm
```

---

### `src/startup/arm_cortex_m/nvic.hpp`
**Purpose**: NVIC (Nested Vectored Interrupt Controller) configuration

```cpp
#pragma once

#include "startup/arm_cortex_m/core_common.hpp"
#include "core/types.hpp"

namespace alloy::startup::arm::nvic {

using namespace alloy::core;

/**
 * Enable an interrupt
 * @param irq_number Interrupt number (0-239)
 */
inline void enable_irq(u8 irq_number) {
    u32 reg = irq_number / 32;
    u32 bit = irq_number % 32;
    NVIC->ISER[reg] = (1U << bit);
}

/**
 * Disable an interrupt
 * @param irq_number Interrupt number (0-239)
 */
inline void disable_irq(u8 irq_number) {
    u32 reg = irq_number / 32;
    u32 bit = irq_number % 32;
    NVIC->ICER[reg] = (1U << bit);
}

/**
 * Set interrupt priority
 * @param irq_number Interrupt number (0-239)
 * @param priority Priority value (0-255, lower = higher priority)
 */
inline void set_priority(u8 irq_number, u8 priority) {
    NVIC->IPR[irq_number] = priority;
}

/**
 * Get interrupt priority
 * @param irq_number Interrupt number (0-239)
 * @return Priority value (0-255)
 */
inline u8 get_priority(u8 irq_number) {
    return NVIC->IPR[irq_number];
}

/**
 * Set interrupt pending
 * @param irq_number Interrupt number (0-239)
 */
inline void set_pending(u8 irq_number) {
    u32 reg = irq_number / 32;
    u32 bit = irq_number % 32;
    NVIC->ISPR[reg] = (1U << bit);
}

/**
 * Clear interrupt pending
 * @param irq_number Interrupt number (0-239)
 */
inline void clear_pending(u8 irq_number) {
    u32 reg = irq_number / 32;
    u32 bit = irq_number % 32;
    NVIC->ICPR[reg] = (1U << bit);
}

/**
 * Check if interrupt is active
 * @param irq_number Interrupt number (0-239)
 * @return true if active
 */
inline bool is_active(u8 irq_number) {
    u32 reg = irq_number / 32;
    u32 bit = irq_number % 32;
    return (NVIC->IABR[reg] & (1U << bit)) != 0;
}

} // namespace alloy::startup::arm::nvic
```

---

### `src/startup/arm_cortex_m/systick.hpp`
**Purpose**: SysTick timer configuration

```cpp
#pragma once

#include "startup/arm_cortex_m/core_common.hpp"
#include "core/types.hpp"
#include "core/error.hpp"

namespace alloy::startup::arm::systick {

using namespace alloy::core;

/**
 * Configure SysTick timer
 * @param ticks Number of ticks between interrupts (1-16777215)
 * @param enable_interrupt Enable SysTick interrupt
 * @return Ok on success, error on invalid ticks
 */
inline Result<void> configure(u32 ticks, bool enable_interrupt = true) {
    if (ticks == 0 || ticks > 0x00FFFFFF) {
        return Error(ErrorCode::InvalidParameter);
    }

    // Disable SysTick
    SysTick->CTRL = 0;

    // Set reload value
    SysTick->LOAD = ticks - 1;

    // Clear current value
    SysTick->VAL = 0;

    // Configure and enable
    u32 ctrl = systick_ctrl::ENABLE | systick_ctrl::CLKSOURCE;
    if (enable_interrupt) {
        ctrl |= systick_ctrl::TICKINT;
    }
    SysTick->CTRL = ctrl;

    return Ok();
}

/**
 * Disable SysTick timer
 */
inline void disable() {
    SysTick->CTRL = 0;
}

/**
 * Get current SysTick value
 * @return Current counter value (counts down from reload)
 */
inline u32 get_value() {
    return SysTick->VAL;
}

/**
 * Check if SysTick has counted to 0
 * @return true if counted to 0 since last read
 */
inline bool has_counted_to_zero() {
    return (SysTick->CTRL & systick_ctrl::COUNTFLAG) != 0;
}

} // namespace alloy::startup::arm::systick
```

## Usage Example

```cpp
// In board SystemInit() function
#include "startup/arm_cortex_m/core_common.hpp"
#include "startup/arm_cortex_m/nvic.hpp"
#include "startup/arm_cortex_m/systick.hpp"

using namespace alloy::startup::arm;

// Configure SysTick for 1ms interrupt at 72MHz
systick::configure(72000, true);  // 72000 ticks = 1ms

// Enable specific interrupt
nvic::enable_irq(27);  // USART1 on STM32F1
nvic::set_priority(27, 0x80);  // Medium priority
```

## Testing
- ✅ Compile on all Cortex-M variants (M0/M3/M4/M7/M33)
- ✅ Verify register addresses match ARM specification
- ✅ Test NVIC enable/disable/priority
- ✅ Test SysTick configuration with actual timing

## Dependencies
- `core/types.hpp` - For u8, u32, etc
- `core/error.hpp` - For Result<T>

## References
- ARM Cortex-M3 Technical Reference Manual
- ARM Cortex-M4 Technical Reference Manual
- ARM Cortex-M7 Technical Reference Manual
