/// Alloy RTOS Scheduler
///
/// Implements priority-based preemptive scheduling with O(1) task selection.
///
/// Algorithm:
/// - Priority bitmap for O(1) highest priority search
/// - Array of task lists (one per priority level)
/// - Uses CLZ instruction on ARM for fast bit scanning
///
/// Performance:
/// - get_highest_priority(): 1-2 cycles (CLZ instruction)
/// - make_ready(): ~5-10 cycles
/// - make_not_ready(): ~10-20 cycles

#ifndef ALLOY_RTOS_SCHEDULER_HPP
#define ALLOY_RTOS_SCHEDULER_HPP

#include "rtos/rtos.hpp"
#include "core/types.hpp"

namespace alloy::rtos {

/// Ready Queue
///
/// Manages ready tasks using priority bitmap for O(1) selection.
/// Uses linked lists for tasks at same priority (future: round-robin).
class ReadyQueue {
private:
    /// Priority bitmap - bit N set = tasks at priority N are ready
    core::u8 priority_bitmap_;

    /// Array of task lists, one per priority level (0-7)
    TaskControlBlock* ready_lists_[8];

public:
    /// Constructor
    constexpr ReadyQueue()
        : priority_bitmap_(0)
        , ready_lists_{}
    {}

    /// Get highest priority ready task (O(1))
    ///
    /// @return Pointer to highest priority task, or nullptr if none ready
    TaskControlBlock* get_highest_priority();

    /// Add task to ready queue (O(1))
    ///
    /// @param task Task to make ready
    void make_ready(TaskControlBlock* task);

    /// Remove task from ready queue (O(1))
    ///
    /// @param task Task to remove
    void make_not_ready(TaskControlBlock* task);

    /// Check if any tasks are ready
    ///
    /// @return true if at least one task is ready
    bool has_ready_tasks() const {
        return priority_bitmap_ != 0;
    }

    /// Get priority bitmap (for debugging)
    core::u8 get_bitmap() const {
        return priority_bitmap_;
    }

private:
    /// Find highest set bit in bitmap (0-7)
    ///
    /// Uses __builtin_clz (Count Leading Zeros) for O(1) operation.
    /// On ARM Cortex-M, this compiles to a single CLZ instruction.
    ///
    /// @param bitmap 8-bit priority bitmap
    /// @return Highest set bit position (0-7), or 0 if bitmap is 0
    static inline core::u8 find_highest_bit(core::u8 bitmap) {
        if (bitmap == 0) return 0;

        // __builtin_clz works on 32-bit values, so we need to adjust
        // CLZ on 0x01 (bit 0 set) returns 31
        // CLZ on 0x80 (bit 7 set) returns 24
        // We want: bit 0 -> priority 0, bit 7 -> priority 7
        return 7 - static_cast<core::u8>(__builtin_clz(static_cast<core::u32>(bitmap)) - 24);
    }
};

/// Scheduler State
struct SchedulerState {
    ReadyQueue ready_queue;              ///< Ready task queue
    TaskControlBlock* current_task;      ///< Currently running task
    TaskControlBlock* delayed_tasks;     ///< Linked list of delayed tasks
    core::u32 tick_counter;              ///< System tick counter (milliseconds)
    bool started;                        ///< RTOS started flag
    bool need_context_switch;            ///< Context switch requested

    constexpr SchedulerState()
        : ready_queue()
        , current_task(nullptr)
        , delayed_tasks(nullptr)
        , tick_counter(0)
        , started(false)
        , need_context_switch(false)
    {}
};

/// Global scheduler state
extern SchedulerState g_scheduler;

/// Scheduler Functions
namespace scheduler {

/// Initialize scheduler
void init();

/// Start scheduler (never returns)
[[noreturn]] void start();

/// Scheduler tick - called from SysTick ISR every 1ms
void tick();

/// Delay current task
void delay(core::u32 ms);

/// Yield CPU to another task
void yield();

/// Block current task (add to wait list)
void block_current_task(TaskControlBlock** wait_list);

/// Unblock a single task from wait list
void unblock_one_task(TaskControlBlock** wait_list);

/// Unblock all tasks from wait list
void unblock_all_tasks(TaskControlBlock** wait_list);

/// Reschedule (find next task to run)
void reschedule();

/// Wake delayed tasks (called from tick)
void wake_delayed_tasks();

} // namespace scheduler

} // namespace alloy::rtos

#endif // ALLOY_RTOS_SCHEDULER_HPP
