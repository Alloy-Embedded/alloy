/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T, E> type
 *
 * Tests the Rust-style Result<T, E> error handling type.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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
// Result Mapping Tests
// ==============================================================================

TEST_CASE("map() transforms Ok value", "[result][core][map]") {
    auto result = divide(10, 2);
    auto doubled = result.map([](int x) { return x * 2; });

    REQUIRE(doubled.is_ok());
    REQUIRE(doubled.unwrap() == 10);
}

TEST_CASE("map() preserves Err", "[result][core][map]") {
    auto result = divide(10, 0);
    auto doubled = result.map([](int x) { return x * 2; });

    REQUIRE(doubled.is_err());
    REQUIRE(doubled.unwrap_err() == ErrorCode::InvalidParameter);
}

TEST_CASE("map_err() transforms error", "[result][core][map]") {
    auto result = divide(10, 0);
    auto mapped = result.map_err([](ErrorCode) { return ErrorCode::Unknown; });

    REQUIRE(mapped.is_err());
    REQUIRE(mapped.unwrap_err() == ErrorCode::Unknown);
}

TEST_CASE("map_err() preserves Ok", "[result][core][map]") {
    auto result = divide(10, 2);
    auto mapped = result.map_err([](ErrorCode) { return ErrorCode::Unknown; });

    REQUIRE(mapped.is_ok());
    REQUIRE(mapped.unwrap() == 5);
}

// ==============================================================================
// Result and_then() Monadic Chaining Tests
// ==============================================================================

TEST_CASE("and_then() chains Ok results", "[result][core][monad]") {
    auto result = divide(10, 2)
        .and_then([](int x) { return divide(x, 5); });

    REQUIRE(result.is_ok());
    REQUIRE(result.unwrap() == 1);
}

TEST_CASE("and_then() short-circuits on first Err", "[result][core][monad]") {
    auto result = divide(10, 0)
        .and_then([](int x) { return divide(x, 5); });

    REQUIRE(result.is_err());
    REQUIRE(result.unwrap_err() == ErrorCode::InvalidParameter);
}

TEST_CASE("and_then() propagates second Err", "[result][core][monad]") {
    auto result = divide(10, 2)
        .and_then([](int x) { return divide(x, 0); });

    REQUIRE(result.is_err());
    REQUIRE(result.unwrap_err() == ErrorCode::InvalidParameter);
}

// ==============================================================================
// Result Pattern Matching Tests
// ==============================================================================

TEST_CASE("match() handles Ok case", "[result][core][match]") {
    auto result = divide(10, 2);
    int output = 0;

    result.match(
        [&](int value) { output = value; },
        [&](ErrorCode) { output = -1; }
    );

    REQUIRE(output == 5);
}

TEST_CASE("match() handles Err case", "[result][core][match]") {
    auto result = divide(10, 0);
    ErrorCode captured_error = ErrorCode::Success;

    result.match(
        [&](int) { /* not called */ },
        [&](ErrorCode error) { captured_error = error; }
    );

    REQUIRE(captured_error == ErrorCode::InvalidParameter);
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
    REQUIRE(result.unwrap_err() == ErrorCode::Timeout);
}

TEST_CASE("Result<void, E> can be chained", "[result][core][void]") {
    auto result = perform_action(true)
        .and_then([](){ return perform_action(true); });

    REQUIRE(result.is_ok());
}

// ==============================================================================
// Result Real-World Scenarios
// ==============================================================================

TEST_CASE("Complex chaining scenario", "[result][core][integration]") {
    auto result = get_username(true)
        .map([](const std::string& name) { return name.length(); })
        .and_then([](size_t len) -> Result<int, ErrorCode> {
            if (len > 10) {
                return Err(ErrorCode::InvalidParameter);
            }
            return Ok(static_cast<int>(len));
        });

    REQUIRE(result.is_ok());
    REQUIRE(result.unwrap() == 5); // "alice" has 5 characters
}

TEST_CASE("Error propagation in chain", "[result][core][integration]") {
    auto result = get_username(false)
        .map([](const std::string& name) { return name.length(); })
        .and_then([](size_t len) -> Result<int, ErrorCode> {
            return Ok(static_cast<int>(len));
        });

    REQUIRE(result.is_err());
    REQUIRE(result.unwrap_err() == ErrorCode::Busy);
}
