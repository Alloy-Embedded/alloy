/// Alloy RTOS - Event Flags
///
/// Lightweight task synchronization using 32-bit event flags.
///
/// Features:
/// - 32 independent event flags (bits 0-31)
/// - wait_any(): Wait for ANY flag to be set
/// - wait_all(): Wait for ALL flags to be set
/// - set(): Set one or more flags
/// - clear(): Clear one or more flags
/// - Blocking waits with timeout support
///
/// Use cases:
/// - Multi-event synchronization (e.g., wait for multiple sensors ready)
/// - State machines (each bit = different state/event)
/// - Lightweight notification (more efficient than multiple semaphores)
///
/// Memory footprint:
/// - EventFlags: 12 bytes
///
/// Usage:
/// ```cpp
/// // Define event flags
/// constexpr uint32_t EVENT_SENSOR_READY  = (1 << 0);
/// constexpr uint32_t EVENT_DATA_VALID    = (1 << 1);
/// constexpr uint32_t EVENT_UART_TX_DONE  = (1 << 2);
///
/// EventFlags system_events;
///
/// void sensor_task() {
///     while (1) {
///         read_sensor();
///         system_events.set(EVENT_SENSOR_READY | EVENT_DATA_VALID);
///         RTOS::delay(100);
///     }
/// }
///
/// void processing_task() {
///     while (1) {
///         // Wait for both sensor ready AND data valid
///         if (system_events.wait_all(EVENT_SENSOR_READY | EVENT_DATA_VALID, 1000)) {
///             process_data();
///             system_events.clear(EVENT_DATA_VALID);
///         }
///     }
/// }
/// ```

#ifndef ALLOY_RTOS_EVENT_HPP
#define ALLOY_RTOS_EVENT_HPP

#include "rtos/rtos.hpp"
#include "rtos/scheduler.hpp"
#include "core/types.hpp"
#include "hal/interface/systick.hpp"

namespace alloy::rtos {

/// Event Flags
///
/// Provides 32 independent event flags for task synchronization.
/// Each bit represents a separate event/condition.
///
/// Key operations:
/// - set(flags): Set one or more flags (OR with current flags)
/// - clear(flags): Clear one or more flags (AND with ~flags)
/// - wait_any(flags): Block until ANY of the specified flags is set
/// - wait_all(flags): Block until ALL of the specified flags are set
///
/// Thread-safe: Yes (atomic operations with interrupts disabled)
///
/// Example: Multi-sensor coordination
/// ```cpp
/// constexpr uint32_t SENSOR_1_READY = (1 << 0);
/// constexpr uint32_t SENSOR_2_READY = (1 << 1);
/// constexpr uint32_t SENSOR_3_READY = (1 << 2);
///
/// EventFlags sensor_events;
///
/// void sensor1_task() {
///     while (1) {
///         read_sensor1();
///         sensor_events.set(SENSOR_1_READY);
///         RTOS::delay(100);
///     }
/// }
///
/// void data_fusion_task() {
///     // Wait for all 3 sensors ready
///     uint32_t all_sensors = SENSOR_1_READY | SENSOR_2_READY | SENSOR_3_READY;
///     if (sensor_events.wait_all(all_sensors, 1000)) {
///         fuse_sensor_data();
///         sensor_events.clear(all_sensors);
///     }
/// }
/// ```
class EventFlags {
private:
    core::u32 flags_;                    ///< Current event flags (32-bit mask)
    TaskControlBlock* wait_list_;        ///< Tasks blocked waiting for events

public:
    /// Constructor
    ///
    /// @param initial_flags Initial flag state (default: 0 = all clear)
    explicit EventFlags(core::u32 initial_flags = 0)
        : flags_(initial_flags)
        , wait_list_(nullptr)
    {}

    /// Set event flags
    ///
    /// Sets (ORs) the specified flags. Previously set flags remain set.
    /// Wakes up tasks waiting for these flags.
    ///
    /// Safe to call from ISR.
    ///
    /// @param flags Flags to set (bit mask)
    ///
    /// Example:
    /// ```cpp
    /// events.set(EVENT_SENSOR_READY | EVENT_DATA_VALID);
    /// ```
    void set(core::u32 flags);

    /// Clear event flags
    ///
    /// Clears (ANDs with ~flags) the specified flags.
    ///
    /// @param flags Flags to clear (bit mask)
    ///
    /// Example:
    /// ```cpp
    /// events.clear(EVENT_DATA_VALID);
    /// ```
    void clear(core::u32 flags);

