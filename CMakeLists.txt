cmake_minimum_required(VERSION 3.25)

# Export compile commands for IDE/LSP support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project definition
project(alloy
    VERSION 0.1.0
    DESCRIPTION "The modern C++20 framework for bare-metal embedded systems"
    LANGUAGES CXX C ASM
)

# Require C++20 standard (no modules)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Alloy configuration variables
set(ALLOY_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Alloy framework root directory")
set(ALLOY_BOARD "host" CACHE STRING "Target board (host, rp_pico, bluepill, cf_rl78, esp32_devkit)")
set(ALLOY_MCU "" CACHE STRING "Target MCU (set by board configuration)")
set(ALLOY_ARCH "" CACHE STRING "Target architecture (set by board configuration)")

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(compiler_options)

# Testing support
option(ALLOY_BUILD_TESTS "Build unit tests" ON)

if(ALLOY_BUILD_TESTS)
    enable_testing()

    # Fetch Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
endif()

# Load board configuration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/boards/${ALLOY_BOARD}.cmake)
    message(STATUS "Loading board: ${ALLOY_BOARD}")
    include(cmake/boards/${ALLOY_BOARD}.cmake)
else()
    message(WARNING "Board '${ALLOY_BOARD}' not found, using defaults")
endif()

# Display configuration
message(STATUS "========================================")
message(STATUS "Alloy Framework Configuration")
message(STATUS "========================================")
message(STATUS "  Board       : ${ALLOY_BOARD}")
message(STATUS "  MCU         : ${ALLOY_MCU}")
message(STATUS "  Architecture: ${ALLOY_ARCH}")
message(STATUS "  Build Type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "========================================")

# Add subdirectories
# add_subdirectory(src/core)

# HAL subdirectories (conditional based on board)
if(ALLOY_BOARD STREQUAL "host")
    add_subdirectory(src/hal/host)
endif()

# add_subdirectory(src/drivers)

# Examples
add_subdirectory(examples/blinky)
# add_subdirectory(examples/portable_blinky)  # Portable example for all MCUs

# Tests
if(ALLOY_BUILD_TESTS)
    add_subdirectory(tests/unit)
endif()

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
