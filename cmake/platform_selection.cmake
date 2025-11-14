# ==============================================================================
# Platform Selection Module
# ==============================================================================
#
# This module provides the platform selection mechanism for Alloy HAL.
# Users specify ALLOY_PLATFORM at configure time, and this module:
#   1. Validates the platform choice
#   2. Includes the platform-specific CMake configuration
#   3. Sets up compile definitions and include paths
#   4. Ensures only the selected platform's code is compiled (zero overhead)
#
# Design Principles:
#   - Compile-time selection: Only one platform's code is included in the build
#   - Zero runtime overhead: No vtables, no runtime dispatching
#   - Type-safe: Platform-specific types resolved at compile-time
#   - CMake-based: Standard CMake variable for platform selection
#
# Usage:
#   cmake -DALLOY_PLATFORM=same70 -B build-same70
#   cmake -DALLOY_PLATFORM=linux -B build-linux
#   cmake -DALLOY_PLATFORM=esp32 -B build-esp32
#
# @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
# ==============================================================================

# ------------------------------------------------------------------------------
# Supported Platforms
# ------------------------------------------------------------------------------

set(ALLOY_SUPPORTED_PLATFORMS
    "same70"    # Atmel/Microchip SAME70 (ARM Cortex-M7)
    "linux"     # Linux/POSIX (for host-based testing)
    "esp32"     # Espressif ESP32 (Xtensa LX6 + ESP-IDF)
    "stm32g0"   # STMicroelectronics STM32G0 (ARM Cortex-M0+)
    "stm32f4"   # STMicroelectronics STM32F4 (ARM Cortex-M4)
    # Future platforms:
    # "stm32h7"   # STMicroelectronics STM32H7 (ARM Cortex-M7)
    # "nrf52"     # Nordic nRF52 (ARM Cortex-M4)
    # "esp32c3"   # Espressif ESP32-C3 (RISC-V)
    # "rp2040"    # Raspberry Pi RP2040 (ARM Cortex-M0+)
)

# ------------------------------------------------------------------------------
# Platform Validation
# ------------------------------------------------------------------------------

# Check if ALLOY_PLATFORM is defined
if(NOT DEFINED ALLOY_PLATFORM)
    message(FATAL_ERROR
        "ALLOY_PLATFORM is not set!\n"
        "Please specify a platform using: -DALLOY_PLATFORM=<platform>\n"
        "Supported platforms: ${ALLOY_SUPPORTED_PLATFORMS}\n"
        "Example: cmake -DALLOY_PLATFORM=same70 -B build-same70"
    )
endif()

# Convert platform name to lowercase for consistency
string(TOLOWER "${ALLOY_PLATFORM}" ALLOY_PLATFORM)

# Check if platform is supported
list(FIND ALLOY_SUPPORTED_PLATFORMS "${ALLOY_PLATFORM}" PLATFORM_INDEX)
if(PLATFORM_INDEX EQUAL -1)
    message(FATAL_ERROR
        "Unsupported platform: '${ALLOY_PLATFORM}'\n"
        "Supported platforms: ${ALLOY_SUPPORTED_PLATFORMS}\n"
        "Example: cmake -DALLOY_PLATFORM=same70 -B build-same70"
    )
endif()

message(STATUS "Alloy Platform: ${ALLOY_PLATFORM}")

# ------------------------------------------------------------------------------
# Platform-Specific Configuration
# ------------------------------------------------------------------------------

# Set platform-specific compile definition (used by #ifdef in source code)
string(TOUPPER "${ALLOY_PLATFORM}" ALLOY_PLATFORM_UPPER)
set(ALLOY_PLATFORM_DEFINE "ALLOY_PLATFORM_${ALLOY_PLATFORM_UPPER}")

# Add compile definition globally
add_compile_definitions(${ALLOY_PLATFORM_DEFINE})

message(STATUS "Platform define: ${ALLOY_PLATFORM_DEFINE}")

# ------------------------------------------------------------------------------
# Include Platform-Specific CMake Configuration
# ------------------------------------------------------------------------------

