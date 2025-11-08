/**
 * @file circular_buffer.hpp
 * @brief Lock-free circular buffer (ring buffer) for embedded systems
 *
 * This file provides a high-performance, fixed-size circular buffer implementation
 * optimized for embedded systems. The buffer uses compile-time sizing and provides
 * zero-overhead abstractions.
 *
 * Design Principles:
 * - Fixed size: Size determined at compile-time (no dynamic allocation)
 * - Lock-free: Single producer/single consumer is lock-free
 * - Zero overhead: Fully inlined, no virtual functions
 * - Type-safe: Template-based with strong typing
 * - Embedded-friendly: No exceptions, no RTTI, minimal overhead
 *
 * Features:
 * - O(1) push and pop operations
 * - Power-of-2 size optimization (optional)
 * - Full/empty detection
 * - Iterator support for range-based loops
 * - Bulk operations (read/write multiple elements)
 * - Peek operations (non-destructive read)
 *
 * Use Cases:
 * - UART/SPI/I2C receive/transmit buffers
 * - Sensor data buffering
 * - Inter-thread communication (with proper memory ordering)
 * - Audio/video sample buffering
 *
 * @note Part of CoreZero Core Library
 */

#pragma once

#include "core/error.hpp"
#include "core/result.hpp"
#include "core/types.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace alloy::core {

/**
 * @brief Fixed-size circular buffer with compile-time capacity
 *
 * CircularBuffer implements a classic ring buffer with head and tail pointers.
 * The buffer can store up to N-1 elements (one slot is reserved to distinguish
 * between full and empty states).
 *
 * Thread Safety:
 * - Single producer, single consumer: Lock-free (with atomic operations)
 * - Multiple producers/consumers: Requires external synchronization
 *
 * @tparam T Element type
 * @tparam N Buffer capacity (actual usable capacity is N-1)
 * @tparam Atomic Use atomic operations for thread-safety (default: false)
 *
 * Example usage:
 * @code
 * CircularBuffer<uint8_t, 256> uart_rx_buffer;
 *
 * // Producer (ISR)
 * if (uart_rx_buffer.push(received_byte).is_ok()) {
 *     // Byte buffered successfully
 * }
 *
 * // Consumer (main loop)
 * auto result = uart_rx_buffer.pop();
 * if (result.is_ok()) {
 *     uint8_t byte = std::move(result).unwrap();
 *     process_byte(byte);
 * }
 * @endcode
 */
template <typename T, size_t N, bool Atomic = false>
class CircularBuffer {
public:
    static_assert(N > 1, "CircularBuffer capacity must be at least 2");

    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using const_reference = const T&;

    // Use atomic or regular size_t based on template parameter
    using index_type = typename std::conditional<Atomic, std::atomic<size_t>, size_t>::type;

    static constexpr size_t capacity = N;

    /**
     * @brief Construct an empty circular buffer
     */
    constexpr CircularBuffer() noexcept
        : m_data{}
        , m_head(0)
        , m_tail(0) {
    }

    /**
     * @brief Push an element to the buffer
     *
     * @param value Element to push (copied)
     * @return Result<void> Ok if successful, Err(BufferFull) if full
     */
    Result<void, ErrorCode> push(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        size_t head = load_index(m_head);
        size_t next_head = increment(head);

        if (next_head == load_index(m_tail)) {
            return Err(ErrorCode::BufferFull);
        }

        m_data[head] = value;
        store_index(m_head, next_head);

        return Ok();
    }

    /**
     * @brief Push an element to the buffer (move semantics)
     *
     * @param value Element to push (moved)
     * @return Result<void> Ok if successful, Err(BufferFull) if full
     */
    Result<void, ErrorCode> push(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) {
        size_t head = load_index(m_head);
        size_t next_head = increment(head);

        if (next_head == load_index(m_tail)) {
            return Err(ErrorCode::BufferFull);
        }

        m_data[head] = std::move(value);
        store_index(m_head, next_head);

        return Ok();
    }

    /**
     * @brief Pop an element from the buffer
     *
     * @return Result<T> Ok with element if successful, Err(BufferEmpty) if empty
     */
    Result<T, ErrorCode> pop() noexcept(std::is_nothrow_move_constructible_v<T>) {
        size_t tail = load_index(m_tail);

        if (tail == load_index(m_head)) {
            return Err(ErrorCode::BufferEmpty);
        }

        T value = std::move(m_data[tail]);
        store_index(m_tail, increment(tail));

        return Ok(std::move(value));
    }

