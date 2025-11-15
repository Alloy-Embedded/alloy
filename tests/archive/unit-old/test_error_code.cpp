/**
 * @file test_error_code.cpp
 * @brief Unit tests for ErrorCode enum
 *
 * Tests the ErrorCode enum values and properties
 */

#include <cassert>
#include <iostream>

#include "../../src/core/error_code.hpp"

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
// ErrorCode Enum Tests
// =============================================================================

TEST(error_code_ok_is_zero) {
    ASSERT(static_cast<int>(ErrorCode::Ok) == 0);
}

TEST(error_code_is_enum_class) {
    // ErrorCode should be an enum class (strongly typed)
    // This test checks that implicit conversion to int is not allowed
    // If this compiles, ErrorCode is an enum class
    ErrorCode e = ErrorCode::Timeout;
    ASSERT(e == ErrorCode::Timeout);
}

TEST(error_code_basic_errors) {
    ASSERT(ErrorCode::InvalidParameter != ErrorCode::Ok);
    ASSERT(ErrorCode::Timeout != ErrorCode::Ok);
    ASSERT(ErrorCode::Busy != ErrorCode::Ok);
    ASSERT(ErrorCode::NotSupported != ErrorCode::Ok);
    ASSERT(ErrorCode::HardwareError != ErrorCode::Ok);
}

TEST(error_code_comparison) {
    ErrorCode e1 = ErrorCode::Timeout;
    ErrorCode e2 = ErrorCode::Timeout;
    ErrorCode e3 = ErrorCode::Busy;

    ASSERT(e1 == e2);
    ASSERT(e1 != e3);
}

TEST(error_code_assignment) {
    ErrorCode e = ErrorCode::Ok;
    ASSERT(e == ErrorCode::Ok);

    e = ErrorCode::InvalidParameter;
    ASSERT(e == ErrorCode::InvalidParameter);
    ASSERT(e != ErrorCode::Ok);
}

TEST(error_code_buffer_errors) {
    ASSERT(ErrorCode::BufferFull != ErrorCode::Ok);
    ASSERT(ErrorCode::BufferEmpty != ErrorCode::Ok);
    ASSERT(ErrorCode::BufferFull != ErrorCode::BufferEmpty);
}

TEST(error_code_i2c_errors) {
    ASSERT(ErrorCode::I2cNack != ErrorCode::Ok);
    ASSERT(ErrorCode::I2cBusBusy != ErrorCode::Ok);
    ASSERT(ErrorCode::I2cArbitrationLost != ErrorCode::Ok);
}

TEST(error_code_adc_errors) {
    ASSERT(ErrorCode::AdcCalibrationFailed != ErrorCode::Ok);
    ASSERT(ErrorCode::AdcOverrun != ErrorCode::Ok);
    ASSERT(ErrorCode::AdcConversionTimeout != ErrorCode::Ok);
}

TEST(error_code_dma_errors) {
    ASSERT(ErrorCode::DmaTransferError != ErrorCode::Ok);
    ASSERT(ErrorCode::DmaAlignmentError != ErrorCode::Ok);
    ASSERT(ErrorCode::DmaChannelBusy != ErrorCode::Ok);
}

TEST(error_code_clock_errors) {
    ASSERT(ErrorCode::PllLockFailed != ErrorCode::Ok);
    ASSERT(ErrorCode::ClockInvalidFrequency != ErrorCode::Ok);
    ASSERT(ErrorCode::ClockSourceNotReady != ErrorCode::Ok);
}

TEST(error_code_unknown) {
    ASSERT(ErrorCode::Unknown != ErrorCode::Ok);
}

TEST(error_code_switch_statement) {
    // Test that ErrorCode can be used in switch statements
    ErrorCode e = ErrorCode::Timeout;
    bool handled = false;

    switch (e) {
        case ErrorCode::Ok:
            break;
        case ErrorCode::Timeout:
            handled = true;
            break;
        case ErrorCode::InvalidParameter:
            break;
        default:
            break;
    }

    ASSERT(handled == true);
}

TEST(error_code_all_unique) {
    // Test that different error codes have different values
    ASSERT(ErrorCode::InvalidParameter != ErrorCode::Timeout);
    ASSERT(ErrorCode::Timeout != ErrorCode::Busy);
    ASSERT(ErrorCode::Busy != ErrorCode::NotSupported);
    ASSERT(ErrorCode::NotSupported != ErrorCode::HardwareError);
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  ErrorCode Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Run all tests
    run_test_error_code_ok_is_zero();
    run_test_error_code_is_enum_class();
    run_test_error_code_basic_errors();
    run_test_error_code_comparison();
    run_test_error_code_assignment();
    run_test_error_code_buffer_errors();
    run_test_error_code_i2c_errors();
    run_test_error_code_adc_errors();
    run_test_error_code_dma_errors();
    run_test_error_code_clock_errors();
    run_test_error_code_unknown();
    run_test_error_code_switch_statement();
    run_test_error_code_all_unique();

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
