/**
 * @file test_types.cpp
 * @brief Unit tests for core type definitions
 *
 * Tests the fundamental type aliases defined in types.hpp
 */

#include "../../src/core/types.hpp"
#include <cassert>
#include <iostream>
#include <limits>

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
// Integer Type Tests
// =============================================================================

TEST(u8_size) {
    ASSERT(sizeof(u8) == 1);
    ASSERT(std::numeric_limits<u8>::is_integer);
    ASSERT(!std::numeric_limits<u8>::is_signed);
}

TEST(u16_size) {
    ASSERT(sizeof(u16) == 2);
    ASSERT(std::numeric_limits<u16>::is_integer);
    ASSERT(!std::numeric_limits<u16>::is_signed);
}

TEST(u32_size) {
    ASSERT(sizeof(u32) == 4);
    ASSERT(std::numeric_limits<u32>::is_integer);
    ASSERT(!std::numeric_limits<u32>::is_signed);
}

TEST(u64_size) {
    ASSERT(sizeof(u64) == 8);
    ASSERT(std::numeric_limits<u64>::is_integer);
    ASSERT(!std::numeric_limits<u64>::is_signed);
}

TEST(i8_size) {
    ASSERT(sizeof(i8) == 1);
    ASSERT(std::numeric_limits<i8>::is_integer);
    ASSERT(std::numeric_limits<i8>::is_signed);
}

TEST(i16_size) {
    ASSERT(sizeof(i16) == 2);
    ASSERT(std::numeric_limits<i16>::is_integer);
    ASSERT(std::numeric_limits<i16>::is_signed);
}

TEST(i32_size) {
    ASSERT(sizeof(i32) == 4);
    ASSERT(std::numeric_limits<i32>::is_integer);
    ASSERT(std::numeric_limits<i32>::is_signed);
}

TEST(i64_size) {
    ASSERT(sizeof(i64) == 8);
    ASSERT(std::numeric_limits<i64>::is_integer);
    ASSERT(std::numeric_limits<i64>::is_signed);
}

// =============================================================================
// Size Type Tests
// =============================================================================

TEST(usize_properties) {
    ASSERT(sizeof(usize) == sizeof(size_t));
    ASSERT(std::numeric_limits<usize>::is_integer);
    ASSERT(!std::numeric_limits<usize>::is_signed);
}

TEST(isize_properties) {
    ASSERT(sizeof(isize) == sizeof(ptrdiff_t));
    ASSERT(std::numeric_limits<isize>::is_integer);
    ASSERT(std::numeric_limits<isize>::is_signed);
}

// =============================================================================
// Floating Point Type Tests
// =============================================================================

TEST(f32_size) {
    ASSERT(sizeof(f32) == 4);
    ASSERT(!std::numeric_limits<f32>::is_integer);
}

TEST(f64_size) {
    ASSERT(sizeof(f64) == 8);
    ASSERT(!std::numeric_limits<f64>::is_integer);
}

// =============================================================================
// Byte Type Tests
// =============================================================================

TEST(byte_size) {
    ASSERT(sizeof(byte) == 1);
    ASSERT(std::numeric_limits<byte>::is_integer);
    ASSERT(!std::numeric_limits<byte>::is_signed);
}

// =============================================================================
// Value Range Tests
// =============================================================================

TEST(u8_range) {
    u8 max = std::numeric_limits<u8>::max();
    u8 min = std::numeric_limits<u8>::min();
    ASSERT(max == 255);
    ASSERT(min == 0);
}

TEST(i8_range) {
    i8 max = std::numeric_limits<i8>::max();
    i8 min = std::numeric_limits<i8>::min();
    ASSERT(max == 127);
    ASSERT(min == -128);
}

TEST(u32_range) {
    u32 max = std::numeric_limits<u32>::max();
    u32 min = std::numeric_limits<u32>::min();
    ASSERT(max == 4294967295U);
    ASSERT(min == 0);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Core Types Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_u8_size();
    run_test_u16_size();
    run_test_u32_size();
    run_test_u64_size();
    run_test_i8_size();
    run_test_i16_size();
    run_test_i32_size();
    run_test_i64_size();
    run_test_usize_properties();
    run_test_isize_properties();
    run_test_f32_size();
    run_test_f64_size();
    run_test_byte_size();
    run_test_u8_range();
    run_test_i8_range();
    run_test_u32_range();

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