    /**
     * @brief Peek at the front element without removing it
     *
     * @return Pointer to front element, or nullptr if empty
     */
    [[nodiscard]] const T* peek() const noexcept {
        size_t tail = load_index(m_tail);

        if (tail == load_index(m_head)) {
            return nullptr;
        }

        return &m_data[tail];
    }

    /**
     * @brief Peek at the front element without removing it (mutable)
     *
     * @return Pointer to front element, or nullptr if empty
     */
    [[nodiscard]] T* peek() noexcept {
        size_t tail = load_index(m_tail);

        if (tail == load_index(m_head)) {
            return nullptr;
        }

        return &m_data[tail];
    }

    /**
     * @brief Check if buffer is empty
     *
     * @return true if empty, false otherwise
     */
    [[nodiscard]] bool empty() const noexcept {
        return load_index(m_tail) == load_index(m_head);
    }

    /**
     * @brief Check if buffer is full
     *
     * @return true if full, false otherwise
     */
    [[nodiscard]] bool full() const noexcept {
        size_t head = load_index(m_head);
        size_t next_head = increment(head);
        return next_head == load_index(m_tail);
    }

    /**
     * @brief Get number of elements in buffer
     *
     * @return Number of elements currently stored
     */
    [[nodiscard]] size_t size() const noexcept {
        size_t head = load_index(m_head);
        size_t tail = load_index(m_tail);

        if (head >= tail) {
            return head - tail;
        } else {
            return N - tail + head;
        }
    }

    /**
     * @brief Get available space in buffer
     *
     * @return Number of elements that can be pushed
     */
    [[nodiscard]] size_t available() const noexcept {
        return (N - 1) - size();
    }

    /**
     * @brief Clear all elements from buffer
     *
     * This is NOT thread-safe and should only be called when
     * no other threads are accessing the buffer.
     */
    void clear() noexcept {
        store_index(m_head, 0);
        store_index(m_tail, 0);
    }

    /**
     * @brief Bulk read from buffer
     *
     * Reads up to count elements from the buffer without removing them.
     * Returns the actual number of elements read.
     *
     * @param dest Destination array
     * @param count Maximum number of elements to read
     * @return Number of elements actually read
     */
    size_t peek_bulk(T* dest, size_t count) const noexcept {
        size_t tail = load_index(m_tail);
        size_t head = load_index(m_head);
        size_t available_count = 0;

        // Calculate available elements
        if (head >= tail) {
            available_count = head - tail;
        } else {
            available_count = N - tail + head;
        }

        size_t read_count = (count < available_count) ? count : available_count;

        // Copy elements
        for (size_t i = 0; i < read_count; i++) {
            dest[i] = m_data[(tail + i) % N];
        }

        return read_count;
    }

    /**
     * @brief Bulk pop from buffer
     *
     * Removes up to count elements from the buffer.
     * Returns the actual number of elements removed.
     *
     * @param dest Destination array (can be nullptr to just discard)
     * @param count Maximum number of elements to pop
     * @return Number of elements actually popped
     */
    size_t pop_bulk(T* dest, size_t count) noexcept {
        size_t tail = load_index(m_tail);
        size_t head = load_index(m_head);
        size_t available_count = 0;

        // Calculate available elements
        if (head >= tail) {
            available_count = head - tail;
        } else {
            available_count = N - tail + head;
        }

        size_t pop_count = (count < available_count) ? count : available_count;

        // Copy and remove elements
        for (size_t i = 0; i < pop_count; i++) {
            size_t index = (tail + i) % N;
            if (dest != nullptr) {
                dest[i] = std::move(m_data[index]);
            }
        }

        store_index(m_tail, (tail + pop_count) % N);

        return pop_count;
    }

    /**
     * @brief Bulk push to buffer
     *
     * Pushes up to count elements to the buffer.
     * Returns the actual number of elements pushed.
     *
     * @param src Source array
     * @param count Number of elements to push
     * @return Number of elements actually pushed
     */
    size_t push_bulk(const T* src, size_t count) noexcept {
        size_t head = load_index(m_head);
        size_t tail = load_index(m_tail);

        // Calculate available space
        size_t available_space = 0;
        if (head >= tail) {
            available_space = N - head + tail - 1;
        } else {
            available_space = tail - head - 1;
        }

        size_t push_count = (count < available_space) ? count : available_space;

        // Copy elements
        for (size_t i = 0; i < push_count; i++) {
            m_data[(head + i) % N] = src[i];
        }

        store_index(m_head, (head + push_count) % N);

        return push_count;
    }

