# ==============================================================================
# Platform Selection Module
# ==============================================================================
#
# This module provides the platform selection mechanism for Alloy HAL.
# Users specify MICROCORE_PLATFORM at configure time, and this module:
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
#   cmake -DMICROCORE_PLATFORM=same70 -B build-same70
#   cmake -DMICROCORE_PLATFORM=linux -B build-linux
#   cmake -DMICROCORE_PLATFORM=esp32 -B build-esp32
#
# @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
# ==============================================================================

# ------------------------------------------------------------------------------
# Supported Platforms
# ------------------------------------------------------------------------------

set(MICROCORE_SUPPORTED_PLATFORMS
    "same70"    # Atmel/Microchip SAME70 (ARM Cortex-M7)
    "linux"     # Linux/POSIX (for host-based testing)
    "esp32"     # Espressif ESP32 (Xtensa LX6 + ESP-IDF)
    "stm32g0"   # STMicroelectronics STM32G0 (ARM Cortex-M0+)
    "stm32f4"   # STMicroelectronics STM32F4 (ARM Cortex-M4)
    "stm32f7"   # STMicroelectronics STM32F7 (ARM Cortex-M7)
    # Future platforms:
    # "stm32h7"   # STMicroelectronics STM32H7 (ARM Cortex-M7)
    # "nrf52"     # Nordic nRF52 (ARM Cortex-M4)
    # "esp32c3"   # Espressif ESP32-C3 (RISC-V)
    # "rp2040"    # Raspberry Pi RP2040 (ARM Cortex-M0+)
)

# ------------------------------------------------------------------------------
# Platform Validation
# ------------------------------------------------------------------------------

# Check if MICROCORE_PLATFORM is defined
if(NOT DEFINED MICROCORE_PLATFORM)
    message(FATAL_ERROR
        "MICROCORE_PLATFORM is not set!\n"
        "Please specify a platform using: -DMICROCORE_PLATFORM=<platform>\n"
        "Supported platforms: ${MICROCORE_SUPPORTED_PLATFORMS}\n"
        "Example: cmake -DMICROCORE_PLATFORM=same70 -B build-same70"
    )
endif()

# Convert platform name to lowercase for consistency
string(TOLOWER "${MICROCORE_PLATFORM}" MICROCORE_PLATFORM)

# Check if platform is supported
list(FIND MICROCORE_SUPPORTED_PLATFORMS "${MICROCORE_PLATFORM}" PLATFORM_INDEX)
if(PLATFORM_INDEX EQUAL -1)
    message(FATAL_ERROR
        "Unsupported platform: '${MICROCORE_PLATFORM}'\n"
        "Supported platforms: ${MICROCORE_SUPPORTED_PLATFORMS}\n"
        "Example: cmake -DMICROCORE_PLATFORM=same70 -B build-same70"
    )
endif()

message(STATUS "Alloy Platform: ${MICROCORE_PLATFORM}")

# ------------------------------------------------------------------------------
# Platform-Specific Configuration
# ------------------------------------------------------------------------------

# Set platform-specific compile definition (used by #ifdef in source code)
string(TOUPPER "${MICROCORE_PLATFORM}" MICROCORE_PLATFORM_UPPER)
set(MICROCORE_PLATFORM_DEFINE "MICROCORE_PLATFORM_${MICROCORE_PLATFORM_UPPER}")

# Add compile definition globally
add_compile_definitions(${MICROCORE_PLATFORM_DEFINE})

message(STATUS "Platform define: ${MICROCORE_PLATFORM_DEFINE}")

# ------------------------------------------------------------------------------
# Include Platform-Specific CMake Configuration
# ------------------------------------------------------------------------------

