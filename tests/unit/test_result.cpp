/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T, E> type
 *
 * Tests the Rust-style Result<T, E> error handling type.
 */

#include <catch2/catch_test_macros.hpp>

#include "core/result.hpp"
#include "core/error.hpp"

using namespace alloy::core;

// ==============================================================================
// Helper Functions
// ==============================================================================

Result<int, ErrorCode> divide(int a, int b) {
    if (b == 0) {
        return Err(ErrorCode::InvalidParameter);
    }
    return Ok(a / b);
}

Result<std::string, ErrorCode> get_username(bool valid) {
    if (valid) {
        return Ok(std::string("alice"));
    }
    return Err(ErrorCode::Busy);
}

// ==============================================================================
// Result<T, E> Construction Tests
// ==============================================================================

TEST_CASE("Result can be constructed with Ok value", "[result][core]") {
    Result<int, ErrorCode> result = Ok(42);

    REQUIRE(result.is_ok());
    REQUIRE_FALSE(result.is_err());
    REQUIRE(result.unwrap() == 42);
}

TEST_CASE("Result can be constructed with Err value", "[result][core]") {
    Result<int, ErrorCode> result = Err(ErrorCode::Timeout);

    REQUIRE(result.is_err());
    REQUIRE_FALSE(result.is_ok());
    REQUIRE(result.unwrap_err() == ErrorCode::Timeout);
}

TEST_CASE("Result can hold string values", "[result][core]") {
    Result<std::string, ErrorCode> result = Ok(std::string("hello"));

    REQUIRE(result.is_ok());
    REQUIRE(result.unwrap() == "hello");
}

// ==============================================================================
// Result Unwrapping Tests
// ==============================================================================

TEST_CASE("unwrap() returns the Ok value", "[result][core]") {
    auto result = divide(10, 2);

    REQUIRE(result.is_ok());
    REQUIRE(result.unwrap() == 5);
}

TEST_CASE("unwrap_or() returns Ok value when Ok", "[result][core]") {
    auto result = divide(10, 2);

    REQUIRE(result.unwrap_or(999) == 5);
}

TEST_CASE("unwrap_or() returns default when Err", "[result][core]") {
    auto result = divide(10, 0);

    REQUIRE(result.unwrap_or(999) == 999);
}

TEST_CASE("unwrap_err() returns the error", "[result][core]") {
    auto result = divide(10, 0);

    REQUIRE(result.is_err());
    REQUIRE(result.unwrap_err() == ErrorCode::InvalidParameter);
}

// ==============================================================================
// Result State Checking Tests
// ==============================================================================

TEST_CASE("is_ok() correctly identifies Ok state", "[result][core]") {
    auto ok_result = divide(10, 2);
    auto err_result = divide(10, 0);

    REQUIRE(ok_result.is_ok());
    REQUIRE_FALSE(err_result.is_ok());
}

TEST_CASE("is_err() correctly identifies Err state", "[result][core]") {
    auto ok_result = divide(10, 2);
    auto err_result = divide(10, 0);

    REQUIRE_FALSE(ok_result.is_err());
    REQUIRE(err_result.is_err());
}

// ==============================================================================
// Result with void Type Tests
// ==============================================================================

Result<void, ErrorCode> perform_action(bool should_succeed) {
    if (should_succeed) {
        return Ok();
    }
    return Err(ErrorCode::Timeout);
}

TEST_CASE("Result<void, E> can represent success", "[result][core][void]") {
    auto result = perform_action(true);

    REQUIRE(result.is_ok());
}

TEST_CASE("Result<void, E> can represent failure", "[result][core][void]") {
    auto result = perform_action(false);

    REQUIRE(result.is_err());
}

// ==============================================================================
// Result Copy/Move Tests
// ==============================================================================

TEST_CASE("Result can be copied", "[result][core][copy]") {
    Result<int, ErrorCode> original = Ok(42);
    Result<int, ErrorCode> copy = original;

    REQUIRE(copy.is_ok());
    REQUIRE(copy.unwrap() == 42);
    REQUIRE(original.is_ok());
}

TEST_CASE("Result can be moved", "[result][core][move]") {
    Result<std::string, ErrorCode> original = Ok(std::string("hello"));
    Result<std::string, ErrorCode> moved = std::move(original);

    REQUIRE(moved.is_ok());
    REQUIRE(moved.unwrap() == "hello");
}

// ==============================================================================
// Result Real-World Scenarios
// ==============================================================================

TEST_CASE("Chaining operations with manual checks", "[result][core][integration]") {
    auto username_result = get_username(true);

    REQUIRE(username_result.is_ok());

    // Manual chaining - check and extract
    if (username_result.is_ok()) {
        std::string name = username_result.unwrap();
        REQUIRE(name.length() == 5); // "alice"
    }
}

TEST_CASE("Error propagation pattern", "[result][core][integration]") {
    auto result = get_username(false);

    REQUIRE(result.is_err());
    REQUIRE(result.unwrap_err() == ErrorCode::Busy);
}

TEST_CASE("unwrap_or provides safe default", "[result][core][integration]") {
    auto failed_username = get_username(false);
    auto username = failed_username.unwrap_or("guest");

    REQUIRE(username == "guest");
}

// ==============================================================================
// Result Error Handling Patterns
// ==============================================================================

TEST_CASE("Early return on error pattern", "[result][core][pattern]") {
    auto process_user = [](bool valid) -> int {
        auto result = get_username(valid);
        if (result.is_err()) {
            return -1; // Error indicator
        }
        return result.unwrap().length();
    };

    REQUIRE(process_user(true) == 5);
    REQUIRE(process_user(false) == -1);
}

TEST_CASE("Cascading checks pattern", "[result][core][pattern]") {
    auto divide_chain = [](int a, int b, int c) -> Result<int, ErrorCode> {
        auto result1 = divide(a, b);
        if (result1.is_err()) {
            return result1;
        }

        auto result2 = divide(result1.unwrap(), c);
        return result2;
    };

    auto success = divide_chain(100, 10, 2);
    REQUIRE(success.is_ok());
    REQUIRE(success.unwrap() == 5);

    auto failure = divide_chain(100, 0, 2);
    REQUIRE(failure.is_err());
}
