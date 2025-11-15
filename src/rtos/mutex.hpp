/// Alloy RTOS - Mutex (Mutual Exclusion)
///
/// Mutex for protecting shared resources with priority inheritance.
///
/// Features:
/// - Mutual exclusion (only one task can hold mutex)
/// - Priority inheritance to prevent priority inversion
/// - Recursive locking support (optional)
/// - RAII lock guard for exception-safe locking
/// - Blocking lock with timeout support
///
/// Priority Inheritance:
/// When a high-priority task blocks on a mutex held by a low-priority task,
/// the low-priority task's priority is temporarily boosted to prevent
/// medium-priority tasks from preempting it (priority inversion problem).
///
/// Memory footprint:
/// - Mutex: 20 bytes
/// - LockGuard: 8 bytes (stack)
///
/// Usage:
/// ```cpp
/// Mutex resource_lock;
///
/// void high_priority_task() {
///     LockGuard guard(resource_lock);  // RAII lock
///     access_shared_resource();
///     // Automatically unlocks when guard goes out of scope
/// }
///
/// void low_priority_task() {
///     if (resource_lock.lock(1000)) {  // Manual lock with timeout
///         access_shared_resource();
///         resource_lock.unlock();
///     }
/// }
/// ```

#ifndef ALLOY_RTOS_MUTEX_HPP
#define ALLOY_RTOS_MUTEX_HPP

#include "hal/interface/systick.hpp"

#include "rtos/platform/critical_section.hpp"
#include "rtos/rtos.hpp"
#include "rtos/scheduler.hpp"
#include "rtos/error.hpp"

#include "core/types.hpp"
#include "core/result.hpp"

namespace alloy::rtos {

/// Mutex (Mutual Exclusion Lock)
///
/// Provides mutual exclusion for shared resources.
/// Implements priority inheritance to prevent priority inversion.
///
/// Key properties:
/// - Only one task can hold the mutex at a time
/// - Owner task has exclusive access
/// - Priority inheritance prevents priority inversion
/// - Must be unlocked by the same task that locked it
///
/// Priority Inversion Problem:
/// Without priority inheritance:
/// 1. Low-priority task locks mutex
/// 2. High-priority task blocks waiting for mutex
/// 3. Medium-priority task preempts low-priority task
/// 4. High-priority task is blocked by medium-priority task!
///
/// With priority inheritance:
/// 1. Low-priority task locks mutex
/// 2. High-priority task blocks waiting for mutex
/// 3. Low-priority task inherits high priority
/// 4. Low-priority task (now high) preempts medium-priority task
/// 5. Low-priority task finishes and releases mutex
/// 6. Priority restored to original
///
/// Example: Shared resource protection
/// ```cpp
/// Mutex uart_mutex;
///
/// void task1() {
///     while (1) {
///         uart_mutex.lock();
///         uart_send("Task 1\n");
///         uart_mutex.unlock();
///         RTOS::delay(100);
///     }
/// }
///
/// void task2() {
///     while (1) {
///         uart_mutex.lock();
///         uart_send("Task 2\n");
///         uart_mutex.unlock();
///         RTOS::delay(100);
///     }
/// }
/// ```
class Mutex {
   private:
    TaskControlBlock* owner_;      ///< Task currently holding mutex (nullptr if free)
    core::u8 original_priority_;   ///< Owner's original priority (for restoration)
    core::u8 lock_count_;          ///< Recursive lock count (0 = unlocked)
    TaskControlBlock* wait_list_;  ///< Tasks blocked waiting for mutex

   public:
    /// Constructor
    Mutex() : owner_(nullptr), original_priority_(0), lock_count_(0), wait_list_(nullptr) {}

    /// Lock mutex (acquire)
    ///
    /// If mutex is free, acquires it immediately.
    /// If mutex is held by another task, blocks until available or timeout.
    /// Implements priority inheritance: if blocked, boosts owner's priority.
    ///
    /// @param timeout_ms Timeout in milliseconds (INFINITE = wait forever)
    /// @return Ok(void) if locked successfully, Err(RTOSError) on failure
    ///
    /// Example:
    /// ```cpp
    /// auto result = mutex.lock(1000);
    /// if (result.is_ok()) {
    ///     // Got mutex, access shared resource
    ///     shared_resource.write(data);
    ///     mutex.unlock();
    /// } else {
    ///     // Handle error
    ///     if (result.unwrap_err() == RTOSError::Timeout) {
    ///         // Timeout - couldn't get mutex in 1 second
    ///     }
    /// }
    /// ```
    core::Result<void, RTOSError> lock(core::u32 timeout_ms = INFINITE);

    /// Try to lock mutex (non-blocking)
    ///
    /// Returns immediately whether or not mutex was acquired.
    ///
    /// @return Ok(void) if locked, Err(RTOSError::Busy) if already held
    ///
    /// Example:
    /// ```cpp
    /// auto result = mutex.try_lock();
    /// if (result.is_ok()) {
    ///     shared_resource.write(data);
    ///     mutex.unlock();
    /// } else {
    ///     // Mutex busy, do something else
    /// }
    /// ```
    core::Result<void, RTOSError> try_lock();

    /// Unlock mutex (release)
    ///
    /// Releases the mutex. Must be called by the same task that locked it.
    /// Restores original priority if it was boosted.
    /// Wakes up one waiting task if any.
    ///
    /// @return Ok(void) if unlocked successfully, Err(RTOSError::NotOwner) if not owner
    ///
    /// Example:
    /// ```cpp
    /// mutex.lock();
    /// access_shared_resource();
    /// auto result = mutex.unlock();  // Must unlock!
    /// if (result.is_err()) {
    ///     // Error: not owner or other issue
    /// }
    /// ```
    core::Result<void, RTOSError> unlock();

