/// Alloy RTOS - Task Notification Implementation
///
/// Implementation of lightweight task notification mechanism.

#include "rtos/task_notification.hpp"
#include "rtos/scheduler.hpp"

namespace alloy::rtos {

// ============================================================================
// Helper Functions
// ============================================================================

/// Get notification state for current task
TaskNotificationState* TaskNotification::get_current_state() {
    TaskControlBlock* current = RTOS::current_task();
    if (current == nullptr) {
        return nullptr;
    }
    return get_state(current);
}

/// Get notification state for specific task
TaskNotificationState* TaskNotification::get_state(TaskControlBlock* tcb) {
    if (tcb == nullptr) {
        return nullptr;
    }

    // Notification state is stored after TCB in memory
    // Layout: [TCB][TaskNotificationState]
    // This is allocated when task is created
    auto* state = reinterpret_cast<TaskNotificationState*>(
        reinterpret_cast<core::u8*>(tcb) + sizeof(TaskControlBlock)
    );

    return state;
}

// ============================================================================
// Notification API Implementation
// ============================================================================

core::Result<core::u32, RTOSError> TaskNotification::notify(
    TaskControlBlock* tcb,
    core::u32 value,
    NotifyAction action
) noexcept {
    return notify_internal(tcb, value, action, false);
}

core::Result<core::u32, RTOSError> TaskNotification::notify_from_isr(
    TaskControlBlock* tcb,
    core::u32 value,
    NotifyAction action
) noexcept {
    return notify_internal(tcb, value, action, true);
}

core::Result<core::u32, RTOSError> TaskNotification::notify_internal(
    TaskControlBlock* tcb,
    core::u32 value,
    NotifyAction action,
    bool from_isr
) noexcept {
    // Validate task
    if (tcb == nullptr) {
        return core::Err(RTOSError::InvalidPointer);
    }

    // Get notification state
    TaskNotificationState* state = get_state(tcb);
    if (state == nullptr) {
        return core::Err(RTOSError::InvalidState);
    }

    // Get previous value (before modification)
    core::u32 previous_value = state->notification_value.load(std::memory_order_acquire);

    // Apply notification action
    core::u32 new_value = 0;
    switch (action) {
        case NotifyAction::SetBits:
            // OR the value with existing bits
            new_value = previous_value | value;
            state->notification_value.store(new_value, std::memory_order_release);
            break;

        case NotifyAction::Increment:
            // Increment (add value)
            new_value = previous_value + value;
            state->notification_value.store(new_value, std::memory_order_release);
            break;

        case NotifyAction::Overwrite:
            // Overwrite with new value
            state->notification_value.store(value, std::memory_order_release);
            break;

        case NotifyAction::OverwriteIfEmpty:
            // Only overwrite if no pending notification
            if (state->pending_count.load(std::memory_order_acquire) == 0) {
                state->notification_value.store(value, std::memory_order_release);
            } else {
                // Notification already pending
                return core::Err(RTOSError::QueueFull);  // Reuse QueueFull error
            }
            break;

        default:
            return core::Err(RTOSError::InvalidState);
    }

    // Increment pending count
    state->pending_count.fetch_add(1, std::memory_order_acq_rel);

    // Wake task if it's waiting on notification
    if (tcb->state == TaskState::Blocked) {
        // Change state to ready
        tcb->state = TaskState::Ready;

        // Trigger context switch if needed (unless from ISR - will happen on ISR exit)
        if (!from_isr && RTOS::need_context_switch()) {
            RTOS::yield();
        }
    }

    return core::Ok(previous_value);
}

core::Result<core::u32, RTOSError> TaskNotification::wait(
    core::u32 timeout_ms,
    NotifyClearMode clear_mode
) {
    // Get current task notification state
    TaskNotificationState* state = get_current_state();
    if (state == nullptr) {
        return core::Err(RTOSError::NotInitialized);
    }

    // Clear on entry if requested
    if (clear_mode == NotifyClearMode::ClearOnEntry) {
        state->notification_value.store(0, std::memory_order_relaxed);
        state->pending_count.store(0, std::memory_order_relaxed);
    }

    // Check if notification is already pending
    if (state->pending_count.load(std::memory_order_acquire) > 0) {
        // Notification available
        core::u32 value = state->notification_value.load(std::memory_order_acquire);

        // Decrement pending count
        state->pending_count.fetch_sub(1, std::memory_order_acq_rel);

        // Clear on exit if requested
        if (clear_mode == NotifyClearMode::ClearOnExit) {
            state->notification_value.store(0, std::memory_order_release);
        }

        return core::Ok(value);
    }

    // No notification available - need to block
    TaskControlBlock* current = RTOS::current_task();
    if (current == nullptr) {
        return core::Err(RTOSError::NotInitialized);
    }

    // Set timeout if specified
    if (timeout_ms != INFINITE) {
        current->wake_time = hal::SysTick::micros() + (timeout_ms * 1000);
    }

    // Block task
    current->state = TaskState::Blocked;

    // Yield to other tasks
    RTOS::yield();

    // When we get here, either:
    // 1. Notification was received (state changed to Ready by notify())
    // 2. Timeout occurred (state changed to Ready by scheduler)

    // Check if we have a notification
    if (state->pending_count.load(std::memory_order_acquire) > 0) {
        core::u32 value = state->notification_value.load(std::memory_order_acquire);

        // Decrement pending count
        state->pending_count.fetch_sub(1, std::memory_order_acq_rel);

        // Clear on exit if requested
        if (clear_mode == NotifyClearMode::ClearOnExit) {
            state->notification_value.store(0, std::memory_order_release);
        }

        return core::Ok(value);
    }

    // Timeout occurred
    return core::Err(RTOSError::Timeout);
}

core::Result<core::u32, RTOSError> TaskNotification::try_wait(
    NotifyClearMode clear_mode
) {
    // Get current task notification state
    TaskNotificationState* state = get_current_state();
    if (state == nullptr) {
        return core::Err(RTOSError::NotInitialized);
    }

    // Check if notification is pending
    if (state->pending_count.load(std::memory_order_acquire) > 0) {
        core::u32 value = state->notification_value.load(std::memory_order_acquire);

        // Decrement pending count
        state->pending_count.fetch_sub(1, std::memory_order_acq_rel);

        // Clear based on mode
        if (clear_mode == NotifyClearMode::ClearOnEntry ||
            clear_mode == NotifyClearMode::ClearOnExit) {
            state->notification_value.store(0, std::memory_order_release);
        }

        return core::Ok(value);
    }

    // No notification available
    return core::Err(RTOSError::QueueEmpty);  // Reuse QueueEmpty error
}

core::Result<core::u32, RTOSError> TaskNotification::clear() {
    // Get current task notification state
    TaskNotificationState* state = get_current_state();
    if (state == nullptr) {
        return core::Err(RTOSError::NotInitialized);
    }

    // Get previous value
    core::u32 previous_value = state->notification_value.load(std::memory_order_acquire);

    // Clear value and count
    state->notification_value.store(0, std::memory_order_release);
    state->pending_count.store(0, std::memory_order_release);

    return core::Ok(previous_value);
}

core::u32 TaskNotification::peek() {
    TaskNotificationState* state = get_current_state();
    if (state == nullptr) {
        return 0;
    }

    return state->notification_value.load(std::memory_order_acquire);
}

bool TaskNotification::is_pending() {
    TaskNotificationState* state = get_current_state();
    if (state == nullptr) {
        return false;
    }

    return state->pending_count.load(std::memory_order_acquire) > 0;
}

}  // namespace alloy::rtos
