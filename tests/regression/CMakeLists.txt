# ==============================================================================
# Alloy HAL Regression Tests - Catch2 v3
# ==============================================================================
#
# Regression tests capture known bugs and ensure they don't reoccur.
# Each test documents a specific bug that was found and fixed.
#

# ==============================================================================
# Helper Function
# ==============================================================================

function(add_regression_test TEST_NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    add_executable(${TEST_NAME} ${ARG_SOURCES})

    target_link_libraries(${TEST_NAME} PRIVATE
        Catch2::Catch2WithMain
        ${ARG_LIBS}
    )

    target_compile_options(${TEST_NAME} PRIVATE
        -std=c++20
        -Wall
        -Wextra
    )

    # Add platform-specific include paths
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
    )

    catch_discover_tests(${TEST_NAME})
endfunction()

# ==============================================================================
# Regression Tests
# ==============================================================================

# Regression Test 1: Result<T,E> edge cases
add_regression_test(test_result_regressions
    SOURCES test_result_regressions.cpp
)

# Regression Test 2: GPIO/Clock initialization bugs
add_regression_test(test_gpio_clock_regressions
    SOURCES test_gpio_clock_regressions.cpp
)

# Regression Test 3: Concept validation issues
add_regression_test(test_concept_regressions
    SOURCES test_concept_regressions.cpp
)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "Regression tests configured:")
message(STATUS "  - Result<T,E> edge cases")
message(STATUS "  - GPIO/Clock initialization bugs")
message(STATUS "  - Concept validation issues")