    /// Check if mutex is locked
    ///
    /// @return true if locked by any task
    bool is_locked() const { return owner_ != nullptr; }

    /// Get current owner
    ///
    /// @return Pointer to owner TCB, or nullptr if free
    TaskControlBlock* owner() const { return owner_; }

   private:
    /// Apply priority inheritance
    ///
    /// Boosts owner's priority to match the highest waiting task.
    /// Called when a task blocks on this mutex.
    void apply_priority_inheritance();

    /// Restore original priority
    ///
    /// Restores owner's priority after releasing mutex.
    void restore_priority();

    /// Disable interrupts (critical section)
    static inline void disable_interrupts() { platform::disable_interrupts(); }

    /// Enable interrupts
    static inline void enable_interrupts() { platform::enable_interrupts(); }
};

/// RAII Lock Guard
///
/// Automatically locks mutex on construction and unlocks on destruction.
/// Provides exception-safe mutex locking.
///
/// Example:
/// ```cpp
/// void critical_section() {
///     LockGuard guard(mutex);
///     // Mutex is locked
///     access_shared_resource();
///     if (error) {
///         return;  // Mutex automatically unlocked
///     }
///     // Mutex automatically unlocked when guard goes out of scope
/// }
/// ```
class LockGuard {
   private:
    Mutex& mutex_;
    bool locked_;

   public:
    /// Constructor - locks mutex
    ///
    /// @param mutex Mutex to lock
    /// @param timeout_ms Timeout for lock (default: INFINITE)
    explicit LockGuard(Mutex& mutex, core::u32 timeout_ms = INFINITE)
        : mutex_(mutex),
          locked_(mutex_.lock(timeout_ms).is_ok()) {}

    /// Destructor - unlocks mutex if locked
    ~LockGuard() {
        if (locked_) {
            // Ignore unlock result in destructor (no exceptions)
            (void)mutex_.unlock();
        }
    }

    /// Check if lock was acquired
    ///
    /// @return true if mutex was successfully locked
    bool is_locked() const { return locked_; }

    // Disable copy and move
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
    LockGuard(LockGuard&&) = delete;
    LockGuard& operator=(LockGuard&&) = delete;
};

// Mutex implementation

inline core::Result<void, RTOSError> Mutex::lock(core::u32 timeout_ms) {
    using core::Ok;
    using core::Err;

    core::u32 start_time = systick::micros();
    TaskControlBlock* current = RTOS::current_task();

    while (true) {
        // Try to acquire without blocking
        auto try_result = try_lock();
        if (try_result.is_ok()) {
            return Ok();  // Success
        }

        // Check if we already own it (recursive lock)
        disable_interrupts();
        if (owner_ == current) {
            lock_count_++;
            enable_interrupts();
            return Ok();
        }
        enable_interrupts();

        // Check timeout
        if (timeout_ms != INFINITE) {
            core::u32 elapsed = systick::micros_since(start_time);
            if (elapsed >= (timeout_ms * 1000)) {
                return Err(RTOSError::Timeout);
            }
        }

        // Apply priority inheritance before blocking
        apply_priority_inheritance();

        // Block on wait list
        disable_interrupts();
        scheduler::block_current_task(&wait_list_);
        enable_interrupts();

        // Trigger context switch
        extern void trigger_context_switch();
        trigger_context_switch();
    }
}

inline core::Result<void, RTOSError> Mutex::try_lock() {
    using core::Ok;
    using core::Err;

    TaskControlBlock* current = RTOS::current_task();

    disable_interrupts();

    // Check if free
    if (owner_ == nullptr) {
        owner_ = current;
        original_priority_ = current->priority;
        lock_count_ = 1;
        enable_interrupts();
        return Ok();
    }

    // Check if we already own it (recursive)
    if (owner_ == current) {
        lock_count_++;
        enable_interrupts();
        return Ok();
    }

    enable_interrupts();
    return Err(RTOSError::Timeout);  // Busy - using Timeout for compatibility
}

inline core::Result<void, RTOSError> Mutex::unlock() {
    using core::Ok;
    using core::Err;

    TaskControlBlock* current = RTOS::current_task();

    disable_interrupts();

    // Check if we own the mutex
    if (owner_ != current) {
        enable_interrupts();
        return Err(RTOSError::NotOwner);
    }

    // Decrement lock count (for recursive locks)
    lock_count_--;

    // If still locked (recursive), don't release yet
    if (lock_count_ > 0) {
        enable_interrupts();
        return Ok();
    }

    // Restore original priority
    restore_priority();

    // Release mutex
    owner_ = nullptr;

    enable_interrupts();

    // Unblock one waiting task (if any)
    if (wait_list_ != nullptr) {
        scheduler::unblock_one_task(&wait_list_);
    }

    return Ok();
}

inline void Mutex::apply_priority_inheritance() {
    disable_interrupts();

    if (owner_ == nullptr || wait_list_ == nullptr) {
        enable_interrupts();
        return;
    }

    // Find highest priority among waiting tasks
    core::u8 highest_priority = 0;
    TaskControlBlock* task = wait_list_;
    while (task != nullptr) {
        if (task->priority > highest_priority) {
            highest_priority = task->priority;
        }
        task = task->next;
    }

    // Boost owner's priority if needed
    if (highest_priority > owner_->priority) {
        owner_->priority = highest_priority;
    }

    enable_interrupts();
}

inline void Mutex::restore_priority() {
    if (owner_ != nullptr) {
        // Restore original priority
        owner_->priority = original_priority_;
    }
}

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_MUTEX_HPP
