# ==============================================================================
# Alloy HAL Regression Tests - Catch2 v3
# ==============================================================================
#
# Regression tests capture known bugs and ensure they don't reoccur.
# Each test documents a specific bug that was found and fixed.
#

include(${CMAKE_SOURCE_DIR}/tests/alloy_test_helpers.cmake)

# ==============================================================================
# Regression Tests
# ==============================================================================

# Regression Test 1: Result<T,E> edge cases
alloy_add_catch2_test(test_result_regressions
    SOURCES test_result_regressions.cpp
    LABELS regression
)

# Regression Test 2: GPIO/Clock initialization bugs
alloy_add_catch2_test(test_gpio_clock_regressions
    SOURCES test_gpio_clock_regressions.cpp
    LABELS regression
)

# Regression Test 3: Concept validation issues
alloy_add_catch2_test(test_concept_regressions
    SOURCES test_concept_regressions.cpp
    LABELS regression
)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "Regression tests configured:")
message(STATUS "  - Result<T,E> edge cases")
message(STATUS "  - GPIO/Clock initialization bugs")
message(STATUS "  - Concept validation issues")
