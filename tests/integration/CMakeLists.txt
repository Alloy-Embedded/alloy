# ==============================================================================
# Alloy HAL Integration Tests - Catch2 v3
# ==============================================================================
#
# Integration tests validate the interaction between multiple HAL components
# and platform-specific implementations.
#

# ==============================================================================
# Helper Function
# ==============================================================================

function(add_integration_test TEST_NAME)
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
# Integration Tests
# ==============================================================================

# Test 1: GPIO + Clock Integration
add_integration_test(test_gpio_clock_integration
    SOURCES test_gpio_clock_integration.cpp
)

# Test 2: Platform-specific concept validation
add_integration_test(test_platform_concepts
    SOURCES test_platform_concepts.cpp
)

# Test 3: Complete initialization sequence
add_integration_test(test_initialization_sequence
    SOURCES test_initialization_sequence.cpp
)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "Integration tests configured:")
message(STATUS "  - GPIO + Clock integration")
message(STATUS "  - Platform concept validation")
message(STATUS "  - Initialization sequence")
