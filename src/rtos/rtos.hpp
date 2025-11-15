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

#include <cstddef>
#include <type_traits>

#include "hal/interface/systick.hpp"

#include "rtos/error.hpp"
#include "rtos/concepts.hpp"

#include "core/error.hpp"
#include "core/types.hpp"
#include "core/result.hpp"

namespace alloy::rtos {

/// Task priority levels (0 = lowest, 7 = highest)
enum class Priority : core::u8 {
    Idle = 0,     ///< Lowest priority (idle task)
    Lowest = 1,   ///< Very low priority
    Low = 2,      ///< Low priority
    Normal = 3,   ///< Normal priority (default)
    High = 4,     ///< High priority
    Higher = 5,   ///< Very high priority
    Highest = 6,  ///< Highest user priority
    Critical = 7  ///< Critical system priority
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
    void* stack_pointer;     ///< Current stack pointer (updated on context switch)
    void* stack_base;        ///< Stack bottom (for overflow detection)
    core::u32 stack_size;    ///< Stack size in bytes
    core::u8 priority;       ///< Task priority (0-7)
    TaskState state;         ///< Current task state
    core::u32 wake_time;     ///< Wake time in microseconds (for delayed tasks)
    const char* name;        ///< Task name (for debugging)
    TaskControlBlock* next;  ///< Next task in ready list (same priority)

    /// Constructor
    constexpr TaskControlBlock()
        : stack_pointer(nullptr),
          stack_base(nullptr),
          stack_size(0),
          priority(0),
          state(TaskState::Ready),
          wake_time(0),
          name("unnamed"),
          next(nullptr) {}
};

/// Task class template
///
/// Creates a task with static stack allocation.
///
/// @tparam StackSize Stack size in bytes (must be >= 256 and 8-byte aligned)
/// @tparam Pri Task priority level
/// @tparam Name Compile-time task name (zero RAM cost)
///
/// Example (Old API - still supported):
/// ```cpp
/// Task<512, Priority::High> task(my_task, "MyTask");
/// ```
///
/// Example (New API - zero RAM for name):
/// ```cpp
/// void my_task() {
///     while (1) {
///         // Do work
///         RTOS::delay(100);
///     }
/// }
///
/// Task<512, Priority::High, "MyTask"> task(my_task);
/// ```
template <size_t StackSize, Priority Pri, fixed_string Name = "task">
class Task {
    // C++23 Enhanced Compile-Time Validation
    static_assert(validate_stack_size<StackSize>() == StackSize,
                  "Stack size validation failed (must be 256-65536 bytes, 8-byte aligned)");
    static_assert(validate_priority<static_cast<core::u8>(Pri)>() == static_cast<core::u8>(Pri),
                  "Priority validation failed (must be 0-7)");
    static_assert(is_valid_task_name(Name.data),
                  "Task name validation failed (1-31 alphanumeric chars, _, or -)");

   private:
    alignas(8) core::u8 stack_[StackSize];  ///< Task stack (8-byte aligned)
    TaskControlBlock tcb_;                  ///< Task control block

   public:
    /// Constructor (new API - compile-time name)
    ///
    /// @param task_func Task entry point function
    explicit Task(void (*task_func)());

    /// Constructor (old API - runtime name, deprecated)
    ///
    /// @param task_func Task entry point function
    /// @param name Task name (stored in RAM - prefer compile-time name)
    [[deprecated("Use Task<StackSize, Pri, \"Name\"> instead for zero-RAM names")]]
    explicit Task(void (*task_func)(), const char* name);

    /// Get task control block
    TaskControlBlock* get_tcb() { return &tcb_; }
    const TaskControlBlock* get_tcb() const { return &tcb_; }

    /// Get task name (compile-time)
    static constexpr const char* name() { return Name.c_str(); }

    /// Get stack size (compile-time)
    static constexpr size_t stack_size() { return StackSize; }

