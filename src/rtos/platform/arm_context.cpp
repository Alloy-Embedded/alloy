/// ARM Cortex-M Context Switching Implementation

#include "rtos/platform/arm_context.hpp"

#include "rtos/scheduler.hpp"

namespace alloy::rtos {

void init_task_stack(TaskControlBlock* tcb, void (*func)()) {
    // Get pointer to top of stack (stacks grow downward)
    core::u32* sp =
        reinterpret_cast<core::u32*>(static_cast<core::u8*>(tcb->stack_base) + tcb->stack_size);

    // Initialize stack as if exception occurred
    // Hardware-pushed registers (exception frame)
    *(--sp) = 0x01000000;                         // xPSR (Thumb bit set)
    *(--sp) = reinterpret_cast<core::u32>(func);  // PC (task entry point)
    *(--sp) = 0;                                  // LR (link register)
    *(--sp) = 0;                                  // R12
    *(--sp) = 0;                                  // R3
    *(--sp) = 0;                                  // R2
    *(--sp) = 0;                                  // R1
    *(--sp) = 0;                                  // R0

    // Software-pushed registers (saved by PendSV)
    *(--sp) = 0;  // R11
    *(--sp) = 0;  // R10
    *(--sp) = 0;  // R9
    *(--sp) = 0;  // R8
    *(--sp) = 0;  // R7
    *(--sp) = 0;  // R6
    *(--sp) = 0;  // R5
    *(--sp) = 0;  // R4

    // Save stack pointer in TCB
    tcb->stack_pointer = sp;

#ifdef DEBUG
    // Place stack canary at bottom for overflow detection
    constexpr core::u32 STACK_CANARY = 0xDEADBEEF;
    *reinterpret_cast<core::u32*>(tcb->stack_base) = STACK_CANARY;
#endif
}

void trigger_context_switch() {
    // Set PendSV pending bit
    using namespace arm_regs;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

    // ISB ensures PendSV is triggered before continuing
    __asm volatile("isb");
}

[[noreturn]] void start_first_task() {
    // Disable interrupts during setup
    __asm volatile("cpsid i");

    // Get first task's stack pointer
    void* sp = g_scheduler.current_task->stack_pointer;

    // Set PSP to task's stack pointer
    __asm volatile("msr psp, %0    \n"  // Set PSP to task stack
                   :
                   : "r"(sp));

    // Set CONTROL register to use PSP and unprivileged mode
    __asm volatile("mov r0, #2     \n"  // CONTROL = 2 (use PSP, unprivileged)
                   "msr control, r0\n"
                   "isb            \n"  // Ensure change takes effect
    );

    // Pop registers from stack
    __asm volatile("pop {r4-r11}   \n"  // Restore r4-r11
                   "pop {r0-r3}    \n"  // Restore r0-r3
                   "pop {r12}      \n"  // Restore r12
                   "add sp, sp, #4 \n"  // Skip LR
                   "pop {lr}       \n"  // Restore PC to LR
                   "add sp, sp, #4 \n"  // Skip xPSR
    );

    // Enable interrupts
    __asm volatile("cpsie i");

    // Jump to task
    __asm volatile("bx lr          \n"  // Branch to task entry point
    );

    // Never reached
    while (1)
        ;
}

}  // namespace alloy::rtos

// PendSV Handler - Context Switch
//
// This is the heart of the RTOS. Called when PendSV is triggered.
// Saves current task context, loads next task context.
//
// IMPORTANT: Must be naked (no prologue/epilogue) and extern "C"
extern "C" __attribute__((naked)) void PendSV_Handler() {
    __asm volatile(
        // Save current task context
        "mrs r0, psp            \n"  // Get current task's stack pointer
        "stmdb r0!, {r4-r11}    \n"  // Push r4-r11 onto task stack

        // Call C function to save SP and get next task
        "bl PendSV_Handler_C    \n"

        // r0 now contains next task's stack pointer
        // Restore next task context
        "ldmia r0!, {r4-r11}    \n"  // Pop r4-r11 from next task stack
        "msr psp, r0            \n"  // Set PSP to next task's SP

        // Clear context switch flag and return
        "bx lr                  \n"  // Return from exception (hardware restores r0-r3, r12, lr, pc,
                                     // xPSR)
    );
}

// C portion of PendSV handler
//
// Saves current task SP, gets next task SP
// @param sp Current task's stack pointer (after saving r4-r11)
// @return Next task's stack pointer
extern "C" void* PendSV_Handler_C(void* sp) {
    // Save current task's stack pointer
    if (alloy::rtos::g_scheduler.current_task != nullptr) {
        alloy::rtos::g_scheduler.current_task->stack_pointer = sp;
    }

    // Get next task (already selected by scheduler)
    alloy::rtos::TaskControlBlock* next = alloy::rtos::g_scheduler.current_task;

    // Clear context switch flag
    alloy::rtos::g_scheduler.need_context_switch = false;

    // Return next task's stack pointer
    return next->stack_pointer;
}

// SVC Handler - Used to start first task
//
// Called when issuing SVC instruction to start scheduler
extern "C" __attribute__((naked)) void SVC_Handler() {
    // Not used in this implementation
    // We use direct assembly in start_first_task() instead
    __asm volatile("bx lr \n");
}
