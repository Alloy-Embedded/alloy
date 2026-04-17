# ==============================================================================
# Alloy HAL Integration Tests - Catch2 v3
# ==============================================================================
#
# Integration tests validate the interaction between multiple HAL components
# and platform-specific implementations.
#

include(${CMAKE_SOURCE_DIR}/tests/alloy_test_helpers.cmake)

# ==============================================================================
# Integration Tests
# ==============================================================================

# Test 1: GPIO + Clock Integration
alloy_add_catch2_test(test_gpio_clock_integration
    SOURCES test_gpio_clock_integration.cpp
    LABELS integration
)

# Test 2: Complete initialization sequence
alloy_add_catch2_test(test_initialization_sequence
    SOURCES test_initialization_sequence.cpp
    LABELS integration
)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "Integration tests configured:")
message(STATUS "  - GPIO + Clock integration")
message(STATUS "  - Initialization sequence")
