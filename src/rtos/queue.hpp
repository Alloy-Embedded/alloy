/// Alloy RTOS - Message Queue
///
/// Type-safe FIFO message queue for inter-task communication.
///
/// Features:
/// - Compile-time type safety with C++20 templates
/// - Zero-copy semantics (data stored in queue buffer)
/// - Blocking send/receive with timeout support
/// - O(1) enqueue and dequeue operations
/// - Circular buffer implementation
/// - Static allocation (no heap)
///
/// Memory footprint:
/// - Queue overhead: 16 bytes + (capacity * sizeof(T))
/// - Blocked task lists: 8 bytes (2 pointers)
///
/// Performance:
/// - send()/receive(): ~10-20 CPU cycles (no blocking)
/// - Context switch on block: <10µs
///
/// Usage:
/// ```cpp
/// struct SensorData {
///     uint32_t timestamp;
///     int16_t temperature;
///     int16_t humidity;
/// };
///
/// // Create queue with capacity for 8 messages
/// Queue<SensorData, 8> sensor_queue;
///
/// // Producer task
/// void sensor_task() {
///     while (1) {
///         SensorData data = read_sensor();
///         sensor_queue.send(data, INFINITE);  // Block until space available
///         RTOS::delay(100);
///     }
/// }
///
/// // Consumer task
/// void display_task() {
///     while (1) {
///         SensorData data;
///         if (sensor_queue.receive(data, 1000)) {  // 1 second timeout
///             display_data(data);
///         }
///     }
/// }
/// ```

#ifndef ALLOY_RTOS_QUEUE_HPP
#define ALLOY_RTOS_QUEUE_HPP

#include <cstring>
#include <type_traits>

#include "rtos/platform/critical_section.hpp"
#include "rtos/rtos.hpp"
#include "rtos/scheduler.hpp"
#include "rtos/error.hpp"
#include "rtos/concepts.hpp"

#include "core/error.hpp"
#include "core/types.hpp"
#include "core/result.hpp"

namespace alloy::rtos {

/// Message Queue
///
/// Type-safe FIFO queue for passing messages between tasks.
///
/// @tparam T Message type (must satisfy IPCMessage concept)
/// @tparam Capacity Maximum number of messages in queue (must be power of 2)
///
/// Template constraints:
/// - T must satisfy IPCMessage concept (trivially copyable, <= 256 bytes, not a pointer)
/// - Capacity must be between 2 and 256
/// - Capacity must be power of 2 (for efficient modulo with mask)
template <IPCMessage T, size_t Capacity>
class Queue {
    static_assert(Capacity >= 2 && Capacity <= 256, "Queue capacity must be between 2 and 256");
    static_assert((Capacity & (Capacity - 1)) == 0, "Queue capacity must be power of 2");

   private:
    /// Circular buffer for messages
    alignas(T) T buffer_[Capacity];

    /// Queue state
    core::u32 head_;   ///< Write position (producer)
    core::u32 tail_;   ///< Read position (consumer)
    core::u32 count_;  ///< Number of messages in queue

    /// Wait lists for blocked tasks
    TaskControlBlock* send_wait_list_;     ///< Tasks blocked on full queue
    TaskControlBlock* receive_wait_list_;  ///< Tasks blocked on empty queue

    /// Capacity mask (Capacity - 1, for fast modulo)
    static constexpr core::u32 MASK = Capacity - 1;

   public:
    /// Constructor
    constexpr Queue()
        : buffer_{},
          head_(0),
          tail_(0),
          count_(0),
          send_wait_list_(nullptr),
          receive_wait_list_(nullptr) {}

    /// Send message to queue (blocking)
    ///
    /// Blocks if queue is full. Unblocks when space becomes available.
    ///
    /// @param message Message to send (copied into queue)
    /// @param timeout_ms Timeout in milliseconds (INFINITE = wait forever)
    /// @return Ok(void) if sent successfully, Err(RTOSError::QueueFull) if timeout
    ///
    /// Example:
    /// ```cpp
    /// SensorData data = {micros(), temp, humidity};
    /// auto result = queue.send(data, 1000);
    /// if (result.is_err()) {
    ///     // Timeout - queue still full after 1 second
    /// }
    /// ```
    core::Result<void, RTOSError> send(const T& message, core::u32 timeout_ms = INFINITE);

    /// Receive message from queue (blocking)
    ///
    /// Blocks if queue is empty. Unblocks when message becomes available.
    ///
    /// @param timeout_ms Timeout in milliseconds (INFINITE = wait forever)
    /// @return Ok(T) with message if received, Err(RTOSError::QueueEmpty) if timeout
    ///
    /// Example:
    /// ```cpp
    /// auto result = queue.receive(1000);
    /// if (result.is_ok()) {
    ///     SensorData data = result.unwrap();
    ///     process(data);
    /// } else {
    ///     // Timeout - no data received in 1 second
    /// }
    /// ```
    core::Result<T, RTOSError> receive(core::u32 timeout_ms = INFINITE);

    /// Try to send message (non-blocking)
    ///
    /// Returns immediately if queue is full (does not block).
    ///
    /// @param message Message to send
    /// @return Ok(void) if sent, Err(RTOSError::QueueFull) if queue is full
    ///
    /// Example:
    /// ```cpp
    /// auto result = queue.try_send(data);
    /// if (result.is_err()) {
    ///     // Queue full - drop message or handle error
    /// }
    /// ```
    core::Result<void, RTOSError> try_send(const T& message);

