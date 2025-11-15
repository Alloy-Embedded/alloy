/// Alloy RTOS Scheduler Implementation

#include "rtos/scheduler.hpp"

#include "hal/interface/systick.hpp"

// Include platform-specific systick implementation
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
    #if defined(STM32F103) || defined(STM32F1)
        #include "hal/st/stm32f1/systick.hpp"
    #elif defined(STM32F407) || defined(STM32F4)
        #include "hal/st/stm32f4/systick.hpp"
    #elif defined(SAMD21)
        #include "hal/microchip/samd21/systick.hpp"
    #elif defined(ATSAME70)
    // SAME70 uses ARM SysTick (TODO: implement)
    #endif
#elif defined(ESP32) || defined(ESP_PLATFORM)
    #include "hal/espressif/esp32/systick.hpp"
#elif defined(__riscv)
    // RISC-V platforms (RP2040, etc.)
    #include "hal/raspberrypi/rp2040/systick.hpp"
#elif defined(__x86_64__) || defined(__aarch64__) || defined(_WIN64) || defined(__APPLE__)
    #include "hal/vendors/host/systick.hpp"
#endif

namespace alloy::rtos {

// Global scheduler state
SchedulerState g_scheduler;

// ReadyQueue implementation

TaskControlBlock* ReadyQueue::get_highest_priority() {
    if (priority_bitmap_ == 0) {
        return nullptr;  // No tasks ready
    }

    // Find highest set bit using CLZ
    core::u8 highest = find_highest_bit(priority_bitmap_);

    return ready_lists_[highest];
}

void ReadyQueue::make_ready(TaskControlBlock* task) {
    if (task == nullptr)
        return;

    // Set bit for this priority
    priority_bitmap_ |= (1 << task->priority);

    // Add to head of linked list for this priority
    task->next = ready_lists_[task->priority];
    ready_lists_[task->priority] = task;

    task->state = TaskState::Ready;
}

void ReadyQueue::make_not_ready(TaskControlBlock* task) {
    if (task == nullptr)
        return;

    // Remove from linked list
    TaskControlBlock** current = &ready_lists_[task->priority];
    while (*current != nullptr) {
        if (*current == task) {
            *current = task->next;
            task->next = nullptr;
            break;
        }
        current = &((*current)->next);
    }

    // If no more tasks at this priority, clear bit
    if (ready_lists_[task->priority] == nullptr) {
        priority_bitmap_ &= ~(1 << task->priority);
    }
}

// Scheduler implementation

namespace scheduler {

void init() {
    g_scheduler = SchedulerState();
}

[[noreturn]] void start() {
    // Mark scheduler as started
    g_scheduler.started = true;

    // Get first task to run (highest priority)
    g_scheduler.current_task = g_scheduler.ready_queue.get_highest_priority();

    if (g_scheduler.current_task == nullptr) {
        // No tasks to run - should never happen if idle task exists
        while (1) {
#if defined(__ARM_ARCH)
            __asm volatile("wfi");  // Wait for interrupt (ARM)
#elif defined(ESP32)
            __asm volatile("waiti 0");  // Wait for interrupt (Xtensa)
#else
            // Generic wait - just loop
#endif
        }
    }

    g_scheduler.current_task->state = TaskState::Running;

    // Start first task (platform-specific)
    alloy::rtos::start_first_task();

    // Never reached
    while (1)
        ;
}

void tick() {
    // Increment tick counter
    g_scheduler.tick_counter++;

    // Wake delayed tasks
    wake_delayed_tasks();

    // Reschedule if needed
    reschedule();
}

void delay(core::u32 ms) {
    if (!g_scheduler.started || ms == 0)
        return;

    TaskControlBlock* current = g_scheduler.current_task;
    if (current == nullptr)
        return;

    // Calculate wake time using RTOS tick counter (1ms resolution)
    current->wake_time = g_scheduler.tick_counter + ms;
    current->state = TaskState::Delayed;

    // Remove from ready queue
    g_scheduler.ready_queue.make_not_ready(current);

    // Add to delayed list
    current->next = g_scheduler.delayed_tasks;
    g_scheduler.delayed_tasks = current;

    // Force reschedule
    reschedule();

    // Trigger context switch
    g_scheduler.need_context_switch = true;
}

void yield() {
    if (!g_scheduler.started)
        return;

    // Simply reschedule
    reschedule();

    if (g_scheduler.need_context_switch) {
        // Trigger context switch (platform-specific)
        extern void trigger_context_switch();
        trigger_context_switch();
    }
}

void block_current_task(TaskControlBlock** wait_list) {
    if (!g_scheduler.started)
        return;

    TaskControlBlock* current = g_scheduler.current_task;
    if (current == nullptr)
        return;

    // Remove from ready queue
    current->state = TaskState::Blocked;
    g_scheduler.ready_queue.make_not_ready(current);

    // Add to wait list
    current->next = *wait_list;
    *wait_list = current;

    // Reschedule
    reschedule();

    // Trigger context switch
    g_scheduler.need_context_switch = true;
}

void unblock_one_task(TaskControlBlock** wait_list) {
    if (*wait_list == nullptr)
        return;

    // Remove first task from wait list
    TaskControlBlock* task = *wait_list;
    *wait_list = task->next;
    task->next = nullptr;

    // Add to ready queue
    g_scheduler.ready_queue.make_ready(task);

    // Reschedule if higher priority than current
    if (g_scheduler.current_task == nullptr ||
        task->priority > g_scheduler.current_task->priority) {
        reschedule();
    }
}

void unblock_all_tasks(TaskControlBlock** wait_list) {
    while (*wait_list != nullptr) {
        unblock_one_task(wait_list);
    }
}

void reschedule() {
    if (!g_scheduler.started)
        return;

    // Find highest priority ready task
    TaskControlBlock* next = g_scheduler.ready_queue.get_highest_priority();

    if (next == nullptr) {
        // No tasks ready - should not happen if idle task exists
        return;
    }

    // Check if we need to switch
    if (next != g_scheduler.current_task) {
        // Put current task back in ready queue (if it was running)
        if (g_scheduler.current_task != nullptr &&
            g_scheduler.current_task->state == TaskState::Running) {
            g_scheduler.current_task->state = TaskState::Ready;
            // Note: current task is already in ready queue
        }

        // Switch to next task
        next->state = TaskState::Running;
        g_scheduler.current_task = next;
        g_scheduler.need_context_switch = true;
    }
}

void wake_delayed_tasks() {
    TaskControlBlock** current = &g_scheduler.delayed_tasks;
    core::u32 now = g_scheduler.tick_counter;

    while (*current != nullptr) {
        if (now >= (*current)->wake_time) {
            // Time to wake up this task
            TaskControlBlock* task = *current;
            *current = task->next;  // Remove from delayed list
            task->next = nullptr;

            // Add to ready queue
            g_scheduler.ready_queue.make_ready(task);
        } else {
            current = &((*current)->next);
        }
    }
}

}  // namespace scheduler

// RTOS API implementation

namespace RTOS {

void start() {
    scheduler::init();
    scheduler::start();
}

void delay(core::u32 ms) {
    scheduler::delay(ms);
}

void yield() {
    scheduler::yield();
}

TaskControlBlock* current_task() {
    return g_scheduler.current_task;
}

core::u32 get_tick_count() {
    return g_scheduler.tick_counter;
}

void tick() {
    scheduler::tick();
}

bool need_context_switch() {
    return g_scheduler.need_context_switch;
}

void register_task(TaskControlBlock* tcb) {
    if (tcb == nullptr)
        return;

    // Add task to ready queue
    g_scheduler.ready_queue.make_ready(tcb);
}

}  // namespace RTOS

}  // namespace alloy::rtos
