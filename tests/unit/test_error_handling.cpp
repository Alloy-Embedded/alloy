/// Unit tests for core error handling
///
/// Tests the Result<T, ErrorCode> type and error handling mechanisms.

#include "core/error.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace alloy::core;

// Test fixture for error handling tests
class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/// Test: Result success case with integer
TEST_F(ErrorHandlingTest, ResultOkInteger) {
    // Given/When: Creating a successful Result
    Result<int> result = Result<int>::ok(42);

    // Then: Result should be in OK state
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_error());
    EXPECT_EQ(result.value(), 42);
}

/// Test: Result error case
TEST_F(ErrorHandlingTest, ResultError) {
    // Given/When: Creating a failed Result
    Result<int> result = Result<int>::error(ErrorCode::Timeout);

    // Then: Result should be in error state
    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), ErrorCode::Timeout);
}

/// Test: Result with different error codes
TEST_F(ErrorHandlingTest, DifferentErrorCodes) {
    Result<int> r1 = Result<int>::error(ErrorCode::InvalidParameter);
    EXPECT_EQ(r1.error(), ErrorCode::InvalidParameter);

    Result<int> r2 = Result<int>::error(ErrorCode::Busy);
    EXPECT_EQ(r2.error(), ErrorCode::Busy);

    Result<int> r3 = Result<int>::error(ErrorCode::NotSupported);
    EXPECT_EQ(r3.error(), ErrorCode::NotSupported);

    Result<int> r4 = Result<int>::error(ErrorCode::HardwareError);
    EXPECT_EQ(r4.error(), ErrorCode::HardwareError);
}

/// Test: Result copy constructor (success case)
TEST_F(ErrorHandlingTest, ResultCopyConstructorSuccess) {
    // Given: A successful Result
    Result<int> original = Result<int>::ok(100);

    // When: Copying the Result
    Result<int> copy = original;

    // Then: Both should be OK with same value
    EXPECT_TRUE(copy.is_ok());
    EXPECT_EQ(copy.value(), 100);
    EXPECT_TRUE(original.is_ok());
    EXPECT_EQ(original.value(), 100);
}

/// Test: Result copy constructor (error case)
TEST_F(ErrorHandlingTest, ResultCopyConstructorError) {
    // Given: A failed Result
    Result<int> original = Result<int>::error(ErrorCode::Timeout);

    // When: Copying the Result
    Result<int> copy = original;

    // Then: Both should have the same error
    EXPECT_TRUE(copy.is_error());
    EXPECT_EQ(copy.error(), ErrorCode::Timeout);
    EXPECT_TRUE(original.is_error());
    EXPECT_EQ(original.error(), ErrorCode::Timeout);
}

/// Test: Result move constructor (success case)
TEST_F(ErrorHandlingTest, ResultMoveConstructorSuccess) {
    // Given: A successful Result
    Result<int> original = Result<int>::ok(200);

    // When: Moving the Result
    Result<int> moved = std::move(original);

    // Then: Moved Result should have the value
    EXPECT_TRUE(moved.is_ok());
    EXPECT_EQ(moved.value(), 200);
}

/// Test: Result move constructor (error case)
TEST_F(ErrorHandlingTest, ResultMoveConstructorError) {
    // Given: A failed Result
    Result<int> original = Result<int>::error(ErrorCode::Busy);

    // When: Moving the Result
    Result<int> moved = std::move(original);

    // Then: Moved Result should have the error
    EXPECT_TRUE(moved.is_error());
    EXPECT_EQ(moved.error(), ErrorCode::Busy);
}

/// Test: Result copy assignment (success case)
TEST_F(ErrorHandlingTest, ResultCopyAssignmentSuccess) {
    // Given: Two Results
    Result<int> r1 = Result<int>::ok(10);
    Result<int> r2 = Result<int>::error(ErrorCode::Unknown);

    // When: Assigning success to error
    r2 = r1;

    // Then: r2 should now be OK
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.value(), 10);
}

