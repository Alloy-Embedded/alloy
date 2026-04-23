# ==============================================================================
# Alloy HAL Unit Tests - Catch2 Framework
# ==============================================================================
#
# Modern C++20 unit tests using Catch2 v3
#
# Test Organization:
#   - core/      : Core utilities (Result<T,E>, error handling)
#   - hal/       : HAL interface tests (concepts, APIs)
#   - platform/  : Platform-specific tests
#
# Running Tests:
#   cmake -B build-tests -DALLOY_BOARD=host -DALLOY_BUILD_TESTS=ON
#   cmake --build build-tests
#   cd build-tests && ctest --output-on-failure
#
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

include(${CMAKE_SOURCE_DIR}/tests/alloy_test_helpers.cmake)

# ==============================================================================
# Core Tests
# ==============================================================================

# Test: Result<T, E> Type
alloy_add_catch2_test(test_result
    SOURCES test_result.cpp
    LABELS unit
)

# Test: Error Handling
alloy_add_catch2_test(test_error
    SOURCES test_error.cpp
    LABELS unit
)

# ==============================================================================
# HAL Concept Tests (C++20)
# ==============================================================================

# Test: GpioPin Concept
alloy_add_catch2_test(test_gpio_concept
    SOURCES test_gpio_concept.cpp
    LABELS unit
)

# Test: ClockPlatform Concept
alloy_add_catch2_test(test_clock_concept
    SOURCES test_clock_concept.cpp
    LABELS unit
)

# Test: Logger Core and Sink Integration
alloy_add_catch2_test(test_logger
    SOURCES test_logger.cpp
    LABELS unit
)

# ==============================================================================
# Test Summary
# ==============================================================================

message(STATUS "========================================")
message(STATUS "Unit Tests Configured:")
message(STATUS "  ✓ Core: Result<T,E>, Error")
message(STATUS "  ✓ HAL:  GPIO, Clock Concepts")
message(STATUS "  ✓ Utility: Logger core")
message(STATUS "========================================")
