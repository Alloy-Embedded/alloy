/// Alloy Core - Memory Utilities
///
/// Compile-time memory footprint validation and assertions.
///
/// Purpose:
/// - Enforce memory budgets at compile time
/// - Verify zero-overhead abstractions
/// - Validate alignment requirements
///
/// Usage:
/// ```cpp
/// // Ensure Task struct fits within budget
/// ALLOY_ASSERT_MAX_SIZE(Task<256>, 512);
///
/// // Verify GPIO register is zero-overhead
/// ALLOY_ASSERT_ZERO_OVERHEAD(GPIO_Port);
///
/// // Ensure proper alignment for DMA
/// ALLOY_ASSERT_ALIGNMENT(DMA_Buffer, 32);
/// ```

#ifndef ALLOY_CORE_MEMORY_HPP
#define ALLOY_CORE_MEMORY_HPP

#include <cstddef>
#include <type_traits>

namespace alloy::core {

/// Assert that a type's size does not exceed a maximum
///
/// Triggers a compile error if sizeof(T) > max_size.
/// Useful for enforcing memory budgets on types like Task, Queue, etc.
///
/// @tparam T Type to check
/// @tparam MaxSize Maximum allowed size in bytes
///
/// Example:
/// ```cpp
/// // Ensure Task control block fits within 64 bytes
/// ALLOY_ASSERT_MAX_SIZE(TaskControlBlock, 64);
/// ```
#define ALLOY_ASSERT_MAX_SIZE(T, MaxSize) \
    static_assert(sizeof(T) <= (MaxSize), \
        #T " exceeds maximum size: sizeof(" #T ") = " \
        "actual bytes, max = " #MaxSize " bytes")

/// Assert that a type is zero-overhead (size equals expected minimum)
///
/// Triggers a compile error if sizeof(T) != expected_size.
/// Useful for verifying that wrappers/abstractions have no runtime overhead.
///
/// @tparam T Type to check
/// @tparam ExpectedSize Expected size in bytes (typically sizeof(underlying type))
///
/// Example:
/// ```cpp
/// // Ensure GPIO wrapper has no overhead vs raw pointer
/// ALLOY_ASSERT_ZERO_OVERHEAD(GPIO_Port, sizeof(void*));
///
/// // Ensure Register<T> has no overhead
/// ALLOY_ASSERT_ZERO_OVERHEAD(Register<uint32_t>, sizeof(uint32_t));
/// ```
#define ALLOY_ASSERT_ZERO_OVERHEAD(T, ExpectedSize) \
    static_assert(sizeof(T) == (ExpectedSize), \
        #T " has memory overhead: sizeof(" #T ") = actual bytes, " \
        "expected = " #ExpectedSize " bytes (zero overhead)")

/// Assert that a type meets alignment requirements
///
/// Triggers a compile error if alignof(T) < required_alignment.
/// Useful for DMA buffers, cache-aligned structures, etc.
///
/// @tparam T Type to check
/// @tparam RequiredAlignment Required alignment in bytes (must be power of 2)
///
/// Example:
/// ```cpp
/// // Ensure DMA buffer is 32-byte aligned for cache coherency
/// ALLOY_ASSERT_ALIGNMENT(DMA_Buffer, 32);
///
/// // Ensure atomic variable is naturally aligned
/// ALLOY_ASSERT_ALIGNMENT(Atomic<uint64_t>, 8);
/// ```
#define ALLOY_ASSERT_ALIGNMENT(T, RequiredAlignment) \
    static_assert(alignof(T) >= (RequiredAlignment), \
        #T " does not meet alignment requirement: alignof(" #T ") = " \
        "actual bytes, required = " #RequiredAlignment " bytes")

/// Compile-time memory budget validator
///
/// Use this class to validate memory footprints at compile time.
///
/// Example:
/// ```cpp
/// template<typename T>
/// class MyContainer {
///     static_assert(MemoryBudget<T>::fits_in(256),
///         "Container element too large for tiny MCUs");
/// };
/// ```
template<typename T>
struct MemoryBudget {
    /// Check if type fits within given budget
    static constexpr bool fits_in(size_t max_bytes) {
        return sizeof(T) <= max_bytes;
    }

    /// Check if type has zero overhead vs. raw size
    static constexpr bool is_zero_overhead(size_t raw_size) {
        return sizeof(T) == raw_size;
    }

    /// Check if type meets alignment requirement
    static constexpr bool is_aligned(size_t required_alignment) {
        return alignof(T) >= required_alignment;
    }

    /// Get size in bytes
    static constexpr size_t size() {
        return sizeof(T);
    }

    /// Get alignment in bytes
    static constexpr size_t alignment() {
        return alignof(T);
    }
};

/// Memory category classifications (from ADR-013)
namespace memory_class {
    /// Tiny MCUs (2-8KB RAM) - e.g., ATmega328P
    constexpr size_t tiny_ram = 8 * 1024;
    constexpr size_t tiny_overhead_budget = 512;

    /// Small MCUs (8-32KB RAM) - e.g., STM32F103C6, RL78/G13
    constexpr size_t small_ram = 32 * 1024;
    constexpr size_t small_overhead_budget = 2 * 1024;

    /// Medium MCUs (32-128KB RAM) - e.g., STM32F407
    constexpr size_t medium_ram = 128 * 1024;
    constexpr size_t medium_overhead_budget = 8 * 1024;

    /// Large MCUs (128+KB RAM) - e.g., SAME70
    constexpr size_t large_overhead_budget = 16 * 1024;
}

/// Calculate memory overhead for a type
///
/// Overhead = sizeof(T) - expected_minimum_size
///
/// Example:
/// ```cpp
/// // How much overhead does our GPIO wrapper add?
/// constexpr auto overhead = memory_overhead<GPIO_Port>(sizeof(void*));
/// static_assert(overhead == 0, "GPIO has overhead!");
/// ```
template<typename T>
constexpr size_t memory_overhead(size_t expected_minimum) {
    return (sizeof(T) > expected_minimum) ? (sizeof(T) - expected_minimum) : 0;
}

/// Check if type is trivially copyable (important for low-level code)
///
/// Non-trivially-copyable types have hidden overhead (copy constructors, etc.)
///
/// Example:
/// ```cpp
/// static_assert(is_trivially_copyable<SensorData>(),
///     "SensorData must be trivially copyable for DMA");
/// ```
template<typename T>
constexpr bool is_trivially_copyable() {
    return std::is_trivially_copyable_v<T>;
}

} // namespace alloy::core

#endif // ALLOY_CORE_MEMORY_HPP
