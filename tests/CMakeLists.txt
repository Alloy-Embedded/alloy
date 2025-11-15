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
# Test Summary
# ==============================================================================

message(STATUS "========================================")
message(STATUS "Alloy HAL Test Suite Configured")
message(STATUS "========================================")
message(STATUS "Run tests:")
message(STATUS "  cd build-tests && ctest --output-on-failure")
message(STATUS "========================================")
