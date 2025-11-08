/// Unit tests for core error handling
///
/// Tests the Result<T, ErrorCode> type and error handling mechanisms.

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "core/error.hpp"

using namespace alloy::core;

/// Test: Result success case with integer
TEST_CASE("Result success case with integer", "[error][result]") {
    // Given/When: Creating a successful Result
    Result<int> result = Result<int>::ok(42);

    // Then: Result should be in OK state
    REQUIRE(result.is_ok());
    REQUIRE_FALSE(result.is_error());
    REQUIRE(result.value() == 42);
}

/// Test: Result error case
TEST_CASE("Result error case", "[error][result]") {
    // Given/When: Creating a failed Result
    Result<int> result = Result<int>::error(ErrorCode::Timeout);

    // Then: Result should be in error state
    REQUIRE_FALSE(result.is_ok());
    REQUIRE(result.is_error());
    REQUIRE(result.error() == ErrorCode::Timeout);
}

/// Test: Result with different error codes
TEST_CASE("Result with different error codes", "[error][result]") {
    Result<int> r1 = Result<int>::error(ErrorCode::InvalidParameter);
    REQUIRE(r1.error() == ErrorCode::InvalidParameter);

    Result<int> r2 = Result<int>::error(ErrorCode::Busy);
    REQUIRE(r2.error() == ErrorCode::Busy);

    Result<int> r3 = Result<int>::error(ErrorCode::NotSupported);
    REQUIRE(r3.error() == ErrorCode::NotSupported);

    Result<int> r4 = Result<int>::error(ErrorCode::HardwareError);
    REQUIRE(r4.error() == ErrorCode::HardwareError);
}

/// Test: Result copy constructor (success case)
TEST_CASE("Result copy constructor success", "[error][result][copy]") {
    // Given: A successful Result
    Result<int> original = Result<int>::ok(100);

    // When: Copying the Result
    Result<int> copy = original;

    // Then: Both should be OK with same value
    REQUIRE(copy.is_ok());
    REQUIRE(copy.value() == 100);
    REQUIRE(original.is_ok());
    REQUIRE(original.value() == 100);
}

/// Test: Result copy constructor (error case)
TEST_CASE("Result copy constructor error", "[error][result][copy]") {
    // Given: A failed Result
    Result<int> original = Result<int>::error(ErrorCode::Timeout);

    // When: Copying the Result
    Result<int> copy = original;

    // Then: Both should have the same error
    REQUIRE(copy.is_error());
    REQUIRE(copy.error() == ErrorCode::Timeout);
    REQUIRE(original.is_error());
    REQUIRE(original.error() == ErrorCode::Timeout);
}

/// Test: Result move constructor (success case)
TEST_CASE("Result move constructor success", "[error][result][move]") {
    // Given: A successful Result
    Result<int> original = Result<int>::ok(200);

    // When: Moving the Result
    Result<int> moved = std::move(original);

    // Then: Moved Result should have the value
    REQUIRE(moved.is_ok());
    REQUIRE(moved.value() == 200);
}

/// Test: Result move constructor (error case)
TEST_CASE("Result move constructor error", "[error][result][move]") {
    // Given: A failed Result
    Result<int> original = Result<int>::error(ErrorCode::Busy);

    // When: Moving the Result
    Result<int> moved = std::move(original);

    // Then: Moved Result should have the error
    REQUIRE(moved.is_error());
    REQUIRE(moved.error() == ErrorCode::Busy);
}

/// Test: Result copy assignment (success case)
TEST_CASE("Result copy assignment success", "[error][result][assignment]") {
    // Given: Two Results
    Result<int> r1 = Result<int>::ok(10);
    Result<int> r2 = Result<int>::error(ErrorCode::Unknown);

    // When: Assigning success to error
    r2 = r1;

    // Then: r2 should now be OK
    REQUIRE(r2.is_ok());
    REQUIRE(r2.value() == 10);
}

