/// Xtensa (ESP32) Context Switching Implementation

#ifdef ESP32

#include "rtos/platform/xtensa_context.hpp"
#include "rtos/scheduler.hpp"

// ESP-IDF includes for ESP32
#include "esp_timer.h"
#include "esp_attr.h"
#include "freertos/portmacro.h"
#include "xtensa/xtensa_context.h"

namespace alloy::rtos {

void init_task_stack(TaskControlBlock* tcb, void (*func)()) {
    // Get pointer to top of stack (stacks grow downward)
    core::u32* sp = reinterpret_cast<core::u32*>(
        static_cast<core::u8*>(tcb->stack_base) + tcb->stack_size
    );

    // Align stack to 16 bytes (Xtensa requirement)
    sp = reinterpret_cast<core::u32*>(
        reinterpret_cast<core::u32>(sp) & ~0xF
    );

    // Initialize Xtensa exception frame
    // Note: Xtensa is little-endian, and uses a0 as return address

    // Create exception frame on stack
    xtensa::XtExceptionFrame* frame =
        reinterpret_cast<xtensa::XtExceptionFrame*>(sp) - 1;

    // Initialize all registers to safe values
    frame->pc = reinterpret_cast<core::u32>(func);  // Task entry point
    frame->ps = 0x00040000;  // PS with INTLEVEL=0, UM=1 (user mode with interrupts enabled)
    frame->a0 = 0;           // Return address (task should never return)
    frame->a1 = reinterpret_cast<core::u32>(frame);  // Stack pointer
    frame->a2 = 0;           // First argument
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
    frame->sar = 0;          // Shift amount register
    frame->exccause = 0;
    frame->excvaddr = 0;
    frame->lbeg = 0;         // Loop registers
    frame->lend = 0;
    frame->lcount = 0;

    // Save stack pointer in TCB
    tcb->stack_pointer = frame;

#ifdef DEBUG
    // Place stack canary at bottom for overflow detection
    constexpr core::u32 STACK_CANARY = 0xDEADBEEF;
    *reinterpret_cast<core::u32*>(tcb->stack_base) = STACK_CANARY;
#endif
}

namespace xtensa {

// Timer handle for RTOS tick
static esp_timer_handle_t rtos_timer = nullptr;

void init_xtensa_rtos() {
    // Initialize timer for RTOS tick
    setup_timer_interrupt();
}

void setup_timer_interrupt() {
    // Create periodic timer for 1ms (1000us) tick
    esp_timer_create_args_t timer_args = {
        .callback = &timer_isr_handler,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "rtos_tick",
        .skip_unhandled_events = false
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &rtos_timer));

    // Start timer with 1ms period
    ESP_ERROR_CHECK(esp_timer_start_periodic(rtos_timer, 1000));
}

} // namespace xtensa

void trigger_context_switch() {
    // On ESP32, we trigger a context switch by yielding to FreeRTOS
    // Since ESP-IDF uses FreeRTOS underneath, we can use portYIELD_FROM_ISR
    // However, for a pure implementation, we would trigger a software interrupt

    // For now, we'll use a simple approach: set a flag and handle in timer ISR
    g_scheduler.need_context_switch = true;
}

[[noreturn]] void start_first_task() {
    // Initialize Xtensa-specific hardware
    xtensa::init_xtensa_rtos();

    // Get first task's stack pointer
    void* sp = g_scheduler.current_task->stack_pointer;

    // Load context and jump to task
    // Note: This is simplified - a full implementation would use
    // Xtensa-specific assembly to restore the exception frame

    // For ESP32 with ESP-IDF, we can leverage the existing context switch mechanism
    // by setting up the stack properly and returning from interrupt

    // Cast to exception frame and restore registers
    xtensa::XtExceptionFrame* frame =
        static_cast<xtensa::XtExceptionFrame*>(sp);

    // Jump to task (simplified - real implementation needs assembly)
    // This will be handled by the context switch assembly routine
    void (*task_entry)() = reinterpret_cast<void(*)()>(frame->pc);
    task_entry();

    // Never reached
    while (1);
}

} // namespace alloy::rtos

// Timer ISR handler - called every 1ms
extern "C" void IRAM_ATTR timer_isr_handler(void* arg) {
    // Call RTOS scheduler tick
    alloy::rtos::RTOS::tick();

    // If context switch needed, handle it
    if (alloy::rtos::RTOS::need_context_switch()) {
        alloy::rtos::trigger_context_switch();
    }
}

// Context switch handler (software interrupt)
extern "C" void context_switch_handler(void* arg) {
    // Save current task context
    if (alloy::rtos::g_scheduler.current_task != nullptr) {
        // Context is already saved by interrupt entry
        // Just update the stack pointer
        // Note: In a full implementation, we would save additional registers here
    }

    // Get next task (already selected by scheduler)
    alloy::rtos::TaskControlBlock* next = alloy::rtos::g_scheduler.current_task;

    // Clear context switch flag
    alloy::rtos::g_scheduler.need_context_switch = false;

    // Restore next task's context
    // This would be done in assembly for a full implementation
    // For now, we rely on the timer ISR to handle scheduling
}

#endif // ESP32