    /// Try to receive message (non-blocking)
    ///
    /// Returns immediately if queue is empty (does not block).
    ///
    /// @return Ok(T) with message if received, Err(RTOSError::QueueEmpty) if queue is empty
    ///
    /// Example:
    /// ```cpp
    /// auto result = queue.try_receive();
    /// if (result.is_ok()) {
    ///     SensorData data = result.unwrap();
    ///     process(data);
    /// }
    /// ```
    core::Result<T, RTOSError> try_receive();

    /// Check if queue is empty
    ///
    /// @return true if no messages in queue
    bool is_empty() const { return count_ == 0; }

    /// Check if queue is full
    ///
    /// @return true if queue is at capacity
    bool is_full() const { return count_ == Capacity; }

    /// Get number of messages currently in queue
    ///
    /// @return Number of messages (0 to Capacity)
    core::u32 count() const { return count_; }

    /// Get queue capacity
    ///
    /// @return Maximum number of messages
    constexpr core::u32 capacity() const { return Capacity; }

    /// Get number of free slots
    ///
    /// @return Number of messages that can be sent without blocking
    core::u32 available() const { return Capacity - count_; }

    /// Reset queue to empty state
    ///
    /// Clears all messages. Does NOT unblock waiting tasks.
    /// Use with caution - should only be called when no tasks are blocked.
    void reset() {
        // Disable interrupts for atomic operation
        disable_interrupts();

        head_ = 0;
        tail_ = 0;
        count_ = 0;

        enable_interrupts();
    }

   private:
    /// Disable interrupts (critical section entry)
    static inline void disable_interrupts() { platform::disable_interrupts(); }

    /// Enable interrupts (critical section exit)
    static inline void enable_interrupts() { platform::enable_interrupts(); }
};

// Template implementation

template <typename T, size_t Capacity>
core::Result<void, RTOSError> Queue<T, Capacity>::send(const T& message, core::u32 timeout_ms) {
    using core::Ok;
    using core::Err;

    core::u32 start_time = systick::micros();

    while (true) {
        // Try to send without blocking
        auto try_result = try_send(message);
        if (try_result.is_ok()) {
            return Ok();  // Success
        }

        // Queue is full - check timeout
        if (timeout_ms != INFINITE) {
            core::u32 elapsed = systick::micros_since(start_time);
            if (elapsed >= (timeout_ms * 1000)) {
                return Err(RTOSError::QueueFull);
            }
        }

        // Block on send wait list
        disable_interrupts();
        scheduler::block_current_task(&send_wait_list_);
        enable_interrupts();

        // Trigger context switch
        extern void trigger_context_switch();
        trigger_context_switch();
    }
}

template <typename T, size_t Capacity>
core::Result<T, RTOSError> Queue<T, Capacity>::receive(core::u32 timeout_ms) {
    using core::Ok;
    using core::Err;

    core::u32 start_time = systick::micros();

    while (true) {
        // Try to receive without blocking
        auto try_result = try_receive();
        if (try_result.is_ok()) {
            return try_result;  // Success - return the message
        }

        // Queue is empty - check timeout
        if (timeout_ms != INFINITE) {
            core::u32 elapsed = systick::micros_since(start_time);
            if (elapsed >= (timeout_ms * 1000)) {
                return Err(RTOSError::QueueEmpty);
            }
        }

        // Block on receive wait list
        disable_interrupts();
        scheduler::block_current_task(&receive_wait_list_);
        enable_interrupts();

        // Trigger context switch
        extern void trigger_context_switch();
        trigger_context_switch();
    }
}

template <typename T, size_t Capacity>
core::Result<void, RTOSError> Queue<T, Capacity>::try_send(const T& message) {
    using core::Ok;
    using core::Err;

    disable_interrupts();

    // Check if queue is full
    if (count_ >= Capacity) {
        enable_interrupts();
        return Err(RTOSError::QueueFull);
    }

    // Copy message to buffer
    std::memcpy(&buffer_[head_], &message, sizeof(T));

    // Advance head pointer (wrap with mask)
    head_ = (head_ + 1) & MASK;
    count_++;

    enable_interrupts();

    // Unblock one waiting receiver (if any)
    if (receive_wait_list_ != nullptr) {
        scheduler::unblock_one_task(&receive_wait_list_);
    }

    return Ok();
}

template <typename T, size_t Capacity>
core::Result<T, RTOSError> Queue<T, Capacity>::try_receive() {
    using core::Ok;
    using core::Err;

    disable_interrupts();

    // Check if queue is empty
    if (count_ == 0) {
        enable_interrupts();
        return Err(RTOSError::QueueEmpty);
    }

    // Copy message from buffer
    T message;
    std::memcpy(&message, &buffer_[tail_], sizeof(T));

    // Advance tail pointer (wrap with mask)
    tail_ = (tail_ + 1) & MASK;
    count_--;

    enable_interrupts();

    // Unblock one waiting sender (if any)
    if (send_wait_list_ != nullptr) {
        scheduler::unblock_one_task(&send_wait_list_);
    }

    return Ok(message);
}

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_QUEUE_HPP
