/// Alloy RTOS - C++20/23 Concepts and Compile-Time Utilities
///
/// Provides compile-time type safety and validation for RTOS primitives.
///
/// Features:
/// - fixed_string: Zero-RAM compile-time strings (C++20 NTTP)
/// - IPCMessage<T>: Concept for queue message types
/// - RTOSTickSource: Concept for tick providers
/// - QueueProducer/Consumer: Role-based concepts
/// - Compile-time validation helpers
///
/// Memory footprint:
/// - fixed_string: 0 bytes RAM (compile-time only)
/// - Concepts: 0 bytes (compile-time checks only)
///
/// Usage:
/// ```cpp
/// // fixed_string for zero-RAM task names
/// Task<512, Priority::High, "SensorTask"> task(sensor_func);
///
/// // IPCMessage concept ensures type safety
/// template <IPCMessage T, size_t N>
/// class Queue { ... };
///
/// // RTOSTickSource concept validates tick providers
/// template <RTOSTickSource TickSource>
/// void integrate_tick_source() { ... }
/// ```

#ifndef ALLOY_RTOS_CONCEPTS_HPP
#define ALLOY_RTOS_CONCEPTS_HPP

#include <concepts>
#include <type_traits>
#include <cstddef>
#include <algorithm>

#include "core/types.hpp"

namespace alloy::rtos {

// ============================================================================
// fixed_string - Zero-RAM Compile-Time String (C++20 NTTP)
// ============================================================================

/// Compile-time fixed-size string (NTTP - Non-Type Template Parameter)
///
/// Stores string at compile-time with zero RAM cost at runtime.
/// Can be used as non-type template parameter in C++20.
///
/// Example:
/// ```cpp
/// template <fixed_string Name>
/// class Task {
///     static constexpr const char* name() { return Name.data; }
/// };
///
/// Task<"MyTask"> task;  // Name stored in .rodata, not RAM
/// ```
///
/// @tparam N String length (including null terminator)
template <size_t N>
struct fixed_string {
    char data[N]{};  ///< Character array (null-terminated)

    /// Constructor from string literal
    ///
    /// @param str String literal (must be N-1 chars + null)
    consteval fixed_string(const char (&str)[N]) {
        std::copy_n(str, N, data);
    }

    /// Get length (excluding null terminator)
    ///
    /// @return String length
    consteval size_t size() const {
        return N - 1;
    }

    /// Get length (including null terminator)
    ///
    /// @return Buffer length
    consteval size_t length() const {
        return N;
    }

    /// Check if empty
    ///
    /// @return true if string is empty
    consteval bool empty() const {
        return N <= 1;
    }

    /// Array subscript operator
    ///
    /// @param i Index
    /// @return Character at index
    consteval char operator[](size_t i) const {
        return data[i];
    }

    /// Get C-string pointer
    ///
    /// @return Pointer to null-terminated string
    consteval const char* c_str() const {
        return data;
    }

    /// Compare two fixed_strings
    ///
    /// @param other Other string
    /// @return true if equal
    template <size_t M>
    consteval bool operator==(const fixed_string<M>& other) const {
        if (N != M) return false;
        for (size_t i = 0; i < N; ++i) {
            if (data[i] != other.data[i]) return false;
        }
        return true;
    }

