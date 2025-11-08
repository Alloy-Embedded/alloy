/**
 * @file test_circular_buffer.cpp
 * @brief Unit tests for CircularBuffer
 *
 * Tests cover:
 * - Basic push/pop operations
 * - Full and empty detection
 * - Wraparound behavior
 * - Bulk operations
 * - Overwrite mode
 * - Iterator support
 * - Peek operations
 */

#include "core/circular_buffer.hpp"
#include "core/error.hpp"
#include "core/result.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace alloy::core;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        tests_run++; \
        std::cout << "Running test: " #name << "..."; \
        try { \
            test_##name(); \
            tests_passed++; \
            std::cout << " PASS" << std::endl; \
        } catch (const std::exception& e) { \
            std::cout << " FAIL: " << e.what() << std::endl; \
        } catch (...) { \
            std::cout << " FAIL: Unknown exception" << std::endl; \
        } \
    } \
    void test_##name()

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Assertion failed: " #condition); \
        } \
    } while(0)

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST(buffer_starts_empty) {
    CircularBuffer<int, 8> buffer;

    ASSERT(buffer.empty());
    ASSERT(!buffer.full());
    ASSERT(buffer.size() == 0);
    ASSERT(buffer.available() == 7);  // N-1 usable slots
}

TEST(push_single_element) {
    CircularBuffer<int, 8> buffer;

    auto result = buffer.push(42);

    ASSERT(result.is_ok());
    ASSERT(!buffer.empty());
    ASSERT(!buffer.full());
    ASSERT(buffer.size() == 1);
    ASSERT(buffer.available() == 6);
}

TEST(push_and_pop) {
    CircularBuffer<int, 8> buffer;

    buffer.push(42);
    auto result = buffer.pop();

    ASSERT(result.is_ok());
    ASSERT(std::move(result).unwrap() == 42);
    ASSERT(buffer.empty());
    ASSERT(buffer.size() == 0);
}

TEST(pop_empty_fails) {
    CircularBuffer<int, 8> buffer;

    auto result = buffer.pop();

    ASSERT(!result.is_ok());
    ASSERT(result.err() == ErrorCode::BufferEmpty);
}

TEST(push_to_full) {
    CircularBuffer<int, 8> buffer;

    // Fill buffer (capacity is 7, not 8)
    for (int i = 0; i < 7; i++) {
        auto result = buffer.push(i);
        ASSERT(result.is_ok());
    }

    ASSERT(buffer.full());
    ASSERT(buffer.size() == 7);
    ASSERT(buffer.available() == 0);
}

TEST(push_when_full_fails) {
    CircularBuffer<int, 8> buffer;

    // Fill buffer
    for (int i = 0; i < 7; i++) {
        buffer.push(i);
    }

    // Try to push one more
    auto result = buffer.push(999);

    ASSERT(!result.is_ok());
    ASSERT(result.err() == ErrorCode::BufferFull);
}

TEST(fifo_ordering) {
    CircularBuffer<int, 8> buffer;

    // Push 5 elements
    for (int i = 0; i < 5; i++) {
        buffer.push(i * 10);
    }

    // Pop and verify FIFO order
    for (int i = 0; i < 5; i++) {
        auto result = buffer.pop();
        ASSERT(result.is_ok());
        ASSERT(std::move(result).unwrap() == i * 10);
    }
}

// ============================================================================
// Wraparound Tests
// ============================================================================

TEST(wraparound_behavior) {
    CircularBuffer<int, 8> buffer;

    // Fill buffer
    for (int i = 0; i < 7; i++) {
        buffer.push(i);
    }

    // Pop 3 elements
    buffer.pop();
    buffer.pop();
    buffer.pop();

    // Push 3 more (this causes wraparound)
    buffer.push(100);
    buffer.push(101);
    buffer.push(102);

    // Verify size
    ASSERT(buffer.size() == 7);

    // Pop all and verify order
    std::vector<int> expected = {3, 4, 5, 6, 100, 101, 102};
    for (int exp : expected) {
        auto result = buffer.pop();
        ASSERT(result.is_ok());
        ASSERT(std::move(result).unwrap() == exp);
    }
}

