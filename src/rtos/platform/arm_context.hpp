/// ARM Cortex-M Context Switching
///
/// Implements context switching for ARM Cortex-M processors using PendSV.
///
/// Theory:
/// - ARM Cortex-M has hardware support for RTOS context switching
/// - Uses PendSV exception (lowest priority) for context switches
/// - Hardware automatically saves/restores r0-r3, r12, lr, pc, xPSR
/// - Software saves/restores r4-r11
///
/// Context switch time: ~5-10µs @ 72MHz
///
/// Stack layout (after exception entry):
/// ```
/// High memory
/// ┌─────────────┐
/// │    xPSR     │ ← Hardware pushed
/// │     PC      │
/// │     LR      │
/// │     R12     │
/// │     R3      │
/// │     R2      │
/// │     R1      │
/// │     R0      │
/// ├─────────────┤
/// │     R11     │ ← Software pushed
/// │     R10     │
/// │     R9      │
/// │     R8      │
/// │     R7      │
/// │     R6      │
/// │     R5      │
/// │     R4      │ ← SP points here
/// └─────────────┘
/// Low memory
/// ```

#ifndef ALLOY_RTOS_ARM_CONTEXT_HPP
#define ALLOY_RTOS_ARM_CONTEXT_HPP

#include "rtos/rtos.hpp"

#include "core/types.hpp"

namespace alloy::rtos {

/// Initialize task stack for ARM Cortex-M
///
/// Sets up initial stack frame as if the task was interrupted.
/// When first scheduled, PendSV will restore this context.
///
/// @param tcb Task control block
/// @param func Task entry point
void init_task_stack(TaskControlBlock* tcb, void (*func)());

/// Start first task (never returns)
///
/// Loads context of first task and starts executing it.
[[noreturn]] void start_first_task();

/// Trigger a context switch
///
/// Sets PendSV pending bit to request a context switch.
/// The actual switch happens in PendSV_Handler.
void trigger_context_switch();

/// PendSV Handler - performs context switch
///
/// This is called automatically by the CPU when PendSV is triggered.
/// Must be declared as extern "C" and naked (no prologue/epilogue).
extern "C" void PendSV_Handler();

/// SVC Handler - used for starting first task
///
/// This is called when we issue SVC instruction to start scheduler.
extern "C" void SVC_Handler();

// Platform-specific register definitions
namespace arm_regs {

/// System Control Block (SCB) registers
struct SCB_Type {
    volatile core::u32 CPUID;  // Offset: 0x000 (R/ )  CPUID Base Register
    volatile core::u32 ICSR;   // Offset: 0x004 (R/W)  Interrupt Control and State Register
    volatile core::u32 VTOR;   // Offset: 0x008 (R/W)  Vector Table Offset Register
    volatile core::u32
        AIRCR;  // Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register
    volatile core::u32 SCR;     // Offset: 0x010 (R/W)  System Control Register
    volatile core::u32 CCR;     // Offset: 0x014 (R/W)  Configuration Control Register
    volatile core::u8 SHP[12];  // Offset: 0x018 (R/W)  System Handlers Priority Registers
    volatile core::u32 SHCSR;   // Offset: 0x024 (R/W)  System Handler Control and State Register
};

// SCB base address (ARM Cortex-M standard)
constexpr core::u32 SCB_BASE = 0xE000ED00UL;
inline SCB_Type* const SCB = reinterpret_cast<SCB_Type*>(SCB_BASE);

// ICSR register bits
constexpr core::u32 SCB_ICSR_PENDSVSET_Pos = 28;
constexpr core::u32 SCB_ICSR_PENDSVSET_Msk = (1UL << SCB_ICSR_PENDSVSET_Pos);

}  // namespace arm_regs

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_ARM_CONTEXT_HPP
