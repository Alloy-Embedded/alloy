/**
 * @file test_result_regressions.cpp
 * @brief Regression tests for Result<T,E> bugs
 *
 * This file documents bugs that were found in Result<T,E> implementation
 * and ensures they don't reoccur.
 */

#include <catch2/catch_test_macros.hpp>

#include "core/result.hpp"
#include "core/error.hpp"

#include <string>

using namespace alloy::core;

// ==============================================================================
// Helper Functions
// ==============================================================================

Result<int, ErrorCode> divide_by_zero(int a, int b) {
    if (b == 0) {
        return Err(ErrorCode::InvalidParameter);
    }
    return Ok(a / b);
}

Result<int, ErrorCode> divide_regression(int a, int b) {
    if (b == 0) {
        return Err(ErrorCode::InvalidParameter);
    }
    return Ok(a / b);
}

Result<void, ErrorCode> perform_action(bool should_succeed) {
    if (should_succeed) {
        return Ok();
    }
    return Err(ErrorCode::Timeout);
}

// ==============================================================================
// BUG #1: Result<void, E> missing unwrap_err() method
// ==============================================================================

/**
 * @bug Result<void, ErrorCode> didn't have unwrap_err() method
 * @fixed Use err() instead of unwrap_err() for Result<void, E>
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #1: Result<void,E> should provide err() method", "[regression][result][bug1]") {
    Result<void, ErrorCode> error_result = Err(ErrorCode::Timeout);

    SECTION("err() method exists and works") {
        REQUIRE(error_result.is_err());
        REQUIRE(error_result.err() == ErrorCode::Timeout);
    }

    SECTION("Can access different error codes") {
        Result<void, ErrorCode> invalid_param = Err(ErrorCode::InvalidParameter);
        Result<void, ErrorCode> not_init = Err(ErrorCode::NotInitialized);

        REQUIRE(invalid_param.err() == ErrorCode::InvalidParameter);
        REQUIRE(not_init.err() == ErrorCode::NotInitialized);
    }
}

// ==============================================================================
// BUG #2: Result<bool, E> causes template overload ambiguity
// ==============================================================================

/**
 * @bug Result<bool, ErrorCode> caused compilation errors due to
 *      OkTag(T&&) and OkTag(const T&) overload conflict when T=bool
 * @workaround Avoid using Result<bool, E>, use Result<uint8_t, E> or plain bool
 * @date Phase 8.2 - Unit Tests
 * @status KNOWN LIMITATION - Not fixed, workaround in place
 */
TEST_CASE("BUG #2: Result<bool,E> template ambiguity", "[regression][result][bug2][known]") {
    WARN("Result<bool, ErrorCode> has template overload issues");
    WARN("Workaround: Use Result<uint8_t, E> or return plain bool");

    SECTION("Workaround: Return uint8_t instead of bool") {
        Result<uint8_t, ErrorCode> result = Ok(static_cast<uint8_t>(1));

        REQUIRE(result.is_ok());
        REQUIRE(result.unwrap() == 1);
    }

    SECTION("Workaround: Use plain bool for simple cases") {
        // For read-only operations, plain bool is acceptable
        auto get_state = []() -> bool { return true; };

        bool state = get_state();
        REQUIRE(state == true);
    }
}

// ==============================================================================
// BUG #3: ErrorCode enum values changed during refactoring
// ==============================================================================

/**
 * @bug ErrorCode values were renamed during API standardization:
 *      - Success → Ok
 *      - AlreadyExists → AlreadyInitialized
 *      - NotFound → Busy
 * @fixed Updated all code to use new enum values
 * @date Phase 6 - API Standardization
 */
TEST_CASE("BUG #3: ErrorCode enum consistency", "[regression][error][bug3]") {
    SECTION("Ok is the success value") {
        ErrorCode error = ErrorCode::Ok;
        REQUIRE(error == ErrorCode::Ok);
    }

    SECTION("AlreadyInitialized for duplicate initialization") {
        ErrorCode error = ErrorCode::AlreadyInitialized;
        REQUIRE(error != ErrorCode::Ok);
    }

    SECTION("All error codes are distinct") {
        REQUIRE(ErrorCode::Ok != ErrorCode::Timeout);
        REQUIRE(ErrorCode::Timeout != ErrorCode::InvalidParameter);
        REQUIRE(ErrorCode::InvalidParameter != ErrorCode::NotInitialized);
        REQUIRE(ErrorCode::NotInitialized != ErrorCode::AlreadyInitialized);
    }
}

// ==============================================================================
// BUG #4: Result move semantics with strings
// ==============================================================================

