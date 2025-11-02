/// Alloy RTOS - Semaphores
///
/// Binary and counting semaphores for task synchronization.
///
/// Features:
/// - Binary semaphores (0 or 1 token)
/// - Counting semaphores (0 to MaxCount tokens)
/// - Blocking take/give with timeout support
/// - Thread-safe operations
/// - Static allocation (no heap)
///
/// Use cases:
/// - Binary: Signal events between tasks (ISR → Task)
/// - Counting: Resource pools, rate limiting
///
/// Memory footprint:
/// - BinarySemaphore: 12 bytes
/// - CountingSemaphore: 16 bytes
///
/// Usage:
/// ```cpp
/// // Binary semaphore for ISR → Task signaling
/// BinarySemaphore data_ready;
///
/// void UART_IRQHandler() {
///     // ... read data ...
///     data_ready.give();  // Signal task
/// }
///
/// void uart_task() {
///     while (1) {
///         if (data_ready.take(1000)) {  // Wait up to 1 second
///             process_uart_data();
///         }
///     }
/// }
///
/// // Counting semaphore for resource pool
/// CountingSemaphore<5> buffer_pool(5);  // 5 buffers initially available
///
/// void producer() {
///     if (buffer_pool.take(100)) {  // Get buffer
///         fill_buffer();
///         buffer_pool.give();       // Return buffer
///     }
/// }
/// ```

#ifndef ALLOY_RTOS_SEMAPHORE_HPP
#define ALLOY_RTOS_SEMAPHORE_HPP

#include "rtos/rtos.hpp"
#include "rtos/scheduler.hpp"
#include "core/types.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::rtos {

/// Binary Semaphore
///
/// A synchronization primitive with two states: available (1) or taken (0).
/// Used for signaling between tasks, especially from ISR to task.
///
/// Key properties:
/// - Value is always 0 or 1
/// - give() called multiple times still results in count = 1
/// - Ideal for event notifications
/// - Safe to give() from ISR
///
/// Example: ISR signals data ready
/// ```cpp
/// BinarySemaphore data_ready;
///
/// void ADC_IRQHandler() {
///     uint16_t sample = ADC->DR;
///     save_sample(sample);
///     data_ready.give();  // Wake up processing task
/// }
///
/// void adc_task() {
///     while (1) {
///         data_ready.take(INFINITE);  // Block until data ready
///         process_samples();
///     }
/// }
/// ```
class BinarySemaphore {
private:
    core::u8 count_;                     ///< 0 or 1
    TaskControlBlock* wait_list_;        ///< Tasks blocked on take()

public:
    /// Constructor
    ///
    /// @param initial_value Initial count (0 = taken, 1 = available)
    explicit BinarySemaphore(core::u8 initial_value = 0)
        : count_(initial_value > 0 ? 1 : 0)
        , wait_list_(nullptr)
    {}

    /// Give semaphore (increment count to 1)
    ///
    /// If count is already 1, this has no effect (binary property).
    /// Unblocks one waiting task if any are blocked.
    ///
    /// Safe to call from ISR.
    ///
    /// Example:
    /// ```cpp
    /// data_ready.give();  // Signal from ISR or task
    /// ```
    void give();

    /// Take semaphore (decrement count, block if 0)
    ///
    /// If count is 1, decrements to 0 and returns immediately.
    /// If count is 0, blocks until give() is called or timeout.
    ///
    /// @param timeout_ms Timeout in milliseconds (INFINITE = wait forever)
    /// @return true if semaphore taken, false if timeout
    ///
    /// Example:
    /// ```cpp
    /// if (data_ready.take(1000)) {
    ///     // Got semaphore within 1 second
    ///     process_data();
    /// } else {
    ///     // Timeout
    /// }
    /// ```
    bool take(core::u32 timeout_ms = INFINITE);

