/// Xtensa (ESP32) Context Switching Implementation (Bare-Metal)

#ifdef ESP32

    #include "rtos/platform/xtensa_context.hpp"

    #include "rtos/scheduler.hpp"

    #include "core/types.hpp"

namespace alloy::rtos {

void init_task_stack(TaskControlBlock* tcb, void (*func)()) {
    // Get pointer to top of stack (stacks grow downward)
    core::u32* sp =
        reinterpret_cast<core::u32*>(static_cast<core::u8*>(tcb->stack_base) + tcb->stack_size);

    // Align stack to 16 bytes (Xtensa requirement)
    sp = reinterpret_cast<core::u32*>(reinterpret_cast<core::u32>(sp) & ~0xF);

    // Initialize Xtensa exception frame
    xtensa::XtExceptionFrame* frame = reinterpret_cast<xtensa::XtExceptionFrame*>(sp) - 1;

    // Initialize all registers to safe values
    frame->pc = reinterpret_cast<core::u32>(func);  // Task entry point
    frame->ps = 0x00040000;  // PS with INTLEVEL=0, UM=1 (user mode with interrupts enabled)
    frame->a0 = 0;           // Return address (task should never return)
    frame->a1 = reinterpret_cast<core::u32>(frame);  // Stack pointer
    frame->a2 = 0;
    frame->a3 = 0;
    frame->a4 = 0;
    frame->a5 = 0;
    frame->a6 = 0;
    frame->a7 = 0;
    frame->a8 = 0;
    frame->a9 = 0;
    frame->a10 = 0;
    frame->a11 = 0;
    frame->a12 = 0;
    frame->a13 = 0;
    frame->a14 = 0;
    frame->a15 = 0;
    frame->sar = 0;
    frame->exccause = 0;
    frame->excvaddr = 0;
    frame->lbeg = 0;
    frame->lend = 0;
    frame->lcount = 0;

    // Save stack pointer in TCB
    tcb->stack_pointer = frame;
}

namespace xtensa {

void init_xtensa_rtos() {
    // Initialize timer for RTOS tick
    init_rtos_timer();
}

void setup_timer_interrupt() {
    // Initialize hardware timer for 1ms tick
    init_rtos_timer();
}

}  // namespace xtensa

void trigger_context_switch() {
    // For bare-metal, just set flag
    // Full context switch would need assembly implementation
    g_scheduler.need_context_switch = true;
}

[[noreturn]] void start_first_task() {
    // Initialize Xtensa-specific hardware
    xtensa::init_xtensa_rtos();

    // Get first task's stack pointer
    void* sp = g_scheduler.current_task->stack_pointer;

    // Cast to exception frame and restore registers
    xtensa::XtExceptionFrame* frame = static_cast<xtensa::XtExceptionFrame*>(sp);

    // Jump to task (simplified - real implementation needs assembly)
    void (*task_entry)() = reinterpret_cast<void (*)()>(frame->pc);
    task_entry();

    // Never reached
    while (1)
        ;
}

}  // namespace alloy::rtos

// Context switch handler (software interrupt)
extern "C" void context_switch_handler(void* arg) {
    (void)arg;  // Unused

    // Save current task context
    if (alloy::rtos::g_scheduler.current_task != nullptr) {
        // Context is already saved by interrupt entry
    }

    // Get next task
    alloy::rtos::TaskControlBlock* next = alloy::rtos::g_scheduler.current_task;

    // Clear context switch flag
    alloy::rtos::g_scheduler.need_context_switch = false;

    // Restore next task's context
    // This would be done in assembly for a full implementation
}

#endif  // ESP32