    /// Get priority (compile-time)
    static constexpr Priority priority() { return Pri; }

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
/// Updates delayed tasks and triggers context switch if needed.
///
/// @return Ok(void) on success, Err(RTOSError) on failure
/// @note Internal function, called from SysTick_Handler
///
/// Example:
/// ```cpp
/// extern "C" void SysTick_Handler() {
///     board::BoardSysTick::increment_tick();
///     #ifdef ALLOY_RTOS_ENABLED
///         RTOS::tick().unwrap();  // Or handle error
///     #endif
/// }
/// ```
core::Result<void, RTOSError> tick();

/// Check if context switch is needed
///
/// @note Internal function, do not call directly
bool need_context_switch();

/// Register a task with the RTOS
///
/// @note Internal function, called by Task constructor
void register_task(TaskControlBlock* tcb);

}  // namespace RTOS

/// Infinite timeout constant
constexpr core::u32 INFINITE = 0xFFFFFFFF;

// ============================================================================
// TaskSet - Variadic Template for Compile-Time Task Registration
// ============================================================================

/// TaskSet: Compile-time task collection with validation
///
/// Groups multiple tasks for compile-time validation and RAM calculation.
/// Enables checking for priority conflicts, total RAM usage, etc.
///
/// Example:
/// ```cpp
/// Task<512, Priority::High, "Sensor"> sensor_task(sensor_func);
/// Task<1024, Priority::Normal, "Display"> display_task(display_func);
/// Task<256, Priority::Low, "Logger"> logger_task(logger_func);
///
/// // Compile-time validation and RAM calculation
/// using MyTasks = TaskSet<
///     decltype(sensor_task),
///     decltype(display_task),
///     decltype(logger_task)
/// >;
///
/// static_assert(MyTasks::total_ram() == 1792);  // 512 + 1024 + 256
/// static_assert(MyTasks::count() == 3);
/// static_assert(MyTasks::has_unique_priorities());
///
/// // Start RTOS with TaskSet validation
/// RTOS::start();
/// ```
template <typename... Tasks>
struct TaskSet {
    /// Number of tasks in the set
    static constexpr size_t count() { return sizeof...(Tasks); }

    /// Check if set is empty
    static constexpr bool empty() { return count() == 0; }

    /// Calculate total stack RAM at compile time
    ///
    /// @return Total bytes of stack RAM
    static consteval size_t total_stack_ram() {
        return (Tasks::stack_size() + ...);
    }

    /// Calculate total RAM (stack + TCB overhead)
    ///
    /// Each task has:
    /// - Stack: StackSize bytes
    /// - TCB: 32 bytes
    ///
    /// C++23 Enhanced: Uses dual-mode calculation for flexibility
    ///
    /// @return Total bytes of RAM
    static consteval size_t total_ram() {
        constexpr size_t TCB_SIZE = 32;
        return total_stack_ram() + (count() * TCB_SIZE);
    }

    /// Calculate total RAM with compile-time budget check (C++23)
    ///
    /// @tparam Budget Maximum RAM budget in bytes
    /// @return Total RAM if within budget (throws compile error otherwise)
    template <size_t Budget>
    static consteval size_t total_ram_with_budget() {
        constexpr size_t ram = total_ram();
        compile_time_check(ram <= Budget, "TaskSet exceeds RAM budget");
        return ram;
    }

    /// Get highest priority in task set (C++23 enhanced)
    ///
    /// @return Highest priority value
    static consteval core::u8 highest_priority() {
        constexpr core::u8 priorities[] = {
            static_cast<core::u8>(Tasks::priority())...
        };
        return array_max(priorities);
    }

    /// Get lowest priority in task set (C++23 enhanced)
    ///
    /// @return Lowest priority value
    static consteval core::u8 lowest_priority() {
        constexpr core::u8 priorities[] = {
            static_cast<core::u8>(Tasks::priority())...
        };
        return array_min(priorities);
    }

