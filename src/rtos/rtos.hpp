/// Alloy RTOS - Lightweight Real-Time Operating System
///
/// A compile-time configured, priority-based preemptive RTOS for embedded systems.
///
/// Features:
/// - 8 priority levels (0 = lowest, 7 = highest)
/// - Preemptive scheduling with deterministic behavior
/// - Static memory allocation (no heap)
/// - Type-safe IPC mechanisms
/// - Context switch <10µs on ARM Cortex-M
///
/// Memory footprint:
/// - RTOS core: ~60 bytes RAM
/// - Per task: 32 bytes TCB + stack size
///
/// Dependencies:
/// - SysTick Timer HAL (for scheduler heartbeat)
///
/// Usage:
/// ```cpp
/// void task1_func() {
///     while (1) {
///         // Task work
///         RTOS::delay(100);  // Delay 100ms
///     }
/// }
///
/// Task<512, Priority::High> task1(task1_func, "Task1");
///
/// int main() {
///     Board::initialize();
///     RTOS::start();  // Never returns
/// }
/// ```

#ifndef ALLOY_RTOS_HPP
#define ALLOY_RTOS_HPP

#include "core/types.hpp"
#include "core/error.hpp"
#include "hal/interface/systick.hpp"
#include <cstddef>
#include <type_traits>

namespace alloy::rtos {

/// Task priority levels (0 = lowest, 7 = highest)
enum class Priority : core::u8 {
    Idle     = 0,  ///< Lowest priority (idle task)
    Lowest   = 1,  ///< Very low priority
    Low      = 2,  ///< Low priority
    Normal   = 3,  ///< Normal priority (default)
    High     = 4,  ///< High priority
    Higher   = 5,  ///< Very high priority
    Highest  = 6,  ///< Highest user priority
    Critical = 7   ///< Critical system priority
};

/// Task state
enum class TaskState : core::u8 {
    Ready,      ///< Ready to run
    Running,    ///< Currently executing
    Blocked,    ///< Waiting on IPC object
    Suspended,  ///< Manually suspended
    Delayed     ///< Sleeping until wake_time
};

/// Forward declaration
class TaskControlBlock;

/// Task Control Block (TCB)
///
/// Contains all state needed to manage a task.
/// Size: 32 bytes on 32-bit systems
struct TaskControlBlock {
    void* stack_pointer;          ///< Current stack pointer (updated on context switch)
    void* stack_base;             ///< Stack bottom (for overflow detection)
    core::u32 stack_size;         ///< Stack size in bytes
    core::u8 priority;            ///< Task priority (0-7)
    TaskState state;              ///< Current task state
    core::u32 wake_time;          ///< Wake time in microseconds (for delayed tasks)
    const char* name;             ///< Task name (for debugging)
    TaskControlBlock* next;       ///< Next task in ready list (same priority)

    /// Constructor
    constexpr TaskControlBlock()
        : stack_pointer(nullptr)
        , stack_base(nullptr)
        , stack_size(0)
        , priority(0)
        , state(TaskState::Ready)
        , wake_time(0)
        , name("unnamed")
        , next(nullptr)
    {}
};

/// Task class template
///
/// Creates a task with static stack allocation.
///
/// @tparam StackSize Stack size in bytes (must be >= 256 and 8-byte aligned)
/// @tparam Pri Task priority level
///
/// Example:
/// ```cpp
/// void my_task() {
///     while (1) {
///         // Do work
///         RTOS::delay(100);
///     }
/// }
///
/// Task<512, Priority::High> task(my_task, "MyTask");
/// ```
template<size_t StackSize, Priority Pri>
class Task {
    static_assert(StackSize >= 256, "Stack size must be at least 256 bytes");
    static_assert(StackSize % 8 == 0, "Stack size must be 8-byte aligned");
    static_assert(Pri >= Priority::Idle && Pri <= Priority::Critical,
                  "Priority must be between Idle (0) and Critical (7)");

private:
    alignas(8) core::u8 stack_[StackSize];  ///< Task stack (8-byte aligned)
    TaskControlBlock tcb_;                   ///< Task control block

public:
    /// Constructor
    ///
    /// @param task_func Task entry point function
    /// @param name Task name (for debugging)
    explicit Task(void (*task_func)(), const char* name = "task");