/// Test: Result copy assignment (error case)
TEST_CASE("Result copy assignment error", "[error][result][assignment]") {
    // Given: Two Results
    Result<int> r1 = Result<int>::error(ErrorCode::Timeout);
    Result<int> r2 = Result<int>::ok(99);

    // When: Assigning error to success
    r2 = r1;

    // Then: r2 should now be error
    REQUIRE(r2.is_error());
    REQUIRE(r2.error() == ErrorCode::Timeout);
}

/// Test: Result value_or with success
TEST_CASE("Result value_or with success", "[error][result][value_or]") {
    // Given: A successful Result
    Result<int> result = Result<int>::ok(42);

    // When/Then: value_or should return the actual value
    REQUIRE(result.value_or(0) == 42);
}

/// Test: Result value_or with error
TEST_CASE("Result value_or with error", "[error][result][value_or]") {
    // Given: A failed Result
    Result<int> result = Result<int>::error(ErrorCode::Timeout);

    // When/Then: value_or should return the default value
    REQUIRE(result.value_or(999) == 999);
}

/// Test: Result with uint32_t type
TEST_CASE("Result with uint32_t type", "[error][result][types]") {
    Result<u32> result = Result<u32>::ok(0xDEADBEEF);

    REQUIRE(result.is_ok());
    REQUIRE(result.value() == 0xDEADBEEF);
}

/// Test: Result with uint8_t type
TEST_CASE("Result with uint8_t type", "[error][result][types]") {
    Result<u8> result = Result<u8>::ok(255);

    REQUIRE(result.is_ok());
    REQUIRE(result.value() == 255);
}

/// Test: Result<void> success case
TEST_CASE("Result<void> success case", "[error][result][void]") {
    // Given/When: Creating a successful void Result
    Result<void> result = Result<void>::ok();

    // Then: Result should be OK
    REQUIRE(result.is_ok());
    REQUIRE_FALSE(result.is_error());
}

/// Test: Result<void> error case
TEST_CASE("Result<void> error case", "[error][result][void]") {
    // Given/When: Creating a failed void Result
    Result<void> result = Result<void>::error(ErrorCode::HardwareError);

    // Then: Result should be error
    REQUIRE_FALSE(result.is_ok());
    REQUIRE(result.is_error());
    REQUIRE(result.error() == ErrorCode::HardwareError);
}

/// Test: ErrorCode enumeration values
TEST_CASE("ErrorCode enumeration values", "[error][errorcode]") {
    // Verify error codes are distinct
    REQUIRE(ErrorCode::Ok != ErrorCode::InvalidParameter);
    REQUIRE(ErrorCode::Timeout != ErrorCode::Busy);
    REQUIRE(ErrorCode::NotSupported != ErrorCode::HardwareError);

    // Verify Ok is 0 (success convention)
    REQUIRE(static_cast<u8>(ErrorCode::Ok) == 0);
}

/// Test: Result size is reasonable
TEST_CASE("Result size is reasonable", "[error][result][size]") {
    // Result should not be much larger than the contained type
    // It needs: T + bool (discriminator) + maybe some padding
    REQUIRE(sizeof(Result<u8>) <= 8);
    REQUIRE(sizeof(Result<u32>) <= 16);
    REQUIRE(sizeof(Result<void>) <= 4);
}

/// Example: Function returning Result
static Result<u32> divide(u32 numerator, u32 denominator) {
    if (denominator == 0) {
        return Result<u32>::error(ErrorCode::InvalidParameter);
    }
    return Result<u32>::ok(numerator / denominator);
}

/// Test: Real-world usage pattern
TEST_CASE("Real-world usage - divide function", "[error][result][usage]") {
    SECTION("Success case") {
        auto result1 = divide(10, 2);
        REQUIRE(result1.is_ok());
        REQUIRE(result1.value() == 5);
    }

    SECTION("Error case - divide by zero") {
        auto result2 = divide(10, 0);
        REQUIRE(result2.is_error());
        REQUIRE(result2.error() == ErrorCode::InvalidParameter);
    }

    SECTION("Using value_or") {
        auto result1 = divide(10, 2);
        auto result2 = divide(10, 0);
        REQUIRE(result1.value_or(0) == 5);
        REQUIRE(result2.value_or(0) == 0);
    }
}
