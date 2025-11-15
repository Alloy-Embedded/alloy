# ==============================================================================
# Alloy HAL Test Suite - Catch2 v3
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

# Catch2 is fetched by parent CMakeLists.txt
include(CTest)
include(Catch)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# ==============================================================================
# Unit Tests
# ==============================================================================

# Include unit tests subdirectory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/CMakeLists.txt)
    add_subdirectory(unit)
endif()

# ==============================================================================
# Integration Tests
# ==============================================================================

# Include integration tests subdirectory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt)
    add_subdirectory(integration)
endif()

# ==============================================================================
# Regression Tests
# ==============================================================================

# Include regression tests subdirectory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/regression/CMakeLists.txt)
    add_subdirectory(regression)
endif()

# ==============================================================================
# Hardware Validation Tests
# ==============================================================================

# Include hardware validation tests (only for embedded platforms)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hardware/CMakeLists.txt)
    add_subdirectory(hardware)
endif()

# ==============================================================================
# Test Summary
# ==============================================================================

message(STATUS "========================================")
message(STATUS "Alloy HAL Test Suite Configured")
message(STATUS "========================================")
message(STATUS "Run tests:")
message(STATUS "  cd build-tests && ctest --output-on-failure")
message(STATUS "========================================")
