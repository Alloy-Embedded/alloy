/// ARM Cortex-M0/M0+ Context Switching Implementation
///
/// Implements context switching for ARM Cortex-M0 and M0+ cores.
/// These cores have a limited instruction set compared to M3/M4:
/// - No STMDB/LDMIA for high registers (r8-r11)
/// - Must use MOV to transfer high registers to low registers first
///
/// Used by:
/// - Raspberry Pi Pico (RP2040 - dual M0+)
/// - Arduino Zero (SAMD21 - M0+)
///
/// Context switch time: ~8-12µs @ 125MHz (slightly slower than M3/M4)

#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_8M_BASE__)

#include "rtos/platform/arm_context.hpp"
#include "rtos/scheduler.hpp"

namespace alloy::rtos {

void init_task_stack(TaskControlBlock* tcb, void (*func)()) {
    // Get pointer to top of stack (stacks grow downward)
    core::u32* sp = reinterpret_cast<core::u32*>(
        static_cast<core::u8*>(tcb->stack_base) + tcb->stack_size
    );

    // Align to 8 bytes (AAPCS requirement)
    sp = reinterpret_cast<core::u32*>(
        reinterpret_cast<core::u32>(sp) & ~0x7
    );

    // Initialize stack frame as if task was interrupted
    // Hardware-pushed exception frame (done automatically by processor)
    *(--sp) = 0x01000000;  // xPSR (Thumb bit set)
    *(--sp) = reinterpret_cast<core::u32>(func);  // PC (task entry point)
    *(--sp) = 0xFFFFFFFD;  // LR (return to thread mode, use PSP)
    *(--sp) = 0x12121212;  // r12
    *(--sp) = 0x03030303;  // r3
    *(--sp) = 0x02020202;  // r2
    *(--sp) = 0x01010101;  // r1
    *(--sp) = 0x00000000;  // r0 (first argument)

    // Software-pushed registers (we need to save these manually)
    *(--sp) = 0x11111111;  // r11
    *(--sp) = 0x10101010;  // r10
    *(--sp) = 0x09090909;  // r9
    *(--sp) = 0x08080808;  // r8
    *(--sp) = 0x07070707;  // r7
    *(--sp) = 0x06060606;  // r6
    *(--sp) = 0x05050505;  // r5
    *(--sp) = 0x04040404;  // r4

    // Save initialized stack pointer in TCB
    tcb->stack_pointer = sp;

#ifdef DEBUG
    // Place stack canary at bottom for overflow detection
    constexpr core::u32 STACK_CANARY = 0xDEADBEEF;
    *reinterpret_cast<core::u32*>(tcb->stack_base) = STACK_CANARY;
#endif
}

namespace scheduler {

void trigger_context_switch() {
    // Trigger PendSV exception (same as M3/M4)
    // Set PendSV pending bit in ICSR
    using namespace arm_regs;
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

    // ISB ensures PendSV is triggered before continuing
    __asm volatile("isb");
}

[[noreturn]] void start_first_task() {
    // Disable interrupts during setup
    __asm volatile("cpsid i");

    // Get first task's stack pointer
    core::u32* sp = static_cast<core::u32*>(g_scheduler.current_task->stack_pointer);

    // Manually restore r4-r11 from stack
    core::u32 r4  = sp[0];
    core::u32 r5  = sp[1];
    core::u32 r6  = sp[2];
    core::u32 r7  = sp[3];
    core::u32 r8  = sp[4];
    core::u32 r9  = sp[5];
    core::u32 r10 = sp[6];
    core::u32 r11 = sp[7];

    // Update SP to point past software-saved registers
    sp += 8;

    // Set PSP to updated stack pointer
    __asm volatile(
        "msr psp, %0    \n"
        :
        : "r" (sp)
    );

    // Set CONTROL register to use PSP
    __asm volatile(
        "movs r0, #2    \n"
        "msr control, r0\n"
        "isb            \n"
        ::: "r0"
    );

    // Restore r4-r7 and r8-r11
    __asm volatile(
        "mov r4, %0     \n"
        "mov r5, %1     \n"
        "mov r6, %2     \n"
        "mov r7, %3     \n"
        :
        : "r" (r4), "r" (r5), "r" (r6), "r" (r7)
    );

    __asm volatile(
        "mov r8, %0     \n"
        "mov r9, %1     \n"
        "mov r10, %2    \n"
        "mov r11, %3    \n"
        :
        : "r" (r8), "r" (r9), "r" (r10), "r" (r11)
    );

    // Enable interrupts
    __asm volatile("cpsie i");

    // Exception return
    __asm volatile(
        "ldr r0, =0xFFFFFFFD\n"
        "bx r0              \n"
        ::: "r0"
    );

    // Never reached
    while (1);
}

} // namespace scheduler

} // namespace alloy::rtos

// PendSV Handler - Called when context switch is needed
// This must be naked function (no prologue/epilogue)
extern "C" __attribute__((naked)) void PendSV_Handler() {
    // Save context of current task
    __asm volatile(
        // Get current task's PSP
        "mrs r0, psp            \n"

        // Move r8-r11 to r1-r4 temporarily
        "mov r1, r8             \n"
        "mov r2, r9             \n"
        "mov r3, r10            \n"
        "mov r4, r11            \n"

        // Allocate space on PSP stack for r4-r11 (8 registers = 32 bytes)
        "movs r5, #32           \n"  // Load 32 into r5
        "sub r0, r5             \n"  // r0 = r0 - 32

        // Store r4-r7 at PSP
        "stmia r0!, {r4-r7}     \n"

        // Store r8-r11 (via r1-r4) at PSP
        "stmia r0!, {r1-r4}     \n"

        // Move PSP back to start of saved context
        "sub r0, r5             \n"  // r0 = r0 - 32

        // Call C function to save PSP and get next task's PSP
        "bl PendSV_Handler_C    \n"

        // r0 now contains new task's PSP (points to r4)

        // Restore r4-r7
        "ldmia r0!, {r4-r7}     \n"

        // Restore r8-r11 (load into r1-r4 first)
        "ldmia r0!, {r1-r4}     \n"
        "mov r8, r1             \n"
        "mov r9, r2             \n"
        "mov r10, r3            \n"
        "mov r11, r4            \n"

        // Update PSP for new task (r0 already points to correct location)
        "msr psp, r0            \n"

        // Exception return
        "ldr r0, =0xFFFFFFFD    \n"
        "bx r0                  \n"
    );
}

// C function called by PendSV_Handler
// Takes current PSP, returns new PSP
extern "C" void* PendSV_Handler_C(void* current_psp) {
    using namespace alloy::rtos;

    // Save current task's stack pointer
    if (g_scheduler.current_task != nullptr) {
        g_scheduler.current_task->stack_pointer = current_psp;
        g_scheduler.current_task->state = TaskState::Ready;
    }

    // Get next task to run
    g_scheduler.current_task = g_scheduler.ready_queue.get_highest_priority();

    if (g_scheduler.current_task != nullptr) {
        g_scheduler.current_task->state = TaskState::Running;
        return g_scheduler.current_task->stack_pointer;
    }

    // No task ready - should never happen if idle task exists
    return current_psp;
}

#endif // __ARM_ARCH_6M__ || __ARM_ARCH_8M_BASE__
