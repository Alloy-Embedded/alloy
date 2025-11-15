/// Alloy RTOS - Task Notification Mechanism
///
/// Lightweight inter-task notification system for RTOS.
///
/// Features:
/// - Only 8 bytes per task (compared to queue overhead)
/// - Faster than queues for simple notifications
/// - Supports notification values (32-bit)
/// - Multiple notification modes (set bits, increment, overwrite)
/// - C++23 consteval validation
/// - Zero dynamic allocation
///
/// Memory footprint:
/// - Per task: 8 bytes (1x u32 value + 1x u32 flags)
/// - No additional queues needed
///
/// Use cases:
/// - ISR → Task notifications (fast path)
/// - Event flags between tasks
/// - Semaphore-like counting
/// - Simple producer-consumer patterns
///
/// Usage:
/// ```cpp
/// // In task function
/// void sensor_task() {
///     while (1) {
///         // Wait for notification from ISR
///         auto result = TaskNotification::wait(INFINITE);
///         if (result.is_ok()) {
///             u32 value = result.unwrap();
///             // Process notification value
///         }
///     }
/// }
///
/// // From ISR or another task
/// extern "C" void EXTI_IRQHandler() {
///     TaskNotification::notify(sensor_task_tcb, 0x01, NotifyAction::SetBits);
/// }
/// ```

#ifndef ALLOY_RTOS_TASK_NOTIFICATION_HPP
#define ALLOY_RTOS_TASK_NOTIFICATION_HPP

#include <cstddef>
#include <atomic>

#include "rtos/rtos.hpp"
#include "rtos/error.hpp"
#include "core/types.hpp"
#include "core/result.hpp"

namespace alloy::rtos {

// ============================================================================
// Task Notification Types
// ============================================================================

/// Notification action when notifying a task
enum class NotifyAction : core::u8 {
    /// Set notification bits (OR operation)
    /// Use case: Event flags
    /// Example: notify(task, 0x01, SetBits) | notify(task, 0x02, SetBits) → value = 0x03
    SetBits = 0,

    /// Increment notification value
    /// Use case: Counting semaphore replacement
    /// Example: notify(task, 1, Increment) | notify(task, 1, Increment) → value = 2
    Increment = 1,

    /// Overwrite notification value
    /// Use case: Simple data passing (last wins)
    /// Example: notify(task, 100, Overwrite) | notify(task, 200, Overwrite) → value = 200
    Overwrite = 2,

    /// Overwrite only if no pending notification
    /// Use case: Data passing with overflow detection
    /// Example: Returns error if notification already pending
    OverwriteIfEmpty = 3,
};

/// Notification clear mode when waiting
enum class NotifyClearMode : core::u8 {
    /// Clear notification value on entry (before blocking)
    /// Use case: Start fresh, ignore old notifications
    ClearOnEntry = 0,

    /// Clear notification value on exit (after waking)
    /// Use case: Consume the notification value
    ClearOnExit = 1,

    /// Don't clear (manual clear required)
    /// Use case: Persistent flags
    NoClear = 2,
};

// ============================================================================
// Task Notification Storage (per task)
// ============================================================================

/// Task notification state (embedded in TCB)
///
/// Size: 8 bytes (2x u32)
/// - notification_value: 32-bit value (data or flags)
/// - pending_count: Number of pending notifications (0 = none)
///
/// Thread safety: Accessed from ISR and task context
struct TaskNotificationState {
    std::atomic<core::u32> notification_value{0};  ///< Notification value or flags
    std::atomic<core::u32> pending_count{0};       ///< Number of pending notifications

    /// Constructor
    constexpr TaskNotificationState() = default;

    /// Reset notification state
    void reset() {
        notification_value.store(0, std::memory_order_relaxed);
        pending_count.store(0, std::memory_order_relaxed);
    }

    /// Check if notification is pending
    bool is_pending() const {
        return pending_count.load(std::memory_order_acquire) > 0;
    }
};

// ============================================================================
// Task Notification API
// ============================================================================

/// Task Notification - Lightweight inter-task communication
///
/// Provides fast notification mechanism between tasks and ISRs.
/// More efficient than queues for simple event notifications.
///
/// Example:
/// ```cpp
/// // Receiver task
/// void receiver_task() {
///     while (1) {
///         auto result = TaskNotification::wait(INFINITE);
///         if (result.is_ok()) {
///             core::u32 flags = result.unwrap();
///             if (flags & 0x01) {
///                 // Handle event 1
///             }
///             if (flags & 0x02) {
///                 // Handle event 2
///             }
///         }
///     }
/// }
///
/// // Sender task or ISR
/// TaskNotification::notify(receiver_tcb, 0x01, NotifyAction::SetBits);
/// ```
class TaskNotification {
public:
    /// Notify a task
    ///
    /// Sends a notification to the specified task.
    /// Can be called from ISR context (O(1), lock-free).
    ///
    /// @param tcb Target task control block
    /// @param value Notification value
    /// @param action Notification action (SetBits, Increment, Overwrite, OverwriteIfEmpty)
    /// @return Ok(previous_value) on success, Err(RTOSError) on failure
    ///
    /// Example:
    /// ```cpp
    /// // Set bit 0 in notification flags
    /// TaskNotification::notify(task, 0x01, NotifyAction::SetBits);
    ///
    /// // Increment counter
    /// TaskNotification::notify(task, 1, NotifyAction::Increment);
    ///
    /// // Send data value
    /// TaskNotification::notify(task, 0x1234, NotifyAction::Overwrite);
    /// ```
    [[nodiscard]] static core::Result<core::u32, RTOSError> notify(
        TaskControlBlock* tcb,
        core::u32 value,
        NotifyAction action
    ) noexcept;

