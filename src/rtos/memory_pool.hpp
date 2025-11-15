/// Alloy RTOS - Static Memory Pool
///
/// Fixed-size memory pool allocator for RTOS applications.
///
/// Features:
/// - Compile-time size and capacity
/// - O(1) allocation and deallocation
/// - No fragmentation
/// - Thread-safe with lock-free operations
/// - C++23 consteval validation
/// - Zero dynamic allocation
///
/// Memory footprint:
/// - Pool overhead: N * sizeof(void*) for free list
/// - Per block: sizeof(T) bytes
/// - Total: N * (sizeof(T) + sizeof(void*))
///
/// Use cases:
/// - Message buffers for queues
/// - Task-local storage
/// - Dynamic object creation (bounded)
/// - Replacing malloc/free in embedded systems
///
/// Usage:
/// ```cpp
/// // Define pool for sensor data (10 blocks)
/// StaticPool<SensorData, 10> sensor_pool;
///
/// // Allocate from pool
/// auto result = sensor_pool.allocate();
/// if (result.is_ok()) {
///     SensorData* data = result.unwrap();
///     // Use data...
///     sensor_pool.deallocate(data);
/// }
/// ```

#ifndef ALLOY_RTOS_MEMORY_POOL_HPP
#define ALLOY_RTOS_MEMORY_POOL_HPP

#include <cstddef>
#include <atomic>
#include <new>

#include "rtos/error.hpp"
#include "rtos/concepts.hpp"
#include "core/types.hpp"
#include "core/result.hpp"

namespace alloy::rtos {

// ============================================================================
// StaticPool - Fixed-Size Memory Pool
// ============================================================================

/// Static memory pool allocator
///
/// Provides O(1) allocation/deallocation of fixed-size blocks.
/// No fragmentation, bounded memory usage.
///
/// Template parameters:
/// @tparam T Type to allocate (must be PoolAllocatable)
/// @tparam Capacity Maximum number of blocks
///
/// Example:
/// ```cpp
/// struct Message {
///     u32 id;
///     u8 data[64];
/// };
///
/// StaticPool<Message, 16> message_pool;
///
/// // Allocate
/// auto result = message_pool.allocate();
/// if (result.is_ok()) {
///     Message* msg = result.unwrap();
///     msg->id = 1;
///     // ...
///     message_pool.deallocate(msg);
/// }
///
/// // Compile-time info
/// static_assert(message_pool.capacity() == 16);
/// static_assert(message_pool.block_size() == sizeof(Message));
/// ```
template <typename T, size_t Capacity>
class StaticPool {
    // C++23 Enhanced Validation
    static_assert(PoolAllocatable<T>, "Type must be PoolAllocatable");
    static_assert(Capacity > 0, "Pool capacity must be > 0");
    static_assert(Capacity <= 1024, "Pool capacity must be <= 1024");
    static_assert(sizeof(T) >= sizeof(void*), "Type too small for free list");

public:
    /// Constructor
    ///
    /// Initializes the pool with all blocks in the free list.
    constexpr StaticPool() : free_list_(nullptr), available_count_(Capacity) {
        // Initialize free list (link all blocks)
        for (size_t i = 0; i < Capacity; ++i) {
            void** block = reinterpret_cast<void**>(&storage_[i]);
            if (i < Capacity - 1) {
                *block = &storage_[i + 1];  // Point to next block
            } else {
                *block = nullptr;  // Last block
            }
        }
        free_list_.store(reinterpret_cast<void*>(&storage_[0]), std::memory_order_relaxed);
    }

    /// Destructor
    ~StaticPool() = default;

    // Non-copyable, non-movable (pool has fixed location)
    StaticPool(const StaticPool&) = delete;
    StaticPool& operator=(const StaticPool&) = delete;
    StaticPool(StaticPool&&) = delete;
    StaticPool& operator=(StaticPool&&) = delete;