TEST(multiple_wraparounds) {
    CircularBuffer<int, 16> buffer;

    // Perform multiple wrap cycles
    for (int cycle = 0; cycle < 5; cycle++) {
        // Fill buffer
        for (int i = 0; i < 10; i++) {
            buffer.push(cycle * 100 + i);
        }

        // Pop all
        for (int i = 0; i < 10; i++) {
            auto result = buffer.pop();
            ASSERT(result.is_ok());
            ASSERT(std::move(result).unwrap() == cycle * 100 + i);
        }

        ASSERT(buffer.empty());
    }
}

// ============================================================================
// Peek Operations Tests
// ============================================================================

TEST(peek_empty_returns_null) {
    CircularBuffer<int, 8> buffer;

    const int* ptr = buffer.peek();

    ASSERT(ptr == nullptr);
}

TEST(peek_does_not_remove) {
    CircularBuffer<int, 8> buffer;

    buffer.push(42);

    const int* ptr = buffer.peek();
    ASSERT(ptr != nullptr);
    ASSERT(*ptr == 42);
    ASSERT(buffer.size() == 1);  // Still has 1 element

    // Pop should return same value
    auto result = buffer.pop();
    ASSERT(std::move(result).unwrap() == 42);
}

TEST(peek_mutable) {
    CircularBuffer<int, 8> buffer;

    buffer.push(42);

    int* ptr = buffer.peek();
    ASSERT(ptr != nullptr);
    ASSERT(*ptr == 42);

    // Modify through peek
    *ptr = 100;

    // Verify modification
    auto result = buffer.pop();
    ASSERT(std::move(result).unwrap() == 100);
}

// ============================================================================
// Bulk Operations Tests
// ============================================================================

TEST(peek_bulk) {
    CircularBuffer<int, 16> buffer;

    // Push 10 elements
    for (int i = 0; i < 10; i++) {
        buffer.push(i);
    }

    // Peek 5 elements
    int data[5];
    size_t count = buffer.peek_bulk(data, 5);

    ASSERT(count == 5);
    for (int i = 0; i < 5; i++) {
        ASSERT(data[i] == i);
    }

    // Buffer should still have 10 elements
    ASSERT(buffer.size() == 10);
}

TEST(pop_bulk) {
    CircularBuffer<int, 16> buffer;

    // Push 10 elements
    for (int i = 0; i < 10; i++) {
        buffer.push(i);
    }

    // Pop 5 elements
    int data[5];
    size_t count = buffer.pop_bulk(data, 5);

    ASSERT(count == 5);
    for (int i = 0; i < 5; i++) {
        ASSERT(data[i] == i);
    }

    // Buffer should now have 5 elements
    ASSERT(buffer.size() == 5);

    // Verify remaining elements
    for (int i = 5; i < 10; i++) {
        auto result = buffer.pop();
        ASSERT(std::move(result).unwrap() == i);
    }
}

TEST(push_bulk) {
    CircularBuffer<int, 16> buffer;

    int data[] = {10, 20, 30, 40, 50};
    size_t count = buffer.push_bulk(data, 5);

    ASSERT(count == 5);
    ASSERT(buffer.size() == 5);

    // Verify elements
    for (int i = 0; i < 5; i++) {
        auto result = buffer.pop();
        ASSERT(std::move(result).unwrap() == data[i]);
    }
}

TEST(bulk_operations_with_wraparound) {
    CircularBuffer<int, 16> buffer;

    // Fill partially
    for (int i = 0; i < 8; i++) {
        buffer.push(i);
    }

    // Pop some to create wraparound condition
    for (int i = 0; i < 5; i++) {
        buffer.pop();
    }

    // Now push bulk (will wrap)
    int data[] = {100, 101, 102, 103, 104};
    size_t count = buffer.push_bulk(data, 5);

    ASSERT(count == 5);
    ASSERT(buffer.size() == 8);
}

TEST(pop_bulk_nullptr) {
    CircularBuffer<int, 16> buffer;

    // Push some elements
    for (int i = 0; i < 5; i++) {
        buffer.push(i);
    }

    // Pop with nullptr (just discard)
    size_t count = buffer.pop_bulk(nullptr, 3);

    ASSERT(count == 3);
    ASSERT(buffer.size() == 2);
}

// ============================================================================
// Overwrite Mode Tests
// ============================================================================

TEST(overwrite_when_not_full) {
    CircularBuffer<int, 8> buffer;

    buffer.push_overwrite(42);

    ASSERT(buffer.size() == 1);
    auto result = buffer.pop();
    ASSERT(std::move(result).unwrap() == 42);
}

