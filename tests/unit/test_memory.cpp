/**
 * @file test_memory.cpp
 * @brief Unit tests for core memory utilities
 *
 * Tests compile-time memory validation macros and utilities
 */

#include "../../src/core/memory.hpp"
#include "../../src/core/types.hpp"
#include <cassert>
#include <iostream>

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

// =============================================================================
// Test Types
// =============================================================================

struct SmallStruct {
    u8 data;
};

struct MediumStruct {
    u32 data[4];
};

struct LargeStruct {
    u64 data[16];
};

struct alignas(32) AlignedStruct {
    u32 data;
};

struct alignas(64) HighlyAlignedStruct {
    u8 data;
};

// =============================================================================
// Size Assertion Tests
// =============================================================================

TEST(small_struct_size) {
    ASSERT(sizeof(SmallStruct) == 1);
    // Compile-time assertion would be: ALLOY_ASSERT_MAX_SIZE(SmallStruct, 4);
}

TEST(medium_struct_size) {
    ASSERT(sizeof(MediumStruct) == 16);
    // Compile-time assertion would be: ALLOY_ASSERT_MAX_SIZE(MediumStruct, 32);
}

TEST(large_struct_size) {
    ASSERT(sizeof(LargeStruct) == 128);
    // Compile-time assertion would be: ALLOY_ASSERT_MAX_SIZE(LargeStruct, 256);
}

// =============================================================================
// Alignment Tests
// =============================================================================

TEST(aligned_struct_alignment) {
    ASSERT(alignof(AlignedStruct) == 32);
    ASSERT(sizeof(AlignedStruct) >= sizeof(u32));
}

TEST(highly_aligned_struct_alignment) {
    ASSERT(alignof(HighlyAlignedStruct) == 64);
    ASSERT(sizeof(HighlyAlignedStruct) >= sizeof(u8));
}

TEST(basic_type_alignment) {
    ASSERT(alignof(u8) == 1);
    ASSERT(alignof(u16) == 2);
    ASSERT(alignof(u32) == 4);
    ASSERT(alignof(u64) == 8);
}

// =============================================================================
// MemoryBudget Tests
// =============================================================================

TEST(memory_budget_small_type) {
    constexpr bool fits = MemoryBudget<u8>::fits_in(1);
    ASSERT(fits == true);
}

TEST(memory_budget_medium_type) {
    constexpr bool fits = MemoryBudget<MediumStruct>::fits_in(32);
    ASSERT(fits == true);

    constexpr bool doesnt_fit = MemoryBudget<MediumStruct>::fits_in(8);
    ASSERT(doesnt_fit == false);
}

TEST(memory_budget_large_type) {
    constexpr bool fits = MemoryBudget<LargeStruct>::fits_in(256);
    ASSERT(fits == true);

    constexpr bool doesnt_fit = MemoryBudget<LargeStruct>::fits_in(64);
    ASSERT(doesnt_fit == false);
}

TEST(memory_budget_exact_size) {
    constexpr bool fits = MemoryBudget<u32>::fits_in(4);
    ASSERT(fits == true);

    constexpr bool doesnt_fit = MemoryBudget<u32>::fits_in(3);
    ASSERT(doesnt_fit == false);
}

// =============================================================================
// Zero Overhead Tests
// =============================================================================

TEST(basic_types_zero_overhead) {
    ASSERT(sizeof(u8) == 1);
    ASSERT(sizeof(u16) == 2);
    ASSERT(sizeof(u32) == 4);
    ASSERT(sizeof(u64) == 8);
}

TEST(pointer_size) {
    ASSERT(sizeof(void*) == sizeof(usize));
}

// =============================================================================
// Compile-time Assertions (these would be checked at compile time)
// =============================================================================

// These are compile-time checks - if they fail, compilation will fail
static_assert(sizeof(u8) == 1, "u8 must be 1 byte");
static_assert(sizeof(u16) == 2, "u16 must be 2 bytes");
static_assert(sizeof(u32) == 4, "u32 must be 4 bytes");
static_assert(sizeof(u64) == 8, "u64 must be 8 bytes");

static_assert(alignof(u8) == 1, "u8 must be 1-byte aligned");
static_assert(alignof(u16) == 2, "u16 must be 2-byte aligned");
static_assert(alignof(u32) == 4, "u32 must be 4-byte aligned");
static_assert(alignof(u64) == 8, "u64 must be 8-byte aligned");

TEST(compile_time_checks_passed) {
    // If we reach this test, all static_asserts passed
    ASSERT(true);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Core Memory Utilities Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_small_struct_size();
    run_test_medium_struct_size();
    run_test_large_struct_size();
    run_test_aligned_struct_alignment();
    run_test_highly_aligned_struct_alignment();
    run_test_basic_type_alignment();
    run_test_memory_budget_small_type();
    run_test_memory_budget_medium_type();
    run_test_memory_budget_large_type();
    run_test_memory_budget_exact_size();
    run_test_basic_types_zero_overhead();
    run_test_pointer_size();
    run_test_compile_time_checks_passed();

    // Print summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << tests_run << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "========================================" << std::endl;

    return (tests_run == tests_passed) ? 0 : 1;
}