    /// Notify a task from ISR
    ///
    /// ISR-safe version of notify().
    /// Must be called with interrupts disabled or from ISR context.
    ///
    /// @param tcb Target task control block
    /// @param value Notification value
    /// @param action Notification action
    /// @return Ok(previous_value) on success, Err(RTOSError) on failure
    [[nodiscard]] static core::Result<core::u32, RTOSError> notify_from_isr(
        TaskControlBlock* tcb,
        core::u32 value,
        NotifyAction action
    ) noexcept;

    /// Wait for notification
    ///
    /// Blocks the current task until a notification is received.
    /// Returns the notification value.
    ///
    /// @param timeout_ms Timeout in milliseconds (INFINITE for no timeout)
    /// @param clear_mode How to clear notification (ClearOnEntry, ClearOnExit, NoClear)
    /// @return Ok(notification_value) on success, Err(Timeout) on timeout
    ///
    /// Example:
    /// ```cpp
    /// // Wait forever
    /// auto result = TaskNotification::wait(INFINITE);
    ///
    /// // Wait with timeout
    /// auto result = TaskNotification::wait(1000);  // 1 second
    ///
    /// // Wait and clear on exit
    /// auto result = TaskNotification::wait(INFINITE, NotifyClearMode::ClearOnExit);
    /// ```
    [[nodiscard]] static core::Result<core::u32, RTOSError> wait(
        core::u32 timeout_ms,
        NotifyClearMode clear_mode = NotifyClearMode::ClearOnExit
    );

    /// Try to receive notification (non-blocking)
    ///
    /// Checks if a notification is pending without blocking.
    /// Returns immediately.
    ///
    /// @param clear_mode How to clear notification
    /// @return Ok(notification_value) if available, Err(NotAvailable) otherwise
    ///
    /// Example:
    /// ```cpp
    /// auto result = TaskNotification::try_wait();
    /// if (result.is_ok()) {
    ///     // Process notification
    /// } else {
    ///     // No notification available
    /// }
    /// ```
    [[nodiscard]] static core::Result<core::u32, RTOSError> try_wait(
        NotifyClearMode clear_mode = NotifyClearMode::ClearOnExit
    );

    /// Clear notification value
    ///
    /// Manually clear the notification value and pending count.
    /// Useful when using NotifyClearMode::NoClear.
    ///
    /// @return Ok(previous_value) on success
    ///
    /// Example:
    /// ```cpp
    /// // Wait without auto-clear
    /// auto result = TaskNotification::wait(INFINITE, NotifyClearMode::NoClear);
    /// core::u32 flags = result.unwrap();
    ///
    /// // Process flags...
    ///
    /// // Manually clear
    /// TaskNotification::clear();
    /// ```
    [[nodiscard]] static core::Result<core::u32, RTOSError> clear();

    /// Get notification value without clearing
    ///
    /// Peek at the notification value without consuming it.
    ///
    /// @return Current notification value
    [[nodiscard]] static core::u32 peek();

    /// Check if notification is pending
    ///
    /// @return true if notification is pending
    [[nodiscard]] static bool is_pending();

private:
    /// Get notification state for current task
    static TaskNotificationState* get_current_state();

    /// Get notification state for specific task
    static TaskNotificationState* get_state(TaskControlBlock* tcb);

    /// Internal notify implementation
    [[nodiscard]] static core::Result<core::u32, RTOSError> notify_internal(
        TaskControlBlock* tcb,
        core::u32 value,
        NotifyAction action,
        bool from_isr
    ) noexcept;
};

// ============================================================================
// C++23 Concepts for Task Notifications
// ============================================================================

/// Concept: NotificationReceiver<T>
///
/// Validates that a task can receive notifications.
///
/// @tparam T Task type
template <typename T>
concept NotificationReceiver = requires(T task) {
    { task.get_tcb() } -> std::convertible_to<TaskControlBlock*>;
};

/// Concept: NotificationSender<F>
///
/// Validates that a function can be used for notification callbacks.
///
/// @tparam F Function type
template <typename F>
concept NotificationSender = requires(F f, TaskControlBlock* tcb, core::u32 value) {
    { f(tcb, value) } -> std::same_as<core::Result<void, RTOSError>>;
};

// ============================================================================
// Compile-Time Validation
// ============================================================================

/// Validate notification value fits in 32 bits
///
/// @tparam Value Notification value
/// @return true if valid
template <core::u32 Value>
consteval bool is_valid_notification_value() {
    return true;  // All u32 values are valid
}

/// Validate notification flags (for bit operations)
///
/// @tparam Flags Bit flags
/// @return true if valid (non-zero recommended)
template <core::u32 Flags>
consteval bool is_valid_notification_flags() {
    return Flags != 0;  // Zero flags are allowed but not recommended
}

/// Calculate notification overhead per task
///
/// @return Bytes of overhead per task
consteval size_t notification_overhead_per_task() {
    return sizeof(TaskNotificationState);  // 8 bytes
}

/// Validate notification count for budget
///
/// @tparam TaskCount Number of tasks
/// @tparam Budget RAM budget in bytes
/// @return true if fits in budget
template <size_t TaskCount, size_t Budget>
consteval bool notification_memory_fits_budget() {
    constexpr size_t total = TaskCount * notification_overhead_per_task();
    return total <= Budget;
}

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_TASK_NOTIFICATION_HPP