# Platform configuration files are located in: cmake/platforms/<platform>.cmake
set(PLATFORM_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/platforms/${ALLOY_PLATFORM}.cmake")

if(NOT EXISTS "${PLATFORM_CONFIG_FILE}")
    message(FATAL_ERROR
        "Platform configuration file not found: ${PLATFORM_CONFIG_FILE}\n"
        "Expected location: cmake/platforms/${ALLOY_PLATFORM}.cmake\n"
        "This file should define platform-specific settings (toolchain, flags, etc.)"
    )
endif()

# Include the platform-specific configuration
include("${PLATFORM_CONFIG_FILE}")

message(STATUS "Included platform config: ${PLATFORM_CONFIG_FILE}")

# ------------------------------------------------------------------------------
# Platform Source Files
# ------------------------------------------------------------------------------

# Platform implementations are located in: src/hal/platform/<platform>/
set(ALLOY_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/src/hal/platform/${ALLOY_PLATFORM}")

if(NOT EXISTS "${ALLOY_PLATFORM_DIR}")
    message(WARNING
        "Platform directory not found: ${ALLOY_PLATFORM_DIR}\n"
        "Expected location: src/hal/platform/${ALLOY_PLATFORM}/\n"
        "This platform may not be implemented yet."
    )
    # Don't fail here - allow the build to continue for partial implementations
endif()

# Collect all platform-specific source files (*.cpp)
file(GLOB_RECURSE ALLOY_PLATFORM_SOURCES
    "${ALLOY_PLATFORM_DIR}/*.cpp"
)

# Collect all platform-specific header files (*.hpp, *.h)
file(GLOB_RECURSE ALLOY_PLATFORM_HEADERS
    "${ALLOY_PLATFORM_DIR}/*.hpp"
    "${ALLOY_PLATFORM_DIR}/*.h"
)

# Export variables for use by parent CMakeLists.txt
set(ALLOY_PLATFORM_SOURCES ${ALLOY_PLATFORM_SOURCES} PARENT_SCOPE)
set(ALLOY_PLATFORM_HEADERS ${ALLOY_PLATFORM_HEADERS} PARENT_SCOPE)
set(ALLOY_PLATFORM_DIR ${ALLOY_PLATFORM_DIR} PARENT_SCOPE)

message(STATUS "Platform sources: ${ALLOY_PLATFORM_SOURCES}")
message(STATUS "Platform headers: ${ALLOY_PLATFORM_HEADERS}")

# ------------------------------------------------------------------------------
# Platform Include Paths
# ------------------------------------------------------------------------------

# Add platform-specific include path (so code can #include "hal/platform/<platform>/...")
# This is added to the INTERFACE of the alloy-hal target by the parent CMakeLists.txt

# ------------------------------------------------------------------------------
# C++ Standard Requirements
# ------------------------------------------------------------------------------

# Concepts require C++20, but we support C++17 fallback with static_assert
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR
        "Alloy HAL requires at least C++17\n"
        "Current: C++${CMAKE_CXX_STANDARD}\n"
        "Please set: -DCMAKE_CXX_STANDARD=17 or -DCMAKE_CXX_STANDARD=20"
    )
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_STANDARD GREATER_EQUAL 20)
    message(STATUS "C++20 enabled - using concepts for compile-time validation")
else()
    message(STATUS "C++17 enabled - using static_assert for compile-time validation")
endif()

# ------------------------------------------------------------------------------
# Platform Information Summary
# ------------------------------------------------------------------------------

message(STATUS "==============================================================================")
message(STATUS "Alloy HAL Platform Configuration")
message(STATUS "==============================================================================")
message(STATUS "  Platform:         ${ALLOY_PLATFORM}")
message(STATUS "  Platform Define:  ${ALLOY_PLATFORM_DEFINE}")
message(STATUS "  Platform Dir:     ${ALLOY_PLATFORM_DIR}")
message(STATUS "  C++ Standard:     C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "==============================================================================")

# ------------------------------------------------------------------------------
# Compile-Time Validation Helper
# ------------------------------------------------------------------------------

# Function to verify that only the selected platform's code is compiled
# This is called by the parent CMakeLists.txt after all targets are defined
function(alloy_verify_platform_selection TARGET_NAME)
    # Get all source files for the target
    get_target_property(TARGET_SOURCES ${TARGET_NAME} SOURCES)

    # Check that no other platform's sources are included
    foreach(PLATFORM ${ALLOY_SUPPORTED_PLATFORMS})
        if(NOT "${PLATFORM}" STREQUAL "${ALLOY_PLATFORM}")
            foreach(SOURCE ${TARGET_SOURCES})
                string(FIND "${SOURCE}" "platform/${PLATFORM}/" FOUND_INDEX)
                if(NOT FOUND_INDEX EQUAL -1)
                    message(WARNING
                        "Target '${TARGET_NAME}' includes source from wrong platform:\n"
                        "  Expected: platform/${ALLOY_PLATFORM}/\n"
                        "  Found:    ${SOURCE}\n"
                        "This violates the zero-overhead guarantee!"
                    )
                endif()
            endforeach()
        endif()
    endforeach()
endfunction()
