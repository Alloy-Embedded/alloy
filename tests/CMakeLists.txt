# ==============================================================================
# Alloy HAL Test Suite - Catch2 v3
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

include(CTest)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

if(ALLOY_BOARD STREQUAL "host")
    include(Catch)
    include(${CMAKE_CURRENT_SOURCE_DIR}/alloy_test_helpers.cmake)

    # ==========================================================================
    # Unit Tests
    # ==========================================================================

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/CMakeLists.txt)
        add_subdirectory(unit)
    endif()

    # ==========================================================================
    # Integration Tests
    # ==========================================================================

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration/CMakeLists.txt)
        add_subdirectory(integration)
    endif()

    # ==========================================================================
    # Regression Tests
    # ==========================================================================

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/regression/CMakeLists.txt)
        add_subdirectory(regression)
    endif()

    # ==========================================================================
    # Host MMIO Validation
    # ==========================================================================

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/host_mmio/CMakeLists.txt)
        add_subdirectory(host_mmio)
    endif()
endif()

# ==============================================================================
# ELF / Emulation / Performance Validation Scaffolds
# ==============================================================================

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/elf/CMakeLists.txt)
    add_subdirectory(elf)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/emulation/CMakeLists.txt)
    add_subdirectory(emulation)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/perf/CMakeLists.txt)
    add_subdirectory(perf)
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
message(STATUS "Validation categories:")
if(ALLOY_BOARD STREQUAL "host")
    message(STATUS "  - unit")
    message(STATUS "  - integration")
    message(STATUS "  - regression")
    message(STATUS "  - host-mmio")
else()
    message(STATUS "  - cross-target validation for board ${ALLOY_BOARD}")
endif()
message(STATUS "  - elf")
message(STATUS "  - emulation")
message(STATUS "  - hardware")
message(STATUS "  - perf")
message(STATUS "Run tests:")
message(STATUS "  ctest --test-dir ${CMAKE_BINARY_DIR} --output-on-failure")
message(STATUS "========================================")