    /// Allocate a block from the pool
    ///
    /// O(1) operation. Thread-safe (lock-free).
    ///
    /// @return Ok(T*) pointer to allocated block, Err(NoMemory) if pool exhausted
    ///
    /// Example:
    /// ```cpp
    /// auto result = pool.allocate();
    /// if (result.is_ok()) {
    ///     T* obj = result.unwrap();
    ///     new (obj) T();  // Placement new
    ///     // Use obj...
    /// }
    /// ```
    [[nodiscard]] core::Result<T*, RTOSError> allocate() noexcept {
        // Try to pop from free list (lock-free)
        void* block = free_list_.load(std::memory_order_acquire);

        while (block != nullptr) {
            void* next = *reinterpret_cast<void**>(block);

            // Try to swap free_list with next
            if (free_list_.compare_exchange_weak(block, next,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_acquire)) {
                // Success - we got the block
                available_count_.fetch_sub(1, std::memory_order_relaxed);
                return core::Ok(reinterpret_cast<T*>(block));
            }
            // CAS failed - retry with updated block value
        }

        // Pool exhausted
        return core::Err(RTOSError::NoMemory);
    }

    /// Deallocate a block back to the pool
    ///
    /// O(1) operation. Thread-safe (lock-free).
    ///
    /// @param ptr Pointer to block (must be from this pool)
    /// @return Ok(void) on success, Err(InvalidPointer) if invalid pointer
    ///
    /// Example:
    /// ```cpp
    /// T* obj = pool.allocate().unwrap();
    /// // Use obj...
    /// obj->~T();  // Manual destructor call
    /// pool.deallocate(obj);
    /// ```
    [[nodiscard]] core::Result<void, RTOSError> deallocate(T* ptr) noexcept {
        // Validate pointer is from this pool
        if (!is_from_pool(ptr)) {
            return core::Err(RTOSError::InvalidPointer);
        }

        // Push to free list (lock-free)
        void* block = reinterpret_cast<void*>(ptr);
        void* head = free_list_.load(std::memory_order_acquire);

        while (true) {
            *reinterpret_cast<void**>(block) = head;

            if (free_list_.compare_exchange_weak(head, block,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_acquire)) {
                // Success
                available_count_.fetch_add(1, std::memory_order_relaxed);
                return core::Ok();
            }
            // CAS failed - retry with updated head
        }
    }

    /// Get number of available blocks
    ///
    /// @return Number of blocks in free list
    [[nodiscard]] size_t available() const noexcept {
        return available_count_.load(std::memory_order_relaxed);
    }

    /// Get pool capacity (compile-time constant)
    ///
    /// @return Maximum number of blocks
    [[nodiscard]] static consteval size_t capacity() noexcept {
        return Capacity;
    }

    /// Get block size (compile-time constant)
    ///
    /// @return Size of each block in bytes
    [[nodiscard]] static consteval size_t block_size() noexcept {
        return sizeof(T);
    }

    /// Get total pool size (compile-time constant)
    ///
    /// @return Total bytes used by pool
    [[nodiscard]] static consteval size_t total_size() noexcept {
        return sizeof(StaticPool<T, Capacity>);
    }

    /// Check if pool is empty
    ///
    /// @return true if all blocks allocated
    [[nodiscard]] bool is_empty() const noexcept {
        return available() == 0;
    }

    /// Check if pool is full
    ///
    /// @return true if all blocks free
    [[nodiscard]] bool is_full() const noexcept {
        return available() == Capacity;
    }

    /// Reset pool (free all blocks)
    ///
    /// WARNING: Only call when no blocks are in use!
    /// Does not call destructors.
    void reset() noexcept {
        // Rebuild free list
        for (size_t i = 0; i < Capacity; ++i) {
            void** block = reinterpret_cast<void**>(&storage_[i]);
            if (i < Capacity - 1) {
                *block = &storage_[i + 1];
            } else {
                *block = nullptr;
            }
        }
        free_list_.store(reinterpret_cast<void*>(&storage_[0]), std::memory_order_release);
        available_count_.store(Capacity, std::memory_order_release);
    }

private:
    /// Check if pointer is from this pool
    [[nodiscard]] bool is_from_pool(const T* ptr) const noexcept {
        const auto* p = reinterpret_cast<const core::u8*>(ptr);
        const auto* start = reinterpret_cast<const core::u8*>(&storage_[0]);
        const auto* end = reinterpret_cast<const core::u8*>(&storage_[Capacity]);

        return (p >= start) && (p < end);
    }

    /// Storage for pool blocks
    alignas(T) core::u8 storage_[Capacity][sizeof(T)];

    /// Free list head (lock-free linked list)
    std::atomic<void*> free_list_;