    /// Try to take semaphore (non-blocking)
    ///
    /// Returns immediately regardless of count.
    ///
    /// @return true if semaphore was taken, false if count was 0
    ///
    /// Example:
    /// ```cpp
    /// if (data_ready.try_take()) {
    ///     process_data();  // Data available
    /// }
    /// // Continue immediately either way
    /// ```
    bool try_take();

    /// Get current count (0 or 1)
    ///
    /// @return 0 if taken, 1 if available
    core::u8 count() const {
        return count_;
    }

    /// Check if available
    ///
    /// @return true if count is 1
    bool is_available() const {
        return count_ > 0;
    }

private:
    /// Disable interrupts (critical section)
    static inline void disable_interrupts() {
#if defined(__ARM_ARCH)
        __asm volatile("cpsid i" ::: "memory");
#elif defined(ESP32) || defined(ESP_PLATFORM)
        __asm volatile("rsil a15, 15" ::: "memory");
#endif
    }

    /// Enable interrupts
    static inline void enable_interrupts() {
#if defined(__ARM_ARCH)
        __asm volatile("cpsie i" ::: "memory");
#elif defined(ESP32) || defined(ESP_PLATFORM)
        __asm volatile("rsil a15, 0" ::: "memory");
#endif
    }
};

/// Counting Semaphore
///
/// A synchronization primitive with count from 0 to MaxCount.
/// Used for resource pools and rate limiting.
///
/// Key properties:
/// - Count can be 0 to MaxCount
/// - Each give() increments count (up to MaxCount)
/// - Each take() decrements count (blocking if 0)
/// - Ideal for managing pools of resources
///
/// @tparam MaxCount Maximum count value (1-255)
///
/// Example: Buffer pool management
/// ```cpp
/// CountingSemaphore<10> buffers(10);  // 10 buffers initially
///
/// void producer() {
///     while (1) {
///         if (buffers.take(100)) {        // Get buffer
///             Buffer* buf = alloc_buffer();
///             fill_buffer(buf);
///             queue.send(buf);
///             // Don't give() here - consumer will
///         }
///     }
/// }
///
/// void consumer() {
///     while (1) {
///         Buffer* buf = queue.receive();
///         process_buffer(buf);
///         free_buffer(buf);
///         buffers.give();  // Return buffer to pool
///     }
/// }
/// ```
template<core::u8 MaxCount>
class CountingSemaphore {
    static_assert(MaxCount > 0 && MaxCount <= 255,
                  "MaxCount must be between 1 and 255");

private:
    core::u8 count_;                     ///< Current count (0 to MaxCount)
    core::u8 max_count_;                 ///< Maximum count
    TaskControlBlock* wait_list_;        ///< Tasks blocked on take()

public:
    /// Constructor
    ///
    /// @param initial_count Initial count (0 to MaxCount)
    explicit CountingSemaphore(core::u8 initial_count = 0)
        : count_(initial_count > MaxCount ? MaxCount : initial_count)
        , max_count_(MaxCount)
        , wait_list_(nullptr)
    {}

    /// Give semaphore (increment count)
    ///
    /// Increments count by 1 (up to MaxCount).
    /// If already at MaxCount, this has no effect.
    /// Unblocks one waiting task if any are blocked.
    ///
    /// Safe to call from ISR.
    ///
    /// Example:
    /// ```cpp
    /// buffers.give();  // Return resource to pool
    /// ```
    void give();

    /// Take semaphore (decrement count, block if 0)
    ///
    /// If count > 0, decrements and returns immediately.
    /// If count is 0, blocks until give() is called or timeout.
    ///
    /// @param timeout_ms Timeout in milliseconds (INFINITE = wait forever)
    /// @return true if semaphore taken, false if timeout
    ///
    /// Example:
    /// ```cpp
    /// if (buffers.take(1000)) {
    ///     // Got resource
    ///     use_resource();
    ///     buffers.give();  // Return when done
    /// }
    /// ```
    bool take(core::u32 timeout_ms = INFINITE);