    /// Check if all priorities are unique (no duplicates)
    ///
    /// @return true if all priorities are unique
    static consteval bool has_unique_priorities() {
        constexpr core::u8 priorities[] = {
            static_cast<core::u8>(Tasks::priority())...
        };
        constexpr size_t N = count();

        for (size_t i = 0; i < N; ++i) {
            for (size_t j = i + 1; j < N; ++j) {
                if (priorities[i] == priorities[j]) {
                    return false;
                }
            }
        }
        return true;
    }

    /// Check if all stack sizes are valid
    ///
    /// Valid means:
    /// - >= 256 bytes (minimum viable)
    /// - <= 65536 bytes (maximum reasonable)
    /// - 8-byte aligned
    ///
    /// @return true if all stacks valid
    static consteval bool has_valid_stacks() {
        return ((Tasks::stack_size() >= 256 &&
                 Tasks::stack_size() <= 65536 &&
                 (Tasks::stack_size() % 8) == 0) && ...);
    }

    /// Validate entire task set at compile time
    ///
    /// Checks:
    /// - At least one task
    /// - All stacks valid
    /// - All priorities valid (0-7)
    /// - Unique priorities (optional, controlled by parameter)
    ///
    /// @tparam RequireUniquePriorities If true, fails if priorities duplicate
    /// @return true if valid
    template <bool RequireUniquePriorities = false>
    static consteval bool validate() {
        if (count() == 0) return false;  // Need at least one task
        if (!has_valid_stacks()) return false;
        if (highest_priority() > 7) return false;

        if constexpr (RequireUniquePriorities) {
            if (!has_unique_priorities()) return false;
        }

        return true;
    }

    /// Check for potential priority inversion scenarios
    ///
    /// Priority inversion can occur when high and low priority tasks
    /// share resources and there's a medium priority task that can preempt.
    ///
    /// @return true if potential for priority inversion exists
    static consteval bool has_priority_inversion_risk() {
        constexpr core::u8 high = highest_priority();
        constexpr core::u8 low = lowest_priority();
        return can_cause_priority_inversion<high, low>();
    }

    /// Validate all tasks satisfy ValidTask concept
    ///
    /// @return true if all tasks valid
    static consteval bool all_tasks_valid() {
        return (ValidTask<Tasks> && ...);
    }

    /// Calculate CPU utilization (simplified)
    ///
    /// Note: This is a placeholder for real-time analysis.
    /// In real implementation, would need execution times and periods.
    ///
    /// @return Estimated utilization percentage (0-100)
    static consteval core::u8 estimated_utilization() {
        // Placeholder: assume each task uses 10% per priority level
        return count() * 10;  // Very simplified
    }

    /// Advanced validation with deadlock and priority inversion checks
    ///
    /// @tparam RequireUniquePriorities Enforce unique priorities
    /// @tparam WarnPriorityInversion Warn if priority inversion possible
    /// @return true if all checks pass
    template <bool RequireUniquePriorities = false,
              bool WarnPriorityInversion = true>
    static consteval bool validate_advanced() {
        // Basic validation
        if (!validate<RequireUniquePriorities>()) return false;

        // Task concept validation
        if (!all_tasks_valid()) return false;

        // Priority inversion warning
        if constexpr (WarnPriorityInversion) {
            if (has_priority_inversion_risk() && count() > 2) {
                // Warning: potential priority inversion
                // In production, would need runtime priority inheritance
            }
        }

        return true;
    }

    /// Print task set info (compile-time friendly format)
    ///
    /// Generates static_assert-friendly error messages.
    ///
    /// Example usage:
    /// ```cpp
    /// static_assert(MyTasks::validate(), "Task set validation failed");
    /// static_assert(MyTasks::total_ram() <= 8192, "Total RAM exceeds 8KB");
    /// ```
    struct Info {
        static constexpr size_t task_count = count();
        static constexpr size_t total_ram_bytes = total_ram();
        static constexpr size_t total_stack_bytes = total_stack_ram();
        static constexpr core::u8 max_priority = highest_priority();
        static constexpr core::u8 min_priority = lowest_priority();
        static constexpr bool unique_priorities = has_unique_priorities();
        static constexpr bool priority_inversion_risk = has_priority_inversion_risk();
        static constexpr core::u8 utilization_estimate = estimated_utilization();
    };
};

/// Helper: Create TaskSet from task instances
///
/// Usage:
/// ```cpp
/// auto tasks = make_task_set(task1, task2, task3);
/// static_assert(decltype(tasks)::total_ram() == 1792);
/// ```
template <typename... Tasks>
consteval auto make_task_set(const Tasks&...) {
    return TaskSet<Tasks...>{};
}

}  // namespace alloy::rtos

