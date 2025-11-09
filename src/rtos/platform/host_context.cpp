/// Alloy RTOS - Host Context Switching Implementation

#include "host_context.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <map>

#include "hal/vendors/host/systick.hpp"

#include "rtos/scheduler.hpp"

namespace alloy::rtos {

// Forward declare platform functions
namespace platform {
void context_switch();
void trigger_context_switch();
HostTaskContext* get_host_context(TaskControlBlock* tcb);
void start_first_task();
void scheduler_tick_thread();
}  // namespace platform

// Global namespace implementations (required by RTOS interface)
void init_task_stack(TaskControlBlock* tcb, void (*func)()) {
    platform::init_task_stack(tcb, func);
}

void trigger_context_switch() {
    platform::trigger_context_switch();
}

namespace scheduler {
void trigger_context_switch() {
    platform::trigger_context_switch();
}
}  // namespace scheduler

}  // namespace alloy::rtos

namespace alloy::rtos::platform {

// Global task context storage
// Maps TCB pointer to host-specific context
static std::map<TaskControlBlock*, HostTaskContext*> g_task_contexts;
static std::mutex g_contexts_mutex;

// Scheduler tick thread
static std::thread g_tick_thread;
static std::atomic<bool> g_tick_thread_running{false};

// Current running task (for context switch coordination)
static std::atomic<TaskControlBlock*> g_current_task_atomic{nullptr};

/// Task wrapper function
///
/// This runs in each task's std::thread. It waits to be signaled
/// by the scheduler before actually executing the task function.
static void task_thread_wrapper(TaskControlBlock* tcb) {
    HostTaskContext* ctx = get_host_context(tcb);
    assert(ctx != nullptr);

    while (!ctx->terminated) {
        // Wait for scheduler to give us permission to run
        {
            std::unique_lock<std::mutex> lock(ctx->mutex);
            ctx->cv.wait(lock,
                         [ctx]() { return ctx->should_run.load() || ctx->terminated.load(); });
        }

        if (ctx->terminated)
            break;

        // Set this task as current
        g_current_task_atomic.store(tcb);
        tcb->state = TaskState::Running;

        // Run the task function
        // Note: Task functions should loop forever or call RTOS::delay()
        ctx->task_func();

        // If we get here, task exited (shouldn't happen in well-formed RTOS)
        ctx->terminated = true;
        tcb->state = TaskState::Suspended;
    }
}

void init_task_stack(TaskControlBlock* tcb, void (*func)()) {
    // Create host-specific context
    HostTaskContext* ctx = new HostTaskContext();
    ctx->task_func = func;
    ctx->should_run = false;
    ctx->terminated = false;

    // Store context
    {
        std::lock_guard<std::mutex> lock(g_contexts_mutex);
        g_task_contexts[tcb] = ctx;
    }

    // Store dummy stack pointer (not used on host, but needed for interface)
    tcb->stack_pointer = static_cast<core::u8*>(tcb->stack_base) + tcb->stack_size;

    // Don't create thread yet - wait for RTOS::start()
}

HostTaskContext* get_host_context(TaskControlBlock* tcb) {
    std::lock_guard<std::mutex> lock(g_contexts_mutex);
    auto it = g_task_contexts.find(tcb);
    return (it != g_task_contexts.end()) ? it->second : nullptr;
}

void context_switch() {
    // Get currently running task
    TaskControlBlock* old_task = g_current_task_atomic.load();

    // Get next task to run from scheduler
    TaskControlBlock* new_task = g_scheduler.ready_queue.get_highest_priority();

    if (new_task == nullptr) {
        // No task ready - shouldn't happen if idle task exists
        std::cerr << "[RTOS Host] Warning: No ready task!" << std::endl;
        return;
    }

    // If same task, nothing to do
    if (new_task == old_task) {
        return;
    }

    // Update scheduler state
    g_scheduler.current_task = new_task;
    g_current_task_atomic.store(new_task);

    // Put old task to sleep (if it exists and isn't blocked/delayed)
    if (old_task != nullptr) {
        HostTaskContext* old_ctx = get_host_context(old_task);
        if (old_ctx != nullptr && old_task->state == TaskState::Running) {
            old_task->state = TaskState::Ready;
            old_ctx->should_run = false;
        }
    }

    // Wake up new task
    HostTaskContext* new_ctx = get_host_context(new_task);
    if (new_ctx != nullptr) {
        new_task->state = TaskState::Running;
        new_ctx->should_run = true;
        new_ctx->cv.notify_one();
    }
}

void trigger_context_switch() {
    g_scheduler.need_context_switch = true;

    // Immediately perform context switch
    scheduler::reschedule();
    context_switch();
}

void scheduler_tick_thread() {
    using namespace std::chrono;

    while (g_tick_thread_running) {
        // Sleep for 1ms
        std::this_thread::sleep_for(milliseconds(1));

        // Call RTOS tick handler
        scheduler::tick();

        // Check if context switch is needed
        if (g_scheduler.need_context_switch) {
            g_scheduler.need_context_switch = false;
            context_switch();
        }
    }
}

void start_first_task() {
    std::cout << "[RTOS Host] Starting scheduler..." << std::endl;

    // Mark scheduler as started
    g_scheduler.started = true;

    // Create threads for all tasks
    {
        std::lock_guard<std::mutex> lock(g_contexts_mutex);
        for (auto& [tcb, ctx] : g_task_contexts) {
            std::cout << "[RTOS Host] Creating thread for task: " << tcb->name << " (priority "
                      << (int)tcb->priority << ")" << std::endl;

            ctx->thread = std::thread(task_thread_wrapper, tcb);

// Set thread priority based on task priority (platform-specific)
// Note: This is best-effort on most platforms
#ifdef __linux__
// Linux: sched_setscheduler
#elif defined(__APPLE__)
// macOS: thread_policy_set (limited control)
#elif defined(_WIN32)
// Windows: SetThreadPriority
#endif
        }
    }

    // Start scheduler tick thread
    g_tick_thread_running = true;
    g_tick_thread = std::thread(scheduler_tick_thread);

    std::cout << "[RTOS Host] Scheduler started. Starting first task..." << std::endl;

    // Get first task and start it
    TaskControlBlock* first_task = g_scheduler.ready_queue.get_highest_priority();
    if (first_task == nullptr) {
        std::cerr << "[RTOS Host] FATAL: No tasks to run!" << std::endl;
        std::terminate();
    }

    g_scheduler.current_task = first_task;
    g_current_task_atomic.store(first_task);

    HostTaskContext* first_ctx = get_host_context(first_task);
    if (first_ctx != nullptr) {
        first_task->state = TaskState::Running;
        first_ctx->should_run = true;
        first_ctx->cv.notify_one();
    }

    // Main thread becomes idle - wait for SIGINT or all tasks to terminate
    std::cout << "[RTOS Host] Main thread becoming idle. Press Ctrl+C to stop." << std::endl;

    while (g_tick_thread_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Check if all tasks terminated (shouldn't happen in well-formed RTOS)
        bool all_terminated = true;
        {
            std::lock_guard<std::mutex> lock(g_contexts_mutex);
            for (const auto& [tcb, ctx] : g_task_contexts) {
                if (!ctx->terminated) {
                    all_terminated = false;
                    break;
                }
            }
        }

        if (all_terminated) {
            std::cout << "[RTOS Host] All tasks terminated. Exiting." << std::endl;
            break;
        }
    }

    // Cleanup
    g_tick_thread_running = false;
    if (g_tick_thread.joinable()) {
        g_tick_thread.join();
    }

    {
        std::lock_guard<std::mutex> lock(g_contexts_mutex);
        for (auto& [tcb, ctx] : g_task_contexts) {
            delete ctx;
        }
        g_task_contexts.clear();
    }

    std::cout << "[RTOS Host] Scheduler stopped." << std::endl;
    std::exit(0);  // Exit cleanly
}

}  // namespace alloy::rtos::platform