    /// Spaceship operator for comparison
    ///
    /// @param other Other string
    /// @return Ordering
    template <size_t M>
    consteval auto operator<=>(const fixed_string<M>& other) const {
        for (size_t i = 0; i < std::min(N, M); ++i) {
            if (auto cmp = data[i] <=> other.data[i]; cmp != 0) {
                return cmp;
            }
        }
        return N <=> M;
    }
};

// Deduction guide for fixed_string
template <size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

// ============================================================================
// IPC Concepts - Type Safety for Message Passing
// ============================================================================

/// Concept: IPCMessage<T>
///
/// Validates that a type is suitable for IPC (inter-process communication).
/// Used by Queue, Mailbox, and other message-passing primitives.
///
/// Requirements:
/// - Trivially copyable (memcpy-safe)
/// - No user-defined destructor
/// - Reasonably sized (<= 256 bytes to avoid stack overflow)
///
/// Example:
/// ```cpp
/// struct SensorData {  // ✅ Valid IPCMessage
///     uint32_t timestamp;
///     int16_t temperature;
///     int16_t humidity;
/// };
///
/// struct BadMessage {  // ❌ Invalid (has std::string)
///     std::string text;
/// };
/// ```
template <typename T>
concept IPCMessage = requires {
    requires std::is_trivially_copyable_v<T>;
    requires !std::is_pointer_v<T>;  // Discourage raw pointers
    requires sizeof(T) <= 256;       // Prevent stack overflow
};

/// Concept: IPCMessageWithSize<T, MaxSize>
///
/// IPCMessage with additional size constraint.
///
/// @tparam T Type to check
/// @tparam MaxSize Maximum allowed size in bytes
template <typename T, size_t MaxSize>
concept IPCMessageWithSize = IPCMessage<T> && (sizeof(T) <= MaxSize);

// ============================================================================
// Queue Concepts - Role-Based Type Safety
// ============================================================================

/// Concept: QueueProducer<Q, T>
///
/// Validates that Q can produce messages of type T.
/// Used for compile-time verification of queue usage patterns.
///
/// Requirements:
/// - Has send(const T&) method
/// - Has try_send(const T&) method
/// - Returns Result<void, RTOSError>
template <typename Q, typename T>
concept QueueProducer = requires(Q q, const T& msg) {
    { q.send(msg) } -> std::same_as<core::Result<void, class RTOSError>>;
    { q.try_send(msg) } -> std::same_as<core::Result<void, RTOSError>>;
};

/// Concept: QueueConsumer<Q, T>
///
/// Validates that Q can consume messages of type T.
///
/// Requirements:
/// - Has receive() method returning Result<T, RTOSError>
/// - Has try_receive() method returning Result<T, RTOSError>
template <typename Q, typename T>
concept QueueConsumer = requires(Q q) {
    { q.receive() } -> std::same_as<core::Result<T, class RTOSError>>;
    { q.try_receive() } -> std::same_as<core::Result<T, class RTOSError>>;
};

/// Concept: BidirectionalQueue<Q, T>
///
/// Validates that Q is both producer and consumer.
template <typename Q, typename T>
concept BidirectionalQueue = QueueProducer<Q, T> && QueueConsumer<Q, T>;

// ============================================================================
// RTOS Tick Source Concept
// ============================================================================

/// Concept: RTOSTickSource
///
/// Validates that a type can provide RTOS tick functionality.
/// Used to ensure SysTick HAL implementations are compatible.
///
/// Requirements:
/// - micros(): Get microsecond timestamp
/// - micros_since(start): Calculate elapsed time
/// - millis(): Get millisecond timestamp
/// - init(freq): Initialize tick source
///
/// Example:
/// ```cpp
/// class BoardSysTick {
///     static core::u32 micros();
///     static core::u32 micros_since(core::u32 start);
///     static core::u32 millis();
///     static void init(core::u32 frequency_hz);
/// };
/// static_assert(RTOSTickSource<BoardSysTick>);
/// ```
template <typename T>
concept RTOSTickSource = requires(core::u32 start, core::u32 freq) {
    { T::micros() } -> std::same_as<core::u32>;
    { T::micros_since(start) } -> std::same_as<core::u32>;
    { T::millis() } -> std::same_as<core::u32>;
    { T::init(freq) } -> std::same_as<void>;
};

// ============================================================================
// Task Concepts
// ============================================================================

/// Concept: TaskFunction
///
/// Validates that a function can be used as task entry point.
///
/// Requirements:
/// - Callable with no arguments
/// - Returns void (tasks run forever)
template <typename F>
concept TaskFunction = requires(F f) {
    requires std::is_invocable_v<F>;
    requires std::is_same_v<std::invoke_result_t<F>, void>;
};

/// Concept: TaskControlBlockLike
///
/// Validates that a type behaves like a TaskControlBlock.
///
/// Requirements:
/// - Has stack_pointer, stack_base, stack_size
/// - Has priority and state
/// - Has wake_time and name
template <typename T>
concept TaskControlBlockLike = requires(T tcb) {
    { tcb.stack_pointer } -> std::convertible_to<void*>;
    { tcb.stack_base } -> std::convertible_to<void*>;
    { tcb.stack_size } -> std::convertible_to<core::u32>;
    { tcb.priority } -> std::convertible_to<core::u8>;
    { tcb.wake_time } -> std::convertible_to<core::u32>;
    { tcb.name } -> std::convertible_to<const char*>;
};

// ============================================================================
// Synchronization Primitive Concepts
// ============================================================================

/// Concept: Lockable
///
/// Validates that a type supports lock/unlock semantics.
///
/// Requirements:
/// - lock() → Result<void, RTOSError>
/// - unlock() → Result<void, RTOSError>
/// - try_lock() → Result<void, RTOSError>
template <typename T>
concept Lockable = requires(T t) {
    { t.lock() } -> std::same_as<core::Result<void, class RTOSError>>;
    { t.unlock() } -> std::same_as<core::Result<void, class RTOSError>>;
    { t.try_lock() } -> std::same_as<core::Result<void, class RTOSError>>;
};

/// Concept: Semaphore
///
/// Validates that a type supports semaphore semantics.
///
/// Requirements:
/// - give() → Result<void, RTOSError>
/// - take() → Result<void, RTOSError>
/// - try_take() → Result<void, RTOSError>
template <typename T>
concept Semaphore = requires(T t) {
    { t.give() } -> std::same_as<core::Result<void, class RTOSError>>;
    { t.take() } -> std::same_as<core::Result<void, class RTOSError>>;
    { t.try_take() } -> std::same_as<core::Result<void, class RTOSError>>;
};

// ============================================================================
// Compile-Time Validation Helpers
// ============================================================================

/// Validate stack size at compile time
///
/// @tparam StackSize Stack size in bytes
/// @return true if valid
template <size_t StackSize>
consteval bool is_valid_stack_size() {
    return StackSize >= 256 &&        // Minimum viable stack
           StackSize <= 65536 &&      // Maximum reasonable stack (64KB)
           (StackSize % 8) == 0;      // Must be 8-byte aligned
}

/// Validate priority at compile time
///
/// @tparam Priority Priority value (0-7)
/// @return true if valid
template <core::u8 Priority>
consteval bool is_valid_priority() {
    return Priority <= 7;  // 8 priority levels (0-7)
}

/// Calculate total stack RAM at compile time
///
/// @tparam StackSizes Variadic pack of stack sizes
/// @return Total RAM in bytes
template <size_t... StackSizes>
consteval size_t calculate_total_stack_ram() {
    return (StackSizes + ...);  // Fold expression
}

/// Validate all stack sizes at compile time
///
/// @tparam StackSizes Variadic pack of stack sizes
/// @return true if all valid
template <size_t... StackSizes>
consteval bool are_all_stacks_valid() {
    return (is_valid_stack_size<StackSizes>() && ...);
}

/// Validate all priorities at compile time
///
/// @tparam Priorities Variadic pack of priorities
/// @return true if all valid
template <core::u8... Priorities>
consteval bool are_all_priorities_valid() {
    return (is_valid_priority<Priorities>() && ...);
}

/// Check if priorities are unique (no duplicates)
///
/// @tparam Priorities Variadic pack of priorities
/// @return true if all unique
template <core::u8... Priorities>
consteval bool are_priorities_unique() {
    constexpr core::u8 priorities[] = {Priorities...};
    constexpr size_t N = sizeof...(Priorities);

    for (size_t i = 0; i < N; ++i) {
        for (size_t j = i + 1; j < N; ++j) {
            if (priorities[i] == priorities[j]) {
                return false;
            }
        }
    }
    return true;
}

/// Get highest priority from pack
///
/// @tparam Priorities Variadic pack of priorities
/// @return Highest priority value
template <core::u8... Priorities>
consteval core::u8 highest_priority() {
    constexpr core::u8 priorities[] = {Priorities...};
    core::u8 max = 0;
    for (core::u8 p : priorities) {
        if (p > max) max = p;
    }
    return max;
}

/// Get lowest priority from pack
///
/// @tparam Priorities Variadic pack of priorities
/// @return Lowest priority value
template <core::u8... Priorities>
consteval core::u8 lowest_priority() {
    constexpr core::u8 priorities[] = {Priorities...};
    core::u8 min = 7;
    for (core::u8 p : priorities) {
        if (p < min) min = p;
    }
    return min;
}

// ============================================================================
// Advanced Type Constraints
// ============================================================================

/// Concept: TriviallyCopyableAndSmall<T, MaxSize>
///
/// Ensures type is suitable for embedded IPC with size constraint.
///
/// @tparam T Type to check
/// @tparam MaxSize Maximum allowed size
template <typename T, size_t MaxSize = 256>
concept TriviallyCopyableAndSmall =
    std::is_trivially_copyable_v<T> &&
    sizeof(T) <= MaxSize &&
    !std::is_pointer_v<T>;

/// Concept: PODType<T>
///
/// Validates Plain Old Data type (C-compatible).
///
/// @tparam T Type to check
template <typename T>
concept PODType =
    std::is_standard_layout_v<T> &&
    std::is_trivial_v<T>;

/// Concept: HasTimestamp<T>
///
/// Validates that a message type has a timestamp field.
/// Useful for time-ordered queues and debugging.
///
/// @tparam T Type to check
template <typename T>
concept HasTimestamp = requires(T t) {
    { t.timestamp } -> std::convertible_to<core::u32>;
};

/// Concept: HasPriority<T>
///
/// Validates that a message type has a priority field.
/// Useful for priority queues.
///
/// @tparam T Type to check
template <typename T>
concept HasPriority = requires(T t) {
    { t.priority } -> std::convertible_to<core::u8>;
};

// ============================================================================
// Advanced Queue Concepts
// ============================================================================

/// Concept: PriorityQueue<Q, T>
///
/// Validates a queue that supports priority-based operations.
///
/// @tparam Q Queue type
/// @tparam T Message type (must have priority)
template <typename Q, typename T>
concept PriorityQueue =
    QueueProducer<Q, T> &&
    QueueConsumer<Q, T> &&
    HasPriority<T>;

/// Concept: TimestampedQueue<Q, T>
///
/// Validates a queue for timestamped messages.
///
/// @tparam Q Queue type
/// @tparam T Message type (must have timestamp)
template <typename Q, typename T>
concept TimestampedQueue =
    QueueProducer<Q, T> &&
    QueueConsumer<Q, T> &&
    HasTimestamp<T>;

/// Concept: BlockingQueue<Q, T>
///
/// Validates a queue with blocking semantics.
///
/// @tparam Q Queue type
/// @tparam T Message type
template <typename Q, typename T>
concept BlockingQueue = requires(Q q, const T& msg, core::u32 timeout) {
    { q.send(msg, timeout) } -> std::same_as<core::Result<void, class RTOSError>>;
    { q.receive(timeout) } -> std::same_as<core::Result<T, class RTOSError>>;
};

/// Concept: NonBlockingQueue<Q, T>
///
/// Validates a queue with non-blocking semantics.
///
/// @tparam Q Queue type
/// @tparam T Message type
template <typename Q, typename T>
concept NonBlockingQueue = requires(Q q, const T& msg) {
    { q.try_send(msg) } -> std::same_as<core::Result<void, class RTOSError>>;
    { q.try_receive() } -> std::same_as<core::Result<T, class RTOSError>>;
};

// ============================================================================
// Task-Related Advanced Concepts
// ============================================================================

/// Concept: HasTaskMetadata<T>
///
/// Validates that a task type provides metadata.
///
/// @tparam T Task type
template <typename T>
concept HasTaskMetadata = requires {
    { T::name() } -> std::convertible_to<const char*>;
    { T::stack_size() } -> std::convertible_to<size_t>;
    { T::priority() } -> std::convertible_to<class Priority>;
};

/// Concept: ValidTask<T>
///
/// Comprehensive validation for a task type.
///
/// @tparam T Task type
template <typename T>
concept ValidTask =
    HasTaskMetadata<T> &&
    requires {
        requires T::stack_size() >= 256;
        requires T::stack_size() <= 65536;
        requires (T::stack_size() % 8) == 0;
    };

// ============================================================================
// Memory Pool Concepts (for Phase 6)
// ============================================================================

/// Concept: PoolAllocatable<T>
///
/// Validates type is suitable for pool allocation.
///
/// @tparam T Type to check
template <typename T>
concept PoolAllocatable =
    std::is_trivially_destructible_v<T> &&
    sizeof(T) <= 4096 &&  // Reasonable size limit
    alignof(T) <= 64;     // Reasonable alignment

/// Concept: MemoryPool<P, T>
///
/// Validates memory pool interface.
///
/// @tparam P Pool type
/// @tparam T Allocated type
template <typename P, typename T>
concept MemoryPool = requires(P p, T* ptr) {
    { p.allocate() } -> std::same_as<core::Result<T*, class RTOSError>>;
    { p.deallocate(ptr) } -> std::same_as<core::Result<void, class RTOSError>>;
    { p.available() } -> std::convertible_to<size_t>;
};

// ============================================================================
// Interrupt Safety Concepts
// ============================================================================

/// Concept: ISRSafe<F>
///
/// Validates that a function can be called from ISR context.
/// ISR-safe functions must:
/// - Not block
/// - Not allocate memory
/// - Be reentrant
///
/// Note: This is a marker concept - actual ISR safety must be
/// verified by code review and testing.
///
/// @tparam F Function type
template <typename F>
concept ISRSafe =
    std::is_invocable_v<F> &&
    requires {
        requires noexcept(std::declval<F>()());
    };

// ============================================================================
// Compile-Time Deadlock Detection Helpers
// ============================================================================

/// Check if two priorities can cause priority inversion
///
/// @tparam HighPri High priority value
/// @tparam LowPri Low priority value
/// @return true if can cause inversion
template <core::u8 HighPri, core::u8 LowPri>
consteval bool can_cause_priority_inversion() {
    return HighPri > LowPri + 1;  // Gap > 1 allows medium priority to preempt
}

/// Detect potential deadlock in resource acquisition order
///
/// Deadlock can occur if:
/// - Task A locks R1 then R2
/// - Task B locks R2 then R1
///
/// This helper validates resource ordering.
///
/// @tparam ResourceIds Ordered resource IDs
/// @return true if ordering is consistent
template <core::u8... ResourceIds>
consteval bool has_consistent_lock_order() {
    constexpr core::u8 ids[] = {ResourceIds...};
    constexpr size_t N = sizeof...(ResourceIds);

    // Check if strictly increasing (consistent order)
    for (size_t i = 1; i < N; ++i) {
        if (ids[i] <= ids[i-1]) {
            return false;  // Not strictly increasing
        }
    }
    return true;
}

// ============================================================================
// Advanced Validation
// ============================================================================

/// Calculate worst-case stack usage for nested calls
///
/// @tparam StackUsages Individual function stack usages
/// @return Total worst-case stack
template <size_t... StackUsages>
consteval size_t worst_case_stack_usage() {
    return (StackUsages + ...);  // Sum all (worst case: all nested)
}

/// Validate that total queue memory fits in RAM budget
///
/// @tparam QueueSizes Individual queue sizes (capacity * sizeof(T))
/// @tparam MaxRAM Maximum RAM budget
/// @return true if fits
template <size_t MaxRAM, size_t... QueueSizes>
consteval bool queue_memory_fits_budget() {
    constexpr size_t total = (QueueSizes + ...);
    return total <= MaxRAM;
}

/// Calculate response time for highest priority task
///
/// Simplified Rate Monotonic Analysis (RMA)
///
/// @tparam ExecutionTime Task execution time (us)
/// @tparam Period Task period (us)
/// @return true if schedulable
template <core::u32 ExecutionTime, core::u32 Period>
consteval bool is_schedulable() {
    // Utilization must be <= 100%
    return ExecutionTime <= Period;
}

// ============================================================================
// C++23 Enhanced Features
// ============================================================================

/// Dual-mode RAM calculation (compile-time and runtime)
///
/// Uses C++23 'if consteval' to provide both compile-time and runtime paths.
/// When called in consteval context, uses fold expressions.
/// When called at runtime, uses traditional loop.
///
/// @tparam StackSizes Variadic pack of stack sizes
/// @return Total RAM
template <size_t... StackSizes>
constexpr size_t calculate_total_ram_dual() {
    if consteval {
        // Compile-time path: fold expression (very fast)
        return (StackSizes + ...);
    } else {
        // Runtime path: traditional loop (for dynamic scenarios)
        constexpr size_t sizes[] = {StackSizes...};
        size_t total = 0;
        for (size_t s : sizes) {
            total += s;
        }
        return total;
    }
}

/// Enhanced consteval error reporting
///
/// Uses C++23 consteval to ensure error messages are generated at compile time.
/// Provides better diagnostics than static_assert.
///
/// @tparam Condition Condition to check
/// @param message Error message
/// @return true if condition is true
consteval bool compile_time_check(bool condition, const char* message) {
    if (!condition) {
        // In C++23, throwing in consteval provides compile-time error with message
        // This is better than static_assert in some contexts
        throw message;
    }
    return true;
}

/// Compile-time string validation
///
/// Uses consteval to validate strings at compile time.
///
/// @tparam N String length
/// @param str String to validate
/// @return true if valid
template <size_t N>
consteval bool is_valid_task_name(const char (&str)[N]) {
    // Check length (1-31 chars + null)
    if (N < 2 || N > 32) return false;

    // Check for null terminator
    if (str[N-1] != '\0') return false;

    // Check for valid characters (alphanumeric, underscore, hyphen)
    for (size_t i = 0; i < N-1; ++i) {
        char c = str[i];
        bool valid = (c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') ||
                     (c == '_') || (c == '-');
        if (!valid) return false;
    }

    return true;
}

/// Compile-time priority validation with better error messages
///
/// @tparam Pri Priority value
/// @return Priority value (or throws compile error)
template <core::u8 Pri>
consteval core::u8 validate_priority() {
    if (Pri > 7) {
        throw "Priority must be between 0 and 7";
    }
    return Pri;
}

/// Compile-time stack size validation with better error messages
///
/// @tparam Size Stack size
/// @return Stack size (or throws compile error)
template <size_t Size>
consteval size_t validate_stack_size() {
    if (Size < 256) {
        throw "Stack size must be at least 256 bytes";
    }
    if (Size > 65536) {
        throw "Stack size must not exceed 65536 bytes";
    }
    if ((Size % 8) != 0) {
        throw "Stack size must be 8-byte aligned";
    }
    return Size;
}

/// Enhanced consteval RAM budget checker with detailed reporting
///
/// @tparam Budget Maximum RAM budget
/// @tparam Sizes Variadic pack of allocations
/// @return true if fits in budget
template <size_t Budget, size_t... Sizes>
consteval bool check_ram_budget_detailed() {
    constexpr size_t total = (Sizes + ...);
    if (total > Budget) {
        // C++23: This will produce a compile error with useful info
        throw "RAM budget exceeded";
    }
    return true;
}

/// Optimized compile-time power of 2 check
///
/// Uses C++23 consteval for guaranteed compile-time evaluation.
///
/// @tparam N Number to check
/// @return true if power of 2
template <size_t N>
consteval bool is_power_of_2() {
    return N > 0 && (N & (N - 1)) == 0;
}

/// Compile-time log2 calculation (for power-of-2 sizes)
///
/// @tparam N Number (must be power of 2)
/// @return log2(N)
template <size_t N>
consteval size_t log2_constexpr() {
    static_assert(is_power_of_2<N>(), "N must be power of 2");

    size_t result = 0;
    size_t value = N;
    while (value > 1) {
        value >>= 1;
        result++;
    }
    return result;
}

/// Compile-time array maximum
///
/// @tparam T Array element type
/// @tparam N Array size
/// @param arr Array
/// @return Maximum value
template <typename T, size_t N>
consteval T array_max(const T (&arr)[N]) {
    T max_val = arr[0];
    for (size_t i = 1; i < N; ++i) {
        if (arr[i] > max_val) {
            max_val = arr[i];
        }
    }
    return max_val;
}

/// Compile-time array minimum
///
/// @tparam T Array element type
/// @tparam N Array size
/// @param arr Array
/// @return Minimum value
template <typename T, size_t N>
consteval T array_min(const T (&arr)[N]) {
    T min_val = arr[0];
    for (size_t i = 1; i < N; ++i) {
        if (arr[i] < min_val) {
            min_val = arr[i];
        }
    }
    return min_val;
}

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_CONCEPTS_HPP