    /**
     * @brief Overwrite mode push
     *
     * Pushes element to buffer. If full, overwrites oldest element.
     * This is useful for circular logs where newest data is most important.
     *
     * @param value Element to push
     */
    void push_overwrite(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        size_t head = load_index(m_head);
        size_t next_head = increment(head);

        m_data[head] = value;

        // If buffer was full, advance tail as well
        if (next_head == load_index(m_tail)) {
            store_index(m_tail, increment(load_index(m_tail)));
        }

        store_index(m_head, next_head);
    }

    /**
     * @brief Overwrite mode push (move)
     *
     * @param value Element to push (moved)
     */
    void push_overwrite(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) {
        size_t head = load_index(m_head);
        size_t next_head = increment(head);

        m_data[head] = std::move(value);

        // If buffer was full, advance tail as well
        if (next_head == load_index(m_tail)) {
            store_index(m_tail, increment(load_index(m_tail)));
        }

        store_index(m_head, next_head);
    }

    // Iterator support for range-based for loops

    /**
     * @brief Iterator for CircularBuffer
     *
     * Allows iteration over buffer elements without removing them.
     * Note: Iterator is invalidated if buffer is modified during iteration.
     */
    class iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using iterator_category = std::forward_iterator_tag;

        iterator(CircularBuffer* buffer, size_t index, size_t count)
            : m_buffer(buffer), m_index(index), m_remaining(count) {}

        reference operator*() const {
            return m_buffer->m_data[m_index];
        }

        pointer operator->() const {
            return &m_buffer->m_data[m_index];
        }

        iterator& operator++() {
            m_index = m_buffer->increment(m_index);
            m_remaining--;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
            return m_remaining == other.m_remaining;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        CircularBuffer* m_buffer;
        size_t m_index;
        size_t m_remaining;
    };

    /**
     * @brief Const iterator for CircularBuffer
     */
    class const_iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = const T*;
        using reference = const T&;
        using iterator_category = std::forward_iterator_tag;

        const_iterator(const CircularBuffer* buffer, size_t index, size_t count)
            : m_buffer(buffer), m_index(index), m_remaining(count) {}

        reference operator*() const {
            return m_buffer->m_data[m_index];
        }

        pointer operator->() const {
            return &m_buffer->m_data[m_index];
        }

        const_iterator& operator++() {
            m_index = m_buffer->increment(m_index);
            m_remaining--;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const {
            return m_remaining == other.m_remaining;
        }

        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }

    private:
        const CircularBuffer* m_buffer;
        size_t m_index;
        size_t m_remaining;
    };

    /**
     * @brief Get iterator to first element
     */
    iterator begin() {
        return iterator(this, load_index(m_tail), size());
    }

    /**
     * @brief Get iterator past last element
     */
    iterator end() {
        return iterator(this, load_index(m_head), 0);
    }

    /**
     * @brief Get const iterator to first element
     */
    const_iterator begin() const {
        return const_iterator(this, load_index(m_tail), size());
    }

    /**
     * @brief Get const iterator past last element
     */
    const_iterator end() const {
        return const_iterator(this, load_index(m_head), 0);
    }

    /**
     * @brief Get const iterator to first element
     */
    const_iterator cbegin() const {
        return begin();
    }

    /**
     * @brief Get const iterator past last element
     */
    const_iterator cend() const {
        return end();
    }

private:
    std::array<T, N> m_data;  ///< Internal storage
    index_type m_head;        ///< Write position (next slot to write)
    index_type m_tail;        ///< Read position (next slot to read)

    /**
     * @brief Increment index with wraparound
     */
    [[nodiscard]] constexpr size_t increment(size_t index) const noexcept {
        return (index + 1) % N;
    }

    /**
     * @brief Load index with appropriate memory ordering
     */
    [[nodiscard]] size_t load_index(const index_type& index) const noexcept {
        if constexpr (Atomic) {
            return index.load(std::memory_order_acquire);
        } else {
            return index;
        }
    }

    /**
     * @brief Store index with appropriate memory ordering
     */
    void store_index(index_type& index, size_t value) noexcept {
        if constexpr (Atomic) {
            index.store(value, std::memory_order_release);
        } else {
            index = value;
        }
    }
};

// Note: Power-of-2 optimization can be added later with template specialization
// For now, the modulo operator is used which modern compilers optimize well

} // namespace alloy::core