/// Test: Result copy assignment (error case)
TEST_F(ErrorHandlingTest, ResultCopyAssignmentError) {
    // Given: Two Results
    Result<int> r1 = Result<int>::error(ErrorCode::Timeout);
    Result<int> r2 = Result<int>::ok(99);

    // When: Assigning error to success
    r2 = r1;

    // Then: r2 should now be error
    EXPECT_TRUE(r2.is_error());
    EXPECT_EQ(r2.error(), ErrorCode::Timeout);
}

/// Test: Result value_or with success
TEST_F(ErrorHandlingTest, ValueOrWithSuccess) {
    // Given: A successful Result
    Result<int> result = Result<int>::ok(42);

    // When/Then: value_or should return the actual value
    EXPECT_EQ(result.value_or(0), 42);
}

/// Test: Result value_or with error
TEST_F(ErrorHandlingTest, ValueOrWithError) {
    // Given: A failed Result
    Result<int> result = Result<int>::error(ErrorCode::Timeout);

    // When/Then: value_or should return the default value
    EXPECT_EQ(result.value_or(999), 999);
}

/// Test: Result with uint32_t type
TEST_F(ErrorHandlingTest, ResultWithUint32) {
    Result<u32> result = Result<u32>::ok(0xDEADBEEF);

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 0xDEADBEEF);
}

/// Test: Result with uint8_t type
TEST_F(ErrorHandlingTest, ResultWithUint8) {
    Result<u8> result = Result<u8>::ok(255);

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 255);
}

/// Test: Result<void> success case
TEST_F(ErrorHandlingTest, ResultVoidSuccess) {
    // Given/When: Creating a successful void Result
    Result<void> result = Result<void>::ok();

    // Then: Result should be OK
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_error());
}

/// Test: Result<void> error case
TEST_F(ErrorHandlingTest, ResultVoidError) {
    // Given/When: Creating a failed void Result
    Result<void> result = Result<void>::error(ErrorCode::HardwareError);

    // Then: Result should be error
    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), ErrorCode::HardwareError);
}

/// Test: ErrorCode enumeration values
TEST(ErrorCodeTest, EnumerationValues) {
    // Verify error codes are distinct
    EXPECT_NE(ErrorCode::Ok, ErrorCode::InvalidParameter);
    EXPECT_NE(ErrorCode::Timeout, ErrorCode::Busy);
    EXPECT_NE(ErrorCode::NotSupported, ErrorCode::HardwareError);

    // Verify Ok is 0 (success convention)
    EXPECT_EQ(static_cast<u8>(ErrorCode::Ok), 0);
}

/// Test: Result size is reasonable
TEST(ResultSizeTest, SizeIsReasonable) {
    // Result should not be much larger than the contained type
    // It needs: T + bool (discriminator) + maybe some padding
    EXPECT_LE(sizeof(Result<u8>), 8);
    EXPECT_LE(sizeof(Result<u32>), 16);
    EXPECT_LE(sizeof(Result<void>), 4);
}

/// Example: Function returning Result
static Result<u32> divide(u32 numerator, u32 denominator) {
    if (denominator == 0) {
        return Result<u32>::error(ErrorCode::InvalidParameter);
    }
    return Result<u32>::ok(numerator / denominator);
}

/// Test: Real-world usage pattern
TEST(ResultUsageTest, DivideFunction) {
    // Success case
    auto result1 = divide(10, 2);
    ASSERT_TRUE(result1.is_ok());
    EXPECT_EQ(result1.value(), 5);

    // Error case (divide by zero)
    auto result2 = divide(10, 0);
    ASSERT_TRUE(result2.is_error());
    EXPECT_EQ(result2.error(), ErrorCode::InvalidParameter);

    // Using value_or
    EXPECT_EQ(result1.value_or(0), 5);
    EXPECT_EQ(result2.value_or(0), 0);
}