# Platform configuration files are located in: cmake/platforms/<platform>.cmake
set(PLATFORM_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/platforms/${MICROCORE_PLATFORM}.cmake")

if(NOT EXISTS "${PLATFORM_CONFIG_FILE}")
    message(FATAL_ERROR
        "Platform configuration file not found: ${PLATFORM_CONFIG_FILE}\n"
        "Expected location: cmake/platforms/${MICROCORE_PLATFORM}.cmake\n"
        "This file should define platform-specific settings (toolchain, flags, etc.)"
    )
endif()

# Include the platform-specific configuration
include("${PLATFORM_CONFIG_FILE}")

message(STATUS "Included platform config: ${PLATFORM_CONFIG_FILE}")

# ------------------------------------------------------------------------------
# Board/MCU Validation
# ------------------------------------------------------------------------------

# Define valid board-to-platform mappings
# This ensures users don't accidentally try to build incompatible combinations
set(BOARD_TO_PLATFORM_nucleo_f401re "stm32f4")
set(BOARD_TO_PLATFORM_nucleo_f722ze "stm32f7")
set(BOARD_TO_PLATFORM_nucleo_g071rb "stm32g0")
set(BOARD_TO_PLATFORM_nucleo_g0b1re "stm32g0")
set(BOARD_TO_PLATFORM_same70_xplained "same70")

# If MICROCORE_BOARD is defined, validate it matches the selected platform
if(DEFINED MICROCORE_BOARD)
    set(EXPECTED_PLATFORM ${BOARD_TO_PLATFORM_${MICROCORE_BOARD}})

    if(NOT DEFINED EXPECTED_PLATFORM)
        message(WARNING
            "Unknown board: '${MICROCORE_BOARD}'\n"
            "Board-to-platform mapping not defined.\n"
            "Proceeding with platform: ${MICROCORE_PLATFORM}\n"
            "Known boards: nucleo_f401re, nucleo_f722ze, nucleo_g071rb, nucleo_g0b1re, same70_xplained"
        )
    elseif(NOT "${EXPECTED_PLATFORM}" STREQUAL "${MICROCORE_PLATFORM}")
        message(FATAL_ERROR
            "Board/Platform mismatch!\n"
            "  Board:            ${MICROCORE_BOARD}\n"
            "  Expected Platform: ${EXPECTED_PLATFORM}\n"
            "  Selected Platform: ${MICROCORE_PLATFORM}\n"
            "\n"
            "Please use the correct platform for this board:\n"
            "  cmake -DMICROCORE_BOARD=${MICROCORE_BOARD} -DMICROCORE_PLATFORM=${EXPECTED_PLATFORM} -B build-${MICROCORE_BOARD}"
        )
    endif()

    message(STATUS "Board: ${MICROCORE_BOARD} (validated against platform ${MICROCORE_PLATFORM})")
endif()

# ------------------------------------------------------------------------------
# Platform Source Files
# ------------------------------------------------------------------------------

# Platform implementations are located in: src/hal/vendors/<vendor>/<family>/
# The platform directory is determined by the platform-specific config file
# (e.g., cmake/platforms/stm32f4.cmake sets MICROCORE_PLATFORM_DIR)

# Note: MICROCORE_PLATFORM_DIR should be set by the platform-specific CMake file
# If not set, we'll use a fallback (though this may not work for all platforms)
if(NOT DEFINED MICROCORE_PLATFORM_DIR)
    # Fallback: try to auto-detect based on platform name
    # This is a temporary solution - platforms should define their own MICROCORE_PLATFORM_DIR
    message(FATAL_ERROR
        "MICROCORE_PLATFORM_DIR not set by platform config!\n"
        "Platform: ${MICROCORE_PLATFORM}\n"
        "Config file: ${PLATFORM_CONFIG_FILE}\n"
        "\n"
        "The platform configuration file must set MICROCORE_PLATFORM_DIR.\n"
        "Example: set(MICROCORE_PLATFORM_DIR \${CMAKE_CURRENT_SOURCE_DIR}/src/hal/vendors/st/stm32f4)"
    )
endif()

# Collect all platform-specific source files (*.cpp)
# EXCLUDE startup.cpp files - they are board-specific and added via STARTUP_SOURCE
# NOTE: Only collect .cpp from the SPECIFIC platform directory, not subdirectories with other MCU variants
file(GLOB MICROCORE_PLATFORM_SOURCES
    "${MICROCORE_PLATFORM_DIR}/*.cpp"
)

# Remove all startup*.cpp files from platform sources (board-specific)
list(FILTER MICROCORE_PLATFORM_SOURCES EXCLUDE REGEX ".*startup.*\\.cpp$")

# Collect all platform-specific header files (*.hpp, *.h)
file(GLOB_RECURSE MICROCORE_PLATFORM_HEADERS
    "${MICROCORE_PLATFORM_DIR}/*.hpp"
    "${MICROCORE_PLATFORM_DIR}/*.h"
)

# Export variables for use by parent CMakeLists.txt
set(MICROCORE_PLATFORM_SOURCES ${MICROCORE_PLATFORM_SOURCES} PARENT_SCOPE)
set(MICROCORE_PLATFORM_HEADERS ${MICROCORE_PLATFORM_HEADERS} PARENT_SCOPE)
set(MICROCORE_PLATFORM_DIR ${MICROCORE_PLATFORM_DIR} PARENT_SCOPE)

message(STATUS "Platform sources: ${MICROCORE_PLATFORM_SOURCES}")
message(STATUS "Platform headers: ${MICROCORE_PLATFORM_HEADERS}")

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
message(STATUS "  Platform:         ${MICROCORE_PLATFORM}")
message(STATUS "  Platform Define:  ${MICROCORE_PLATFORM_DEFINE}")
message(STATUS "  Platform Dir:     ${MICROCORE_PLATFORM_DIR}")
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
    foreach(PLATFORM ${MICROCORE_SUPPORTED_PLATFORMS})
        if(NOT "${PLATFORM}" STREQUAL "${MICROCORE_PLATFORM}")
            foreach(SOURCE ${TARGET_SOURCES})
                string(FIND "${SOURCE}" "platform/${PLATFORM}/" FOUND_INDEX)
                if(NOT FOUND_INDEX EQUAL -1)
                    message(WARNING
                        "Target '${TARGET_NAME}' includes source from wrong platform:\n"
                        "  Expected: platform/${MICROCORE_PLATFORM}/\n"
                        "  Found:    ${SOURCE}\n"
                        "This violates the zero-overhead guarantee!"
                    )
                endif()
            endforeach()
        endif()
    endforeach()
endfunction()