    /// Available block count (for monitoring)
    std::atomic<size_t> available_count_;
};

// ============================================================================
// PoolAllocator - RAII Wrapper for Pool Allocation
// ============================================================================

/// RAII wrapper for pool-allocated objects
///
/// Automatically deallocates on destruction.
///
/// Example:
/// ```cpp
/// StaticPool<Message, 16> pool;
///
/// {
///     auto msg = PoolAllocator<Message>(pool);
///     if (msg.is_valid()) {
///         msg->id = 1;
///         // ...
///     }
/// }  // Automatically deallocated here
/// ```
template <typename T>
class PoolAllocator {
public:
    /// Constructor - allocates from pool
    template <size_t Capacity>
    explicit PoolAllocator(StaticPool<T, Capacity>& pool)
        : pool_base_(reinterpret_cast<void*>(&pool)),
          deallocate_fn_([](void* pool_ptr, T* obj) {
              auto* pool = reinterpret_cast<StaticPool<T, Capacity>*>(pool_ptr);
              pool->deallocate(obj);
          })
    {
        auto result = pool.allocate();
        if (result.is_ok()) {
            ptr_ = result.unwrap();
            new (ptr_) T();  // Placement new
        }
    }

    /// Destructor - deallocates back to pool
    ~PoolAllocator() {
        if (ptr_ != nullptr) {
            ptr_->~T();  // Call destructor
            deallocate_fn_(pool_base_, ptr_);
        }
    }

    // Non-copyable (unique ownership)
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    // Movable
    PoolAllocator(PoolAllocator&& other) noexcept
        : ptr_(other.ptr_),
          pool_base_(other.pool_base_),
          deallocate_fn_(other.deallocate_fn_)
    {
        other.ptr_ = nullptr;
    }

    PoolAllocator& operator=(PoolAllocator&& other) noexcept {
        if (this != &other) {
            if (ptr_ != nullptr) {
                ptr_->~T();
                deallocate_fn_(pool_base_, ptr_);
            }
            ptr_ = other.ptr_;
            pool_base_ = other.pool_base_;
            deallocate_fn_ = other.deallocate_fn_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    /// Check if allocation succeeded
    [[nodiscard]] bool is_valid() const noexcept {
        return ptr_ != nullptr;
    }

    /// Get raw pointer
    [[nodiscard]] T* get() noexcept {
        return ptr_;
    }

    [[nodiscard]] const T* get() const noexcept {
        return ptr_;
    }

    /// Dereference operators
    T& operator*() noexcept {
        return *ptr_;
    }

    const T& operator*() const noexcept {
        return *ptr_;
    }

    T* operator->() noexcept {
        return ptr_;
    }

    const T* operator->() const noexcept {
        return ptr_;
    }

    /// Release ownership (manual deallocation required)
    [[nodiscard]] T* release() noexcept {
        T* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }

private:
    T* ptr_{nullptr};
    void* pool_base_;
    void (*deallocate_fn_)(void*, T*);
};

// ============================================================================
// C++23 Compile-Time Validation
// ============================================================================

/// Calculate memory overhead for pool
///
/// @tparam T Type
/// @tparam Capacity Pool capacity
/// @return Overhead in bytes
template <typename T, size_t Capacity>
consteval size_t pool_overhead() {
    return sizeof(StaticPool<T, Capacity>) - (Capacity * sizeof(T));
}

/// Validate pool fits in memory budget
///
/// @tparam T Type
/// @tparam Capacity Pool capacity
/// @tparam Budget RAM budget in bytes
/// @return true if fits
template <typename T, size_t Capacity, size_t Budget>
consteval bool pool_fits_budget() {
    constexpr size_t total = sizeof(StaticPool<T, Capacity>);
    compile_time_check(total <= Budget, "Pool exceeds memory budget");
    return true;
}

/// Calculate optimal pool capacity for budget
///
/// @tparam T Type
/// @tparam Budget RAM budget
/// @return Maximum capacity that fits in budget
template <typename T, size_t Budget>
consteval size_t optimal_pool_capacity() {
    // Account for overhead (free list pointers)
    constexpr size_t block_size = sizeof(T);
    constexpr size_t overhead_per_block = sizeof(void*);
    constexpr size_t total_per_block = block_size + overhead_per_block;

    return Budget / total_per_block;
}

}  // namespace alloy::rtos

#endif  // ALLOY_RTOS_MEMORY_POOL_HPP
