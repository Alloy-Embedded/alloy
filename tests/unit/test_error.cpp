/**
 * @file test_error.cpp
 * @brief Unit tests for ErrorCode enum
 *
 * Tests the error code system used throughout Alloy HAL.
 */

#include <catch2/catch_test_macros.hpp>

#include "core/error.hpp"

using namespace alloy::core;

// ==============================================================================
// ErrorCode Tests
// ==============================================================================

TEST_CASE("ErrorCode has Ok value", "[error][core]") {
    ErrorCode error = ErrorCode::Ok;

    REQUIRE(error == ErrorCode::Ok);
}

TEST_CASE("ErrorCode has common error types", "[error][core]") {
    REQUIRE(ErrorCode::Timeout != ErrorCode::Ok);
    REQUIRE(ErrorCode::InvalidParameter != ErrorCode::Ok);
    REQUIRE(ErrorCode::NotInitialized != ErrorCode::Ok);
    REQUIRE(ErrorCode::AlreadyInitialized != ErrorCode::Ok);
}

TEST_CASE("ErrorCode values are distinct", "[error][core]") {
    REQUIRE(ErrorCode::Timeout != ErrorCode::InvalidParameter);
    REQUIRE(ErrorCode::NotInitialized != ErrorCode::Busy);
    REQUIRE(ErrorCode::AlreadyInitialized != ErrorCode::Unknown);
}

TEST_CASE("ErrorCode can be compared", "[error][core]") {
    ErrorCode error1 = ErrorCode::Timeout;
    ErrorCode error2 = ErrorCode::Timeout;
    ErrorCode error3 = ErrorCode::InvalidParameter;

    REQUIRE(error1 == error2);
    REQUIRE(error1 != error3);
}

TEST_CASE("ErrorCode can be used in switch", "[error][core]") {
    ErrorCode error = ErrorCode::Timeout;
    int result = 0;

    switch (error) {
        case ErrorCode::Ok:
            result = 1;
            break;
        case ErrorCode::Timeout:
            result = 2;
            break;
        case ErrorCode::InvalidParameter:
            result = 3;
            break;
        default:
            result = 99;
            break;
    }

    REQUIRE(result == 2);
}

// ==============================================================================
// Error Scenarios
// ==============================================================================

TEST_CASE("Typical HAL error scenarios", "[error][core][integration]") {
    SECTION("GPIO pin already configured") {
        ErrorCode error = ErrorCode::AlreadyInitialized;
        REQUIRE(error != ErrorCode::Ok);
    }

    SECTION("UART timeout during transmission") {
        ErrorCode error = ErrorCode::Timeout;
        REQUIRE(error == ErrorCode::Timeout);
    }

    SECTION("Invalid SPI baud rate") {
        ErrorCode error = ErrorCode::InvalidParameter;
        REQUIRE(error == ErrorCode::InvalidParameter);
    }

    SECTION("I2C device not initialized") {
        ErrorCode error = ErrorCode::NotInitialized;
        REQUIRE(error == ErrorCode::NotInitialized);
    }
}