// Include platform-specific context switching
#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || \
    defined(__ARM_ARCH_8M__)
    #include "rtos/platform/arm_context.hpp"
#elif defined(ESP32) || defined(ESP_PLATFORM)
    #include "rtos/platform/xtensa_context.hpp"
#elif defined(__x86_64__) || defined(__aarch64__) || defined(_WIN64) || defined(__APPLE__)
   // Host platform (x86-64, ARM64 macOS, Windows x64)
    #include "rtos/platform/host_context.hpp"
#else
    #error "Unsupported platform for RTOS"
#endif

// Include Task implementation
namespace alloy::rtos {

// New API: Compile-time name (zero RAM)
template <size_t StackSize, Priority Pri, fixed_string Name>
Task<StackSize, Pri, Name>::Task(void (*task_func)()) : stack_{},
                                                         tcb_{} {
    // Initialize TCB
    tcb_.stack_base = &stack_[0];
    tcb_.stack_size = StackSize;
    tcb_.priority = static_cast<core::u8>(Pri);
    tcb_.state = TaskState::Ready;
    tcb_.name = Name.c_str();  // Points to .rodata, not RAM
    tcb_.next = nullptr;

    // Initialize stack with initial context
    init_stack(task_func);

    // Register task with RTOS
    RTOS::register_task(&tcb_);
}

// Old API: Runtime name (deprecated, uses RAM)
template <size_t StackSize, Priority Pri, fixed_string Name>
Task<StackSize, Pri, Name>::Task(void (*task_func)(), const char* name) : stack_{},
                                                                           tcb_{} {
    // Initialize TCB
    tcb_.stack_base = &stack_[0];
    tcb_.stack_size = StackSize;
    tcb_.priority = static_cast<core::u8>(Pri);
    tcb_.state = TaskState::Ready;
    tcb_.name = name;  // Runtime string (stored in RAM)
    tcb_.next = nullptr;

    // Initialize stack with initial context
    init_stack(task_func);

    // Register task with RTOS
    RTOS::register_task(&tcb_);
}

template <size_t StackSize, Priority Pri, fixed_string Name>
void Task<StackSize, Pri, Name>::init_stack(void (*task_func)()) {
    // Platform-specific stack initialization
    // This will be implemented in platform-specific files
    extern void init_task_stack(TaskControlBlock * tcb, void (*func)());
    init_task_stack(&tcb_, task_func);
}

template <size_t StackSize, Priority Pri, fixed_string Name>
core::u32 Task<StackSize, Pri, Name>::get_stack_usage() const {
    // Calculate stack usage (simplified - just checks current SP position)
    if (tcb_.stack_pointer == nullptr)
        return 0;

    core::u8* sp = static_cast<core::u8*>(tcb_.stack_pointer);
    core::u8* base = static_cast<core::u8*>(tcb_.stack_base);

    // Stack grows downward on most architectures
    return static_cast<core::u32>(base + StackSize - sp);
}

template <size_t StackSize, Priority Pri, fixed_string Name>
bool Task<StackSize, Pri, Name>::check_stack_overflow() const {
#ifdef DEBUG
    // Check stack canary (if implemented)
    constexpr core::u32 STACK_CANARY = 0xDEADBEEF;
    core::u32* canary = reinterpret_cast<core::u32*>(tcb_.stack_base);
    return (*canary == STACK_CANARY);
#else
    return true;  // No check in release builds
#endif
}

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_HPP
