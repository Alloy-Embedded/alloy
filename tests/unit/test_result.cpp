/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T, E> type
 *
 * Tests the core Result<T, E> error handling type to ensure
 * it behaves correctly in all scenarios.
 */

#include "../../src/core/result.hpp"
#include "../../src/core/error.hpp"
#include <cassert>
#include <string>
#include <iostream>

using namespace core;

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
// Result<T> Construction Tests
// =============================================================================

TEST(result_ok_construction) {
    auto result = Result<int, ErrorCode>::ok(42);
    ASSERT(result.is_ok());
    ASSERT(!result.is_error());
    ASSERT(result.value() == 42);
}

TEST(result_error_construction) {
    auto result = Result<int, ErrorCode>::error(ErrorCode::Generic);
    ASSERT(!result.is_ok());
    ASSERT(result.is_error());
    ASSERT(result.error() == ErrorCode::Generic);
}

TEST(result_void_ok) {
    auto result = Result<void, ErrorCode>::ok();
    ASSERT(result.is_ok());
    ASSERT(!result.is_error());
}

TEST(result_void_error) {
    auto result = Result<void, ErrorCode>::error(ErrorCode::Timeout);
    ASSERT(!result.is_ok());
    ASSERT(result.is_error());
    ASSERT(result.error() == ErrorCode::Timeout);
}

// =============================================================================
// Result<T> Value Access Tests
// =============================================================================

TEST(result_value_access) {
    auto result = Result<int, ErrorCode>::ok(123);
    ASSERT(result.value() == 123);
}

TEST(result_value_or) {
    auto ok_result = Result<int, ErrorCode>::ok(42);
    ASSERT(ok_result.value_or(999) == 42);

    auto err_result = Result<int, ErrorCode>::error(ErrorCode::Generic);
    ASSERT(err_result.value_or(999) == 999);
}

TEST(result_error_access) {
    auto result = Result<int, ErrorCode>::error(ErrorCode::Hardware);
    ASSERT(result.error() == ErrorCode::Hardware);
}

// =============================================================================
// Result<T> Move Semantics Tests
// =============================================================================

TEST(result_move_construction) {
    auto result1 = Result<std::string, ErrorCode>::ok("hello");
    auto result2 = std::move(result1);
    ASSERT(result2.is_ok());
    ASSERT(result2.value() == "hello");
}

TEST(result_move_assignment) {
    auto result1 = Result<std::string, ErrorCode>::ok("world");
    Result<std::string, ErrorCode> result2 = Result<std::string, ErrorCode>::error(ErrorCode::Generic);
    result2 = std::move(result1);
    ASSERT(result2.is_ok());
    ASSERT(result2.value() == "world");
}

// =============================================================================
// Result<T> Monadic Operations Tests
// =============================================================================

TEST(result_map_ok) {
    auto result = Result<int, ErrorCode>::ok(5);
    auto mapped = result.map([](int x) { return x * 2; });
    ASSERT(mapped.is_ok());
    ASSERT(mapped.value() == 10);
}

TEST(result_map_error) {
    auto result = Result<int, ErrorCode>::error(ErrorCode::Generic);
    auto mapped = result.map([](int x) { return x * 2; });
    ASSERT(mapped.is_error());
    ASSERT(mapped.error() == ErrorCode::Generic);
}

TEST(result_and_then_ok) {
    auto result = Result<int, ErrorCode>::ok(5);
    auto chained = result.and_then([](int x) {
        return Result<int, ErrorCode>::ok(x * 3);
    });
    ASSERT(chained.is_ok());
    ASSERT(chained.value() == 15);
}

TEST(result_and_then_error) {
    auto result = Result<int, ErrorCode>::error(ErrorCode::Timeout);
    auto chained = result.and_then([](int x) {
        return Result<int, ErrorCode>::ok(x * 3);
    });
    ASSERT(chained.is_error());
    ASSERT(chained.error() == ErrorCode::Timeout);
}

TEST(result_and_then_chain_error) {
    auto result = Result<int, ErrorCode>::ok(5);
    auto chained = result.and_then([](int x) {
        return Result<int, ErrorCode>::error(ErrorCode::Hardware);
    });
    ASSERT(chained.is_error());
    ASSERT(chained.error() == ErrorCode::Hardware);
}

TEST(result_or_else_ok) {
    auto result = Result<int, ErrorCode>::ok(42);
    auto recovered = result.or_else([](ErrorCode) {
        return Result<int, ErrorCode>::ok(999);
    });
    ASSERT(recovered.is_ok());
    ASSERT(recovered.value() == 42);
}

TEST(result_or_else_error) {
    auto result = Result<int, ErrorCode>::error(ErrorCode::Generic);
    auto recovered = result.or_else([](ErrorCode err) {
        return Result<int, ErrorCode>::ok(999);
    });
    ASSERT(recovered.is_ok());
    ASSERT(recovered.value() == 999);
}

// =============================================================================
// Complex Chaining Tests
// =============================================================================

TEST(result_complex_chain) {
    auto result = Result<int, ErrorCode>::ok(10)
        .map([](int x) { return x + 5; })
        .and_then([](int x) { return Result<int, ErrorCode>::ok(x * 2); })
        .map([](int x) { return x - 10; });

    ASSERT(result.is_ok());
    ASSERT(result.value() == 20);  // (10 + 5) * 2 - 10 = 20
}

TEST(result_chain_with_error) {
    auto result = Result<int, ErrorCode>::ok(10)
        .map([](int x) { return x + 5; })
        .and_then([](int x) {
            return Result<int, ErrorCode>::error(ErrorCode::Communication);
        })
        .map([](int x) { return x - 10; });  // This shouldn't execute

    ASSERT(result.is_error());
    ASSERT(result.error() == ErrorCode::Communication);
}

// =============================================================================
// Edge Cases Tests
// =============================================================================

TEST(result_with_pointer) {
    int value = 42;
    auto result = Result<int*, ErrorCode>::ok(&value);
    ASSERT(result.is_ok());
    ASSERT(*result.value() == 42);
}

TEST(result_with_struct) {
    struct TestStruct {
        int x;
        std::string s;
    };

    TestStruct ts{123, "test"};
    auto result = Result<TestStruct, ErrorCode>::ok(ts);
    ASSERT(result.is_ok());
    ASSERT(result.value().x == 123);
    ASSERT(result.value().s == "test");
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Result<T, E> Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_result_ok_construction();
    run_test_result_error_construction();
    run_test_result_void_ok();
    run_test_result_void_error();
    run_test_result_value_access();
    run_test_result_value_or();
    run_test_result_error_access();
    run_test_result_move_construction();
    run_test_result_move_assignment();
    run_test_result_map_ok();
    run_test_result_map_error();
    run_test_result_and_then_ok();
    run_test_result_and_then_error();
    run_test_result_and_then_chain_error();
    run_test_result_or_else_ok();
    run_test_result_or_else_error();
    run_test_result_complex_chain();
    run_test_result_chain_with_error();
    run_test_result_with_pointer();
    run_test_result_with_struct();

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