TEST(overwrite_when_full) {
    CircularBuffer<int, 8> buffer;

    // Fill buffer
    for (int i = 0; i < 7; i++) {
        buffer.push(i);
    }

    ASSERT(buffer.full());
    size_t size_before = buffer.size();

    // Overwrite (should discard oldest)
    buffer.push_overwrite(999);

    ASSERT(buffer.size() == size_before);  // Size unchanged
    ASSERT(buffer.full());

    // First element should now be 1 (0 was overwritten)
    auto result = buffer.pop();
    ASSERT(std::move(result).unwrap() == 1);
}

TEST(overwrite_multiple) {
    CircularBuffer<int, 8> buffer;

    // Fill buffer
    for (int i = 0; i < 7; i++) {
        buffer.push(i);
    }

    // Overwrite 3 times
    buffer.push_overwrite(100);
    buffer.push_overwrite(101);
    buffer.push_overwrite(102);

    // Should have [3, 4, 5, 6, 100, 101, 102]
    std::vector<int> expected = {3, 4, 5, 6, 100, 101, 102};
    for (int exp : expected) {
        auto result = buffer.pop();
        ASSERT(std::move(result).unwrap() == exp);
    }
}

// ============================================================================
// Clear Operation Tests
// ============================================================================

TEST(clear_empty_buffer) {
    CircularBuffer<int, 8> buffer;

    buffer.clear();

    ASSERT(buffer.empty());
    ASSERT(buffer.size() == 0);
}

TEST(clear_full_buffer) {
    CircularBuffer<int, 8> buffer;

    // Fill buffer
    for (int i = 0; i < 7; i++) {
        buffer.push(i);
    }

    buffer.clear();

    ASSERT(buffer.empty());
    ASSERT(buffer.size() == 0);
    ASSERT(buffer.available() == 7);
}