    /// Wait for ANY of the specified flags
    ///
    /// Blocks until at least one of the specified flags is set.
    /// Returns the actual flags that were set (may be more than requested).
    ///
    /// @param flags Flags to wait for (bit mask)
    /// @param timeout_ms Timeout in milliseconds (INFINITE = wait forever)
    /// @param auto_clear If true, automatically clears matched flags
    /// @return Actual flags set (0 if timeout)
    ///
    /// Example:
    /// ```cpp
    /// // Wait for EITHER sensor 1 OR sensor 2
    /// uint32_t events = EVENT_SENSOR1 | EVENT_SENSOR2;
    /// uint32_t actual = event_flags.wait_any(events, 1000);
    /// if (actual & EVENT_SENSOR1) {
    ///     // Sensor 1 triggered
    /// }
    /// if (actual & EVENT_SENSOR2) {
    ///     // Sensor 2 triggered
    /// }
    /// ```
    core::u32 wait_any(core::u32 flags, core::u32 timeout_ms = INFINITE, bool auto_clear = false);

    /// Wait for ALL of the specified flags
    ///
    /// Blocks until all of the specified flags are set.
    /// Returns the actual flags (should match requested flags on success).
    ///
    /// @param flags Flags to wait for (bit mask)
    /// @param timeout_ms Timeout in milliseconds (INFINITE = wait forever)
    /// @param auto_clear If true, automatically clears matched flags
    /// @return Actual flags set (0 if timeout)
    ///
    /// Example:
    /// ```cpp
    /// // Wait for ALL sensors ready
    /// uint32_t all_ready = SENSOR1 | SENSOR2 | SENSOR3;
    /// if (event_flags.wait_all(all_ready, 1000)) {
    ///     // All sensors ready
    ///     process_all_sensors();
    ///     event_flags.clear(all_ready);
    /// }
    /// ```
    core::u32 wait_all(core::u32 flags, core::u32 timeout_ms = INFINITE, bool auto_clear = false);

    /// Get current flags (non-blocking)
    ///
    /// @return Current flag state
    core::u32 get() const {
        return flags_;
    }

    /// Check if specific flags are set
    ///
    /// @param flags Flags to check
    /// @return true if ALL specified flags are set
    bool is_set(core::u32 flags) const {
        return (flags_ & flags) == flags;
    }

    /// Check if any of the flags are set
    ///
    /// @param flags Flags to check
    /// @return true if ANY of the specified flags are set
    bool is_any_set(core::u32 flags) const {
        return (flags_ & flags) != 0;
    }

private:
    /// Check if waiting task should be unblocked
    ///
    /// @param task Task waiting on events
    /// @param wait_flags Flags the task is waiting for
    /// @param wait_all True if waiting for ALL flags, false for ANY
    /// @return true if task should be unblocked
    bool should_unblock(const TaskControlBlock* task, core::u32 wait_flags, bool wait_all) const;

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

// EventFlags implementation

inline void EventFlags::set(core::u32 flags) {
    disable_interrupts();

    // Set flags (OR)
    flags_ |= flags;

    enable_interrupts();

    // Unblock all waiting tasks that match
    // Note: We check each task's wait condition in wait_any/wait_all
    if (wait_list_ != nullptr) {
        scheduler::unblock_all_tasks(&wait_list_);
    }
}

inline void EventFlags::clear(core::u32 flags) {
    disable_interrupts();

    // Clear flags (AND with complement)
    flags_ &= ~flags;

    enable_interrupts();
}

inline core::u32 EventFlags::wait_any(core::u32 flags, core::u32 timeout_ms, bool auto_clear) {
    core::u32 start_time = systick::micros();

    while (true) {
        disable_interrupts();

        // Check if any of the requested flags are set
        core::u32 matched = flags_ & flags;
        if (matched != 0) {
            // At least one flag is set
            if (auto_clear) {
                flags_ &= ~matched;  // Clear matched flags
            }
            enable_interrupts();
            return matched;
        }

        enable_interrupts();

        // Check timeout
        if (timeout_ms != INFINITE) {
            core::u32 elapsed = systick::micros_since(start_time);
            if (elapsed >= (timeout_ms * 1000)) {
                return 0;  // Timeout
            }
        }

        // Block on wait list
        disable_interrupts();
        scheduler::block_current_task(&wait_list_);
        enable_interrupts();

        // Trigger context switch
        extern void trigger_context_switch();
        trigger_context_switch();

        // When we wake up, loop back to check flags again
    }
}

inline core::u32 EventFlags::wait_all(core::u32 flags, core::u32 timeout_ms, bool auto_clear) {
    core::u32 start_time = systick::micros();

    while (true) {
        disable_interrupts();

        // Check if all requested flags are set
        if ((flags_ & flags) == flags) {
            // All flags are set
            if (auto_clear) {
                flags_ &= ~flags;  // Clear matched flags
            }
            enable_interrupts();
            return flags;
        }

        enable_interrupts();

        // Check timeout
        if (timeout_ms != INFINITE) {
            core::u32 elapsed = systick::micros_since(start_time);
            if (elapsed >= (timeout_ms * 1000)) {
                return 0;  // Timeout
            }
        }

        // Block on wait list
        disable_interrupts();
        scheduler::block_current_task(&wait_list_);
        enable_interrupts();

        // Trigger context switch
        extern void trigger_context_switch();
        trigger_context_switch();

        // When we wake up, loop back to check flags again
    }
}

} // namespace alloy::rtos

#endif // ALLOY_RTOS_EVENT_HPP
