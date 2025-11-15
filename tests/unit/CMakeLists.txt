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

# Include Catch2 helpers
include(Catch)

# ==============================================================================
# Helper Function: Add Catch2 Test
# ==============================================================================

function(add_catch2_test TEST_NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    add_executable(${TEST_NAME} ${ARG_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE
        Catch2::Catch2WithMain
        ${ARG_LIBS}
    )
    target_compile_options(${TEST_NAME} PRIVATE -std=c++20)
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    catch_discover_tests(${TEST_NAME})
endfunction()

# ==============================================================================
# Core Tests
# ==============================================================================

# Test: Result<T, E> Type
add_catch2_test(test_result
    SOURCES test_result.cpp
)

# Test: Error Handling
add_catch2_test(test_error
    SOURCES test_error.cpp
)

# ==============================================================================
# HAL Concept Tests (C++20)
# ==============================================================================

# Test: GpioPin Concept
add_catch2_test(test_gpio_concept
    SOURCES test_gpio_concept.cpp
)

# Test: ClockPlatform Concept
add_catch2_test(test_clock_concept
    SOURCES test_clock_concept.cpp
)

# ==============================================================================
# Test Summary
# ==============================================================================

message(STATUS "========================================")
message(STATUS "Unit Tests Configured:")
message(STATUS "  ✓ Core: Result<T,E>, Error")
message(STATUS "  ✓ HAL:  GPIO, Clock Concepts")
message(STATUS "========================================")
