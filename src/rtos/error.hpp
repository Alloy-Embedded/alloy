/// Alloy RTOS Error Codes
///
/// Defines RTOS-specific error codes for consistent error handling.
/// All RTOS APIs return Result<T, RTOSError> for type-safe error handling
/// consistent with the HAL layer.
///
/// Usage:
/// ```cpp
/// auto result = mutex.lock(1000);
/// if (result.is_ok()) {
///     // Success - mutex locked
///     access_shared_resource();
///     mutex.unlock();
/// } else {
///     // Handle error
///     RTOSError error = result.unwrap_err();
///     if (error == RTOSError::Timeout) {
///         // Timeout - couldn't get mutex in 1 second
///     }
/// }
/// ```

#ifndef ALLOY_RTOS_ERROR_HPP
#define ALLOY_RTOS_ERROR_HPP

#include "core/types.hpp"

namespace alloy::rtos {

/// RTOS-specific error codes
///
/// Each error represents a specific failure condition that can occur during
/// RTOS operations. These errors are used with Result<T, RTOSError> for
/// type-safe error handling.
///
/// Error categories:
/// - Synchronization errors (Timeout, NotOwner, Deadlock)
/// - IPC errors (QueueFull, QueueEmpty)
/// - Task errors (InvalidPriority, StackOverflow)
/// - Memory errors (NoMemory, InvalidPointer)
enum class RTOSError : core::u8 {
    // ========================================================================
    // Synchronization Errors
    // ========================================================================

    /// Operation timed out waiting for resource
    ///
    /// Example: Waiting for mutex lock exceeded timeout.
    /// ```cpp
    /// auto result = mutex.lock(1000);
    /// if (result.unwrap_err() == RTOSError::Timeout) {
    ///     // Couldn't get mutex in 1 second
    /// }
    /// ```
    Timeout = 1,

    /// Task does not own the resource
    ///
    /// Example: Attempting to unlock a mutex not owned by current task.
    /// ```cpp
    /// auto result = mutex.unlock();
    /// if (result.unwrap_err() == RTOSError::NotOwner) {
    ///     // Only the task that locked the mutex can unlock it
    /// }
    /// ```
    NotOwner,

    /// Potential deadlock detected
    ///
    /// Example: Priority inheritance detected circular dependency.
    /// Note: This is a runtime detection, not compile-time.
    Deadlock,

    // ========================================================================
    // IPC Errors (Queue, Semaphore, Event)
    // ========================================================================

    /// Queue is full, cannot send message
    ///
    /// Example: Sending to queue exceeded timeout because queue was full.
    /// ```cpp
    /// auto result = queue.send(data, 100);
    /// if (result.unwrap_err() == RTOSError::QueueFull) {
    ///     // Queue full after 100ms timeout
    /// }
    /// ```
    QueueFull,

    /// Queue is empty, cannot receive message
    ///
    /// Example: Receiving from queue exceeded timeout because queue was empty.
    /// ```cpp
    /// auto result = queue.receive(100);
    /// if (result.unwrap_err() == RTOSError::QueueEmpty) {
    ///     // No messages after 100ms timeout
    /// }
    /// ```
    QueueEmpty,

    // ========================================================================
    // Task Errors
    // ========================================================================

    /// Invalid task priority specified
    ///
    /// Example: Attempting to create task with priority > 7.
    /// Note: With compile-time validation, this should be rare at runtime.
    InvalidPriority,

    /// Stack overflow detected
    ///
    /// Example: Task stack exceeded allocated size.
    /// ```cpp
    /// if (task.check_stack_overflow() == RTOSError::StackOverflow) {
    ///     // Stack overflow detected - increase stack size
    /// }
    /// ```
    StackOverflow,

    /// Task is not in expected state for operation
    ///
    /// Example: Attempting to resume a task that's not suspended.
    InvalidState,

    // ========================================================================
    // Memory Errors (Memory Pools)
    // ========================================================================

    /// No memory available in pool
    ///
    /// Example: StaticPool allocation failed because pool is exhausted.
    /// ```cpp
    /// auto result = pool.allocate(args...);
    /// if (result.unwrap_err() == RTOSError::NoMemory) {
    ///     // Pool exhausted - all blocks allocated
    /// }
    /// ```
    NoMemory,

    /// Invalid pointer passed to pool deallocation
    ///
    /// Example: Attempting to deallocate pointer not from pool.
    /// ```cpp
    /// auto result = pool.deallocate(ptr);
    /// if (result.unwrap_err() == RTOSError::InvalidPointer) {
    ///     // Pointer not from this pool
    /// }
    /// ```
    InvalidPointer,

    // ========================================================================
    // Scheduler Errors
    // ========================================================================

    /// Scheduler tick error
    ///
    /// Example: RTOS tick encountered error during delayed task wake.
    /// This is rare and indicates a serious issue.
    TickError,

    /// Context switch error
    ///
    /// Example: PendSV handler encountered error during context switch.
    /// This is rare and indicates a serious issue.
    ContextSwitchError,

    // ========================================================================
    // General Errors
    // ========================================================================

    /// RTOS not initialized
    ///
    /// Example: Attempting to use RTOS APIs before RTOS::start().
    NotInitialized,

    /// Unknown error occurred
    ///
    /// This should be rare - most errors should have specific codes.
    Unknown
};

/// Convert RTOSError to human-readable string
///
/// Useful for logging and debugging.
///
/// @param error The error code to convert
/// @return String representation of the error
///
/// Example:
/// ```cpp
/// auto result = mutex.lock(1000);
/// if (result.is_err()) {
///     const char* error_str = to_string(result.unwrap_err());
///     uart_send(error_str);  // e.g., "Timeout"
/// }
/// ```
constexpr const char* to_string(RTOSError error) {
    switch (error) {
        case RTOSError::Timeout:
            return "Timeout";
        case RTOSError::NotOwner:
            return "NotOwner";
        case RTOSError::Deadlock:
            return "Deadlock";
        case RTOSError::QueueFull:
            return "QueueFull";
        case RTOSError::QueueEmpty:
            return "QueueEmpty";
        case RTOSError::InvalidPriority:
            return "InvalidPriority";
        case RTOSError::StackOverflow:
            return "StackOverflow";
        case RTOSError::InvalidState:
            return "InvalidState";
        case RTOSError::NoMemory:
            return "NoMemory";
        case RTOSError::InvalidPointer:
            return "InvalidPointer";
        case RTOSError::TickError:
            return "TickError";
        case RTOSError::ContextSwitchError:
            return "ContextSwitchError";
        case RTOSError::NotInitialized:
            return "NotInitialized";
        case RTOSError::Unknown:
            return "Unknown";
        default:
            return "UnknownError";
    }
}

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_ERROR_HPP