TEST(clear_and_reuse) {
    CircularBuffer<int, 8> buffer;

    // Use buffer
    for (int i = 0; i < 5; i++) {
        buffer.push(i);
    }

    buffer.clear();

    // Reuse buffer
    buffer.push(100);
    buffer.push(101);

    ASSERT(buffer.size() == 2);
    auto result1 = buffer.pop();
    auto result2 = buffer.pop();
    ASSERT(std::move(result1).unwrap() == 100);
    ASSERT(std::move(result2).unwrap() == 101);
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(iterator_empty_buffer) {
    CircularBuffer<int, 8> buffer;

    int count = 0;
    for (auto& elem : buffer) {
        (void)elem;
        count++;
    }

    ASSERT(count == 0);
}

TEST(iterator_basic) {
    CircularBuffer<int, 8> buffer;

    // Push elements
    for (int i = 0; i < 5; i++) {
        buffer.push(i * 10);
    }

    // Iterate and verify
    int expected = 0;
    for (const auto& elem : buffer) {
        ASSERT(elem == expected * 10);
        expected++;
    }

    ASSERT(expected == 5);
}

TEST(iterator_modification) {
    CircularBuffer<int, 8> buffer;

    // Push elements
    for (int i = 0; i < 5; i++) {
        buffer.push(i);
    }

    // Modify through iterator
    for (auto& elem : buffer) {
        elem *= 10;
    }

    // Verify modifications
    for (int i = 0; i < 5; i++) {
        auto result = buffer.pop();
        ASSERT(std::move(result).unwrap() == i * 10);
    }
}

TEST(iterator_with_wraparound) {
    CircularBuffer<int, 8> buffer;

    // Fill buffer
    for (int i = 0; i < 7; i++) {
        buffer.push(i);
    }

    // Pop some elements
    buffer.pop();
    buffer.pop();

    // Push more (causes wraparound)
    buffer.push(100);
    buffer.push(101);

    // Iterate and verify order
    std::vector<int> expected = {2, 3, 4, 5, 6, 100, 101};
    size_t idx = 0;
    for (const auto& elem : buffer) {
        ASSERT(elem == expected[idx]);
        idx++;
    }
}

TEST(const_iterator) {
    CircularBuffer<int, 8> buffer;

    for (int i = 0; i < 5; i++) {
        buffer.push(i);
    }

    const auto& const_buffer = buffer;

    int expected = 0;
    for (const auto& elem : const_buffer) {
        ASSERT(elem == expected);
        expected++;
    }
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST(push_move_semantics) {
    CircularBuffer<std::vector<int>, 8> buffer;

    std::vector<int> vec = {1, 2, 3, 4, 5};
    size_t original_size = vec.size();

    buffer.push(std::move(vec));

    // Original vector should be moved-from
    ASSERT(vec.empty() || vec.size() == 0);  // Moved-from state

    auto result = buffer.pop();
    ASSERT(result.is_ok());
    auto popped = std::move(result).unwrap();
    ASSERT(popped.size() == original_size);
}

// ============================================================================
// Size and Capacity Tests
// ============================================================================

TEST(size_tracking) {
    CircularBuffer<int, 16> buffer;

    ASSERT(buffer.size() == 0);

    // Push 10
    for (int i = 0; i < 10; i++) {
        buffer.push(i);
        ASSERT(buffer.size() == static_cast<size_t>(i + 1));
    }

    // Pop 5
    for (int i = 0; i < 5; i++) {
        buffer.pop();
        ASSERT(buffer.size() == static_cast<size_t>(10 - i - 1));
    }

    ASSERT(buffer.size() == 5);
}

TEST(available_tracking) {
    CircularBuffer<int, 8> buffer;

    ASSERT(buffer.available() == 7);

    buffer.push(1);
    ASSERT(buffer.available() == 6);

    buffer.push(2);
    buffer.push(3);
    ASSERT(buffer.available() == 4);

    buffer.pop();
    ASSERT(buffer.available() == 5);
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST(single_element_buffer) {
    CircularBuffer<int, 2> buffer;  // Only 1 usable slot

    auto r1 = buffer.push(42);
    ASSERT(r1.is_ok());
    ASSERT(buffer.full());

    auto r2 = buffer.push(99);
    ASSERT(!r2.is_ok());  // Should fail

    auto r3 = buffer.pop();
    ASSERT(std::move(r3).unwrap() == 42);
    ASSERT(buffer.empty());
}

TEST(alternating_push_pop) {
    CircularBuffer<int, 8> buffer;

    for (int cycle = 0; cycle < 20; cycle++) {
        auto r1 = buffer.push(cycle);
        ASSERT(r1.is_ok());

        auto r2 = buffer.pop();
        ASSERT(r2.is_ok());
        ASSERT(std::move(r2).unwrap() == cycle);

        ASSERT(buffer.empty());
    }
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "CircularBuffer Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Basic operations
    std::cout << "--- Basic Operations ---" << std::endl;
    run_test_buffer_starts_empty();
    run_test_push_single_element();
    run_test_push_and_pop();
    run_test_pop_empty_fails();
    run_test_push_to_full();
    run_test_push_when_full_fails();
    run_test_fifo_ordering();

    // Wraparound
    std::cout << std::endl << "--- Wraparound ---" << std::endl;
    run_test_wraparound_behavior();
    run_test_multiple_wraparounds();

    // Peek operations
    std::cout << std::endl << "--- Peek Operations ---" << std::endl;
    run_test_peek_empty_returns_null();
    run_test_peek_does_not_remove();
    run_test_peek_mutable();

    // Bulk operations
    std::cout << std::endl << "--- Bulk Operations ---" << std::endl;
    run_test_peek_bulk();
    run_test_pop_bulk();
    run_test_push_bulk();
    run_test_bulk_operations_with_wraparound();
    run_test_pop_bulk_nullptr();

    // Overwrite mode
    std::cout << std::endl << "--- Overwrite Mode ---" << std::endl;
    run_test_overwrite_when_not_full();
    run_test_overwrite_when_full();
    run_test_overwrite_multiple();

    // Clear operation
    std::cout << std::endl << "--- Clear Operation ---" << std::endl;
    run_test_clear_empty_buffer();
    run_test_clear_full_buffer();
    run_test_clear_and_reuse();

    // Iterators
    std::cout << std::endl << "--- Iterators ---" << std::endl;
    run_test_iterator_empty_buffer();
    run_test_iterator_basic();
    run_test_iterator_modification();
    run_test_iterator_with_wraparound();
    run_test_const_iterator();

    // Move semantics
    std::cout << std::endl << "--- Move Semantics ---" << std::endl;
    run_test_push_move_semantics();

    // Size and capacity
    std::cout << std::endl << "--- Size and Capacity ---" << std::endl;
    run_test_size_tracking();
    run_test_available_tracking();

    // Edge cases
    std::cout << std::endl << "--- Edge Cases ---" << std::endl;
    run_test_single_element_buffer();
    run_test_alternating_push_pop();

    // Summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Tests run:    " << tests_run << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    return (tests_run == tests_passed) ? 0 : 1;
}
