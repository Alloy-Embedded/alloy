/// Alloy RTOS - Host (PC) Context Switching
///
/// Implements RTOS context switching for host platforms using std::thread.
/// This allows testing RTOS applications on a PC without embedded hardware.
///
/// Implementation Strategy:
/// - Each task runs in its own std::thread
/// - Context switch is simulated via thread sleep/wake (condition variables)
/// - Priority is mapped to OS thread priority where supported
/// - Scheduler tick runs in a separate timer thread
///
/// Limitations compared to embedded:
/// - Context switch overhead higher (~microseconds vs nanoseconds)
/// - Stack usage tracked but not strictly limited
/// - No true real-time guarantees (depends on OS scheduler)
///
/// Benefits:
/// - Test RTOS logic on PC
/// - Debug with standard tools (gdb, lldb, Visual Studio)
/// - CI/CD integration without hardware
/// - Rapid development iteration

#ifndef ALLOY_RTOS_PLATFORM_HOST_CONTEXT_HPP
#define ALLOY_RTOS_PLATFORM_HOST_CONTEXT_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "rtos/rtos.hpp"

#include "core/types.hpp"

namespace alloy::rtos::platform {

/// Host-specific task context
///
/// Stores std::thread and synchronization primitives for each task
struct HostTaskContext {
    std::thread thread;                   ///< OS thread for this task
    std::mutex mutex;                     ///< Protects task state
    std::condition_variable cv;           ///< For sleep/wake coordination
    std::atomic<bool> should_run{false};  ///< Task is allowed to run
    std::atomic<bool> terminated{false};  ///< Task has exited
    void (*task_func)();                  ///< Task entry point

    HostTaskContext() = default;
    ~HostTaskContext() {
        if (thread.joinable()) {
            terminated = true;
            should_run = true;
            cv.notify_one();
            thread.join();
        }
    }

    // Non-copyable
    HostTaskContext(const HostTaskContext&) = delete;
    HostTaskContext& operator=(const HostTaskContext&) = delete;
};

/// Initialize task stack (called by Task constructor)
///
/// On host, this creates a std::thread for the task but doesn't start it yet.
/// The thread will be started when RTOS::start() is called.
///
/// @param tcb Task control block to initialize
/// @param func Task entry point function
void init_task_stack(TaskControlBlock* tcb, void (*func)());

/// Perform context switch to highest priority ready task
///
/// On host, this wakes the target task's thread and puts current task to sleep.
/// The OS scheduler handles the actual thread switching.
void context_switch();

/// Start first task (called by RTOS::start())
///
/// Initializes all task threads and begins scheduler tick thread.
/// Never returns - enters infinite scheduling loop.
[[noreturn]] void start_first_task();

/// Trigger a context switch (used by yield, delay, IPC operations)
///
/// Sets a flag that causes the scheduler to run at next opportunity.
void trigger_context_switch();

/// Get host task context from TCB
///
/// @param tcb Task control block
/// @return Host-specific task context
HostTaskContext* get_host_context(TaskControlBlock* tcb);

/// Host scheduler tick thread
///
/// Runs in separate thread, calls RTOS::tick() every millisecond.
/// This simulates the SysTick interrupt on embedded platforms.
void scheduler_tick_thread();

}  // namespace alloy::rtos::platform

#endif  // ALLOY_RTOS_PLATFORM_HOST_CONTEXT_HPP