    /// Try to take semaphore (non-blocking)
    ///
    /// Returns immediately regardless of count.
    ///
    /// @return true if semaphore was taken, false if count was 0
    ///
    /// Example:
    /// ```cpp
    /// if (buffers.try_take()) {
    ///     use_resource();
    ///     buffers.give();
    /// }
    /// ```
    bool try_take();

    /// Get current count
    ///
    /// @return Current count (0 to MaxCount)
    core::u8 count() const {
        return count_;
    }

    /// Get maximum count
    ///
    /// @return MaxCount
    core::u8 max_count() const {
        return max_count_;
    }

    /// Check if available
    ///
    /// @return true if count > 0
    bool is_available() const {
        return count_ > 0;
    }

private:
    /// Disable interrupts (critical section)
    static inline void disable_interrupts() {
#if defined(__ARM_ARCH)
        __asm volatile("cpsid i" ::: "memory");
#elif defined(ESP32) || defined(ESP_PLATFORM)
        __asm volatile("rsil a15, 15" ::: "memory");
#endif
    }

    /// Enable interrupts
    static inline void enable_interrupts() {
#if defined(__ARM_ARCH)
        __asm volatile("cpsie i" ::: "memory");
#elif defined(ESP32) || defined(ESP_PLATFORM)
        __asm volatile("rsil a15, 0" ::: "memory");
#endif
    }
};

// BinarySemaphore implementation

inline void BinarySemaphore::give() {
    disable_interrupts();

    // Set count to 1 (binary property - multiple gives don't accumulate)
    count_ = 1;

    enable_interrupts();

    // Unblock one waiting task (if any)
    if (wait_list_ != nullptr) {
        scheduler::unblock_one_task(&wait_list_);
    }
}

inline bool BinarySemaphore::take(core::u32 timeout_ms) {
    core::u32 start_time = systick::micros();

    while (true) {
        // Try to take without blocking
        if (try_take()) {
            return true;  // Success
        }

        // Check timeout
        if (timeout_ms != INFINITE) {
            core::u32 elapsed = systick::micros_since(start_time);
            if (elapsed >= (timeout_ms * 1000)) {
                return false;  // Timeout
            }
        }

        // Block on wait list
        disable_interrupts();
        scheduler::block_current_task(&wait_list_);
        enable_interrupts();

        // Trigger context switch
        extern void trigger_context_switch();
        trigger_context_switch();
    }
}

inline bool BinarySemaphore::try_take() {
    disable_interrupts();

    if (count_ > 0) {
        count_ = 0;
        enable_interrupts();
        return true;
    }

    enable_interrupts();
    return false;
}

// CountingSemaphore implementation

template<core::u8 MaxCount>
void CountingSemaphore<MaxCount>::give() {
    disable_interrupts();

    // Increment count (up to max)
    if (count_ < max_count_) {
        count_++;
    }

    enable_interrupts();

    // Unblock one waiting task (if any)
    if (wait_list_ != nullptr) {
        scheduler::unblock_one_task(&wait_list_);
    }
}

template<core::u8 MaxCount>
bool CountingSemaphore<MaxCount>::take(core::u32 timeout_ms) {
    core::u32 start_time = systick::micros();

    while (true) {
        // Try to take without blocking
        if (try_take()) {
            return true;  // Success
        }

        // Check timeout
        if (timeout_ms != INFINITE) {
            core::u32 elapsed = systick::micros_since(start_time);
            if (elapsed >= (timeout_ms * 1000)) {
                return false;  // Timeout
            }
        }

        // Block on wait list
        disable_interrupts();
        scheduler::block_current_task(&wait_list_);
        enable_interrupts();

        // Trigger context switch
        extern void trigger_context_switch();
        trigger_context_switch();
    }
}

template<core::u8 MaxCount>
bool CountingSemaphore<MaxCount>::try_take() {
    disable_interrupts();

    if (count_ > 0) {
        count_--;
        enable_interrupts();
        return true;
    }

    enable_interrupts();
    return false;
}

} // namespace alloy::rtos

#endif // ALLOY_RTOS_SEMAPHORE_HPP