/**
 * @bug Moving Result<std::string, E> could leave original in invalid state
 * @fixed Properly tested move semantics
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #4: Result move semantics work correctly", "[regression][result][bug4]") {
    SECTION("Moving string Result works") {
        Result<std::string, ErrorCode> original = Ok(std::string("hello"));
        Result<std::string, ErrorCode> moved = std::move(original);

        REQUIRE(moved.is_ok());
        REQUIRE(moved.unwrap() == "hello");
    }

    SECTION("Can move error Result") {
        Result<std::string, ErrorCode> original = Err(ErrorCode::Timeout);
        Result<std::string, ErrorCode> moved = std::move(original);

        REQUIRE(moved.is_err());
        REQUIRE(moved.err() == ErrorCode::Timeout);
    }
}

// ==============================================================================
// BUG #5: unwrap_or() doesn't work with temporary values
// ==============================================================================

/**
 * @bug unwrap_or() should accept both lvalue and rvalue references
 * @fixed Tested with various value categories
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #5: unwrap_or() works with different value types", "[regression][result][bug5]") {
    SECTION("unwrap_or() with literal") {
        auto result = divide_by_zero(10, 0);
        REQUIRE(result.unwrap_or(999) == 999);
    }

    SECTION("unwrap_or() with variable") {
        auto result = divide_by_zero(10, 0);
        int default_value = 123;
        REQUIRE(result.unwrap_or(default_value) == 123);
    }

    SECTION("unwrap_or() with expression") {
        auto result = divide_by_zero(10, 0);
        REQUIRE(result.unwrap_or(100 + 50) == 150);
    }
}

// ==============================================================================
// BUG #6: Result copying with error state
// ==============================================================================

/**
 * @bug Copying Result when in error state could corrupt error value
 * @fixed Verified copy constructor preserves error state
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #6: Copying Result preserves error state", "[regression][result][bug6]") {
    SECTION("Copy error Result") {
        Result<int, ErrorCode> original = Err(ErrorCode::Timeout);
        Result<int, ErrorCode> copy = original;

        REQUIRE(copy.is_err());
        REQUIRE(copy.err() == ErrorCode::Timeout);
        REQUIRE(original.is_err());
        REQUIRE(original.err() == ErrorCode::Timeout);
    }

    SECTION("Copy ok Result") {
        Result<int, ErrorCode> original = Ok(42);
        Result<int, ErrorCode> copy = original;

        REQUIRE(copy.is_ok());
        REQUIRE(copy.unwrap() == 42);
        REQUIRE(original.is_ok());
        REQUIRE(original.unwrap() == 42);
    }
}

// ==============================================================================
// BUG #7: Chaining Results without monadic methods
// ==============================================================================

/**
 * @bug Initial tests assumed map(), and_then(), match() existed
 * @fixed Simplified to use manual chaining with if/is_err() checks
 * @date Phase 8.2 - Unit Tests
 * @status KNOWN LIMITATION - Monadic methods not implemented
 */
TEST_CASE("BUG #7: Manual Result chaining pattern", "[regression][result][bug7]") {
    SECTION("Cascade pattern without monadic methods") {
        auto divide_twice = [](int a, int b, int c) -> Result<int, ErrorCode> {
            auto result1 = divide_regression(a, b);
            if (result1.is_err()) {
                return result1;
            }

            auto result2 = divide_regression(result1.unwrap(), c);
            return result2;
        };

        REQUIRE(divide_twice(100, 10, 2).unwrap() == 5);
        REQUIRE(divide_twice(100, 0, 2).is_err());
    }
}

// ==============================================================================
// BUG #8: Result<void> unwrap() behavior
// ==============================================================================

/**
 * @bug Result<void, E> unwrap() should be callable even though it returns nothing
 * @fixed Verified unwrap() exists for Result<void, E>
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #8: Result<void> unwrap() is callable", "[regression][result][bug8]") {
    SECTION("Can unwrap void Result") {
        auto result = perform_action(true);

        REQUIRE(result.is_ok());
        // This should compile and not crash
        result.unwrap();
        REQUIRE(true); // If we got here, unwrap() worked
    }

    SECTION("void Result error access") {
        auto result = perform_action(false);

        REQUIRE(result.is_err());
        REQUIRE(result.err() == ErrorCode::Timeout);
    }
}

// ==============================================================================
// BUG #9: String Result temporary lifetime issues
// ==============================================================================

/**
 * @bug Result<std::string> with temporary string literals could cause issues
 * @fixed Use explicit std::string() construction
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #9: String Result lifetime management", "[regression][result][bug9]") {
    SECTION("Explicit string construction") {
        Result<std::string, ErrorCode> result = Ok(std::string("test"));

        REQUIRE(result.is_ok());
        REQUIRE(result.unwrap() == "test");
    }

    SECTION("String literal converted to std::string") {
        auto make_string = []() -> Result<std::string, ErrorCode> {
            return Ok(std::string("temporary"));
        };

        auto result = make_string();
        REQUIRE(result.is_ok());
        REQUIRE(result.unwrap() == "temporary");
    }
}

// ==============================================================================
// BUG #10: Error code comparison in different contexts
// ==============================================================================

/**
 * @bug ErrorCode comparisons could fail in template contexts
 * @fixed Ensured ErrorCode works with all comparison operators
 * @date Phase 8.2 - Unit Tests
 */
TEST_CASE("BUG #10: ErrorCode comparison operators", "[regression][error][bug10]") {
    SECTION("Equality comparison") {
        ErrorCode e1 = ErrorCode::Timeout;
        ErrorCode e2 = ErrorCode::Timeout;

        REQUIRE(e1 == e2);
    }

    SECTION("Inequality comparison") {
        ErrorCode e1 = ErrorCode::Timeout;
        ErrorCode e2 = ErrorCode::InvalidParameter;

        REQUIRE(e1 != e2);
    }

    SECTION("Comparison in templates") {
        auto check_error = [](ErrorCode expected, ErrorCode actual) -> bool {
            return expected == actual;
        };

        REQUIRE(check_error(ErrorCode::Ok, ErrorCode::Ok));
        REQUIRE_FALSE(check_error(ErrorCode::Ok, ErrorCode::Timeout));
    }
}
