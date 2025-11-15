/**
 * @file test_concepts.cpp
 * @brief Unit tests for core C++20 concepts
 *
 * Tests the concept definitions used for template constraints
 */

#include <cassert>
#include <iostream>
#include <string>

#include "../../src/core/concepts.hpp"
#include "../../src/core/types.hpp"

using namespace alloy::core;

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                                \
    void test_##name();                                           \
    void run_test_##name() {                                      \
        tests_run++;                                              \
        std::cout << "Running test: " #name << "...";             \
        try {                                                     \
            test_##name();                                        \
            tests_passed++;                                       \
            std::cout << " PASS" << std::endl;                    \
        } catch (const std::exception& e) {                       \
            std::cout << " FAIL: " << e.what() << std::endl;      \
        } catch (...) {                                           \
            std::cout << " FAIL: Unknown exception" << std::endl; \
        }                                                         \
    }                                                             \
    void test_##name()

#define ASSERT(condition)                                              \
    do {                                                               \
        if (!(condition)) {                                            \
            throw std::runtime_error("Assertion failed: " #condition); \
        }                                                              \
    } while (0)

// =============================================================================
// Test Types
// =============================================================================

struct TrivialStruct {
    int x;
    double y;
};

struct NonTrivialStruct {
    std::string s;  // Not trivially copyable
};

enum class TestEnum : u8 { Value1, Value2, Value3 };

// =============================================================================
// TrivialType Concept Tests
// =============================================================================

TEST(trivial_type_basic_types) {
    // Basic types should be trivial
    ASSERT(TrivialType<u8>);
    ASSERT(TrivialType<u16>);
    ASSERT(TrivialType<u32>);
    ASSERT(TrivialType<u64>);
    ASSERT(TrivialType<i8>);
    ASSERT(TrivialType<i16>);
    ASSERT(TrivialType<i32>);
    ASSERT(TrivialType<i64>);
}

TEST(trivial_type_floating_point) {
    ASSERT(TrivialType<f32>);
    ASSERT(TrivialType<f64>);
}

TEST(trivial_type_pointers) {
    ASSERT(TrivialType<void*>);
    ASSERT(TrivialType<int*>);
    ASSERT(TrivialType<const char*>);
}

TEST(trivial_type_trivial_struct) {
    // Struct with only trivial members should be trivial
    ASSERT(TrivialType<TrivialStruct>);
}

TEST(trivial_type_non_trivial_struct) {
    // Struct with non-trivial members (like std::string) should NOT be trivial
    ASSERT(!TrivialType<NonTrivialStruct>);
}

TEST(trivial_type_enums) {
    ASSERT(TrivialType<TestEnum>);
}

// =============================================================================
// Integral Concept Tests
// =============================================================================

TEST(integral_unsigned_types) {
    ASSERT(Integral<u8>);
    ASSERT(Integral<u16>);
    ASSERT(Integral<u32>);
    ASSERT(Integral<u64>);
}

TEST(integral_signed_types) {
    ASSERT(Integral<i8>);
    ASSERT(Integral<i16>);
    ASSERT(Integral<i32>);
    ASSERT(Integral<i64>);
}

TEST(integral_bool) {
    ASSERT(Integral<bool>);
}

TEST(integral_char_types) {
    ASSERT(Integral<char>);
    ASSERT(Integral<signed char>);
    ASSERT(Integral<unsigned char>);
}

TEST(integral_not_floating_point) {
    ASSERT(!Integral<f32>);
    ASSERT(!Integral<f64>);
}

TEST(integral_not_pointer) {
    ASSERT(!Integral<void*>);
    ASSERT(!Integral<int*>);
}

// =============================================================================
// FloatingPoint Concept Tests
// =============================================================================

TEST(floating_point_basic) {
    ASSERT(FloatingPoint<f32>);
    ASSERT(FloatingPoint<f64>);
    ASSERT(FloatingPoint<float>);
    ASSERT(FloatingPoint<double>);
}

TEST(floating_point_not_integral) {
    ASSERT(!FloatingPoint<u32>);
    ASSERT(!FloatingPoint<i32>);
}

TEST(floating_point_not_pointer) {
    ASSERT(!FloatingPoint<void*>);
}

// =============================================================================
// Arithmetic Concept Tests
// =============================================================================

TEST(arithmetic_integral_types) {
    // All integral types are arithmetic
    ASSERT(Arithmetic<u8>);
    ASSERT(Arithmetic<u16>);
    ASSERT(Arithmetic<u32>);
    ASSERT(Arithmetic<u64>);
    ASSERT(Arithmetic<i8>);
    ASSERT(Arithmetic<i16>);
    ASSERT(Arithmetic<i32>);
    ASSERT(Arithmetic<i64>);
}

TEST(arithmetic_floating_types) {
    // All floating point types are arithmetic
    ASSERT(Arithmetic<f32>);
    ASSERT(Arithmetic<f64>);
}

TEST(arithmetic_not_pointer) {
    ASSERT(!Arithmetic<void*>);
    ASSERT(!Arithmetic<int*>);
}

TEST(arithmetic_not_struct) {
    ASSERT(!Arithmetic<TrivialStruct>);
}

// =============================================================================
// Enum Concept Tests
// =============================================================================

TEST(enum_scoped_enum) {
    ASSERT(Enum<TestEnum>);
}

TEST(enum_not_integral) {
    ASSERT(!Enum<u32>);
    ASSERT(!Enum<int>);
}

TEST(enum_not_struct) {
    ASSERT(!Enum<TrivialStruct>);
}

// =============================================================================
// Compile-time Function Tests using Concepts
// =============================================================================

// Example function using Integral concept
template <Integral T>
T add_one(T value) {
    return value + 1;
}

TEST(concept_integral_function) {
    u32 result = add_one(u32(41));
    ASSERT(result == 42);

    i32 result2 = add_one(i32(-1));
    ASSERT(result2 == 0);
}

// Example function using Arithmetic concept
template <Arithmetic T>
T multiply_by_two(T value) {
    return value * 2;
}

TEST(concept_arithmetic_function) {
    u32 int_result = multiply_by_two(u32(21));
    ASSERT(int_result == 42);

    f32 float_result = multiply_by_two(f32(2.5f));
    ASSERT(float_result == 5.0f);
}

// Example function using TrivialType concept
template <TrivialType T>
bool types_equal(T a, T b) {
    // For trivial types, we can use memcmp safely
    return a == b;
}

TEST(concept_trivial_function) {
    ASSERT(types_equal(u32(42), u32(42)));
    ASSERT(!types_equal(u32(42), u32(43)));
}

// =============================================================================
// Compile-time Static Assertions
// =============================================================================

// These verify concepts at compile time
static_assert(TrivialType<u32>, "u32 must be trivial");
static_assert(Integral<u32>, "u32 must be integral");
static_assert(Arithmetic<u32>, "u32 must be arithmetic");
static_assert(Arithmetic<f32>, "f32 must be arithmetic");
static_assert(Enum<TestEnum>, "TestEnum must be enum");

TEST(compile_time_concept_checks) {
    // If we reach here, all static_assert passed
    ASSERT(true);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Core Concepts Unit Tests (C++20)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_trivial_type_basic_types();
    run_test_trivial_type_floating_point();
    run_test_trivial_type_pointers();
    run_test_trivial_type_trivial_struct();
    run_test_trivial_type_non_trivial_struct();
    run_test_trivial_type_enums();

    run_test_integral_unsigned_types();
    run_test_integral_signed_types();
    run_test_integral_bool();
    run_test_integral_char_types();
    run_test_integral_not_floating_point();
    run_test_integral_not_pointer();

    run_test_floating_point_basic();
    run_test_floating_point_not_integral();
    run_test_floating_point_not_pointer();

    run_test_arithmetic_integral_types();
    run_test_arithmetic_floating_types();
    run_test_arithmetic_not_pointer();
    run_test_arithmetic_not_struct();

    run_test_enum_scoped_enum();
    run_test_enum_not_integral();
    run_test_enum_not_struct();

    run_test_concept_integral_function();
    run_test_concept_arithmetic_function();
    run_test_concept_trivial_function();
    run_test_compile_time_concept_checks();

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