    /// Get task control block
    TaskControlBlock* get_tcb() { return &tcb_; }
    const TaskControlBlock* get_tcb() const { return &tcb_; }

    /// Get stack usage (for debugging)
    core::u32 get_stack_usage() const;

    /// Check for stack overflow (debug builds)
    bool check_stack_overflow() const;

private:
    /// Initialize task stack with initial context
    void init_stack(void (*task_func)());
};

/// RTOS Namespace - Main API
namespace RTOS {

/// Start the RTOS scheduler
///
/// This function never returns. It begins task scheduling.
/// The highest priority ready task will run first.
///
/// Prerequisites:
/// - At least one task must be created
/// - SysTick timer must be initialized
[[noreturn]] void start();

/// Delay the current task for specified milliseconds
///
/// @param ms Delay in milliseconds
///
/// The task will be blocked and other tasks can run.
/// Uses SysTick for accurate timing.
void delay(core::u32 ms);

/// Yield the CPU to another task
///
/// If another task of equal or higher priority is ready,
/// it will run. Otherwise, current task continues.
void yield();

/// Get current task
///
/// @return Pointer to current task's TCB, or nullptr if not started
TaskControlBlock* current_task();

/// Get system tick count (milliseconds since RTOS start)
///
/// @return Tick count in milliseconds
core::u32 get_tick_count();

/// Scheduler tick - called from SysTick ISR
///
/// @note Internal function, do not call directly
void tick();

/// Check if context switch is needed
///
/// @note Internal function, do not call directly
bool need_context_switch();

/// Register a task with the RTOS
///
/// @note Internal function, called by Task constructor
void register_task(TaskControlBlock* tcb);

} // namespace RTOS

/// Infinite timeout constant
constexpr core::u32 INFINITE = 0xFFFFFFFF;

} // namespace alloy::rtos

// Include platform-specific context switching
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || \
    defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_8M__)
    #include "rtos/platform/arm_context.hpp"
#elif defined(ESP32) || defined(ESP_PLATFORM)
    #include "rtos/platform/xtensa_context.hpp"
#else
    #error "Unsupported platform for RTOS"
#endif

// Include Task implementation
namespace alloy::rtos {

template<size_t StackSize, Priority Pri>
Task<StackSize, Pri>::Task(void (*task_func)(), const char* name)
    : stack_{}
    , tcb_{}
{
    // Initialize TCB
    tcb_.stack_base = &stack_[0];
    tcb_.stack_size = StackSize;
    tcb_.priority = static_cast<core::u8>(Pri);
    tcb_.state = TaskState::Ready;
    tcb_.name = name;
    tcb_.next = nullptr;

    // Initialize stack with initial context
    init_stack(task_func);

    // Register task with RTOS
    RTOS::register_task(&tcb_);
}

template<size_t StackSize, Priority Pri>
void Task<StackSize, Pri>::init_stack(void (*task_func)()) {
    // Platform-specific stack initialization
    // This will be implemented in platform-specific files
    extern void init_task_stack(TaskControlBlock* tcb, void (*func)());
    init_task_stack(&tcb_, task_func);
}

template<size_t StackSize, Priority Pri>
core::u32 Task<StackSize, Pri>::get_stack_usage() const {
    // Calculate stack usage (simplified - just checks current SP position)
    if (tcb_.stack_pointer == nullptr) return 0;

    core::u8* sp = static_cast<core::u8*>(tcb_.stack_pointer);
    core::u8* base = static_cast<core::u8*>(tcb_.stack_base);

    // Stack grows downward on most architectures
    return static_cast<core::u32>(base + StackSize - sp);
}

template<size_t StackSize, Priority Pri>
bool Task<StackSize, Pri>::check_stack_overflow() const {
#ifdef DEBUG
    // Check stack canary (if implemented)
    constexpr core::u32 STACK_CANARY = 0xDEADBEEF;
    core::u32* canary = reinterpret_cast<core::u32*>(tcb_.stack_base);
    return (*canary == STACK_CANARY);
#else
    return true;  // No check in release builds
#endif
}

} // namespace alloy::rtos

#endif // ALLOY_RTOS_HPP
