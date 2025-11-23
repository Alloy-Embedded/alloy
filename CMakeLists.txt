cmake_minimum_required(VERSION 3.25)

# Export compile commands for IDE/LSP support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project definition
project(microcore
    VERSION 0.1.0
    DESCRIPTION "The modern C++20 framework for bare-metal embedded systems"
    LANGUAGES CXX C ASM
)

# Require C++23 standard (Phase 5: C++23 Enhancements)
# Enables: consteval, if consteval, deducing this, and other C++23 features
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MicroCore configuration variables
set(MICROCORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "MicroCore framework root directory")
set(MICROCORE_BOARD "host" CACHE STRING "Target board (host, bluepill, esp32_devkit, stm32f407vg, arduino_zero, rp_pico)")
set(MICROCORE_MCU "" CACHE STRING "Target MCU (set by board configuration)")
set(MICROCORE_ARCH "" CACHE STRING "Target architecture (set by board configuration)")

# Platform abstraction configuration (new system)
# MICROCORE_PLATFORM is the HAL platform (same70, linux, esp32, stm32f4, etc.)
# It will be auto-detected from MICROCORE_BOARD if not explicitly set
set(MICROCORE_PLATFORM "" CACHE STRING "HAL platform (same70, linux, esp32, stm32f4, nrf52)")

# Board options
set_property(CACHE MICROCORE_BOARD PROPERTY STRINGS
    "host"
    "bluepill"           # STM32F103C8 Blue Pill
    "esp32_devkit"       # ESP32 DevKit
    "stm32f407vg"        # STM32F407VG Discovery
    "arduino_zero"       # ATSAMD21G18 Arduino Zero
    "rp_pico"            # RP2040 Raspberry Pi Pico
    "rp2040_zero"        # Waveshare RP2040-Zero
    "same70_xpld"        # Atmel SAME70 Xplained
    "nucleo_g0b1re"      # STM32 Nucleo-G0B1RE
    "nucleo_g071rb"      # STM32 Nucleo-G071RB
    "nucleo_f401re"      # STM32 Nucleo-F401RE
    "nucleo_f722ze"      # STM32 Nucleo-F722ZE
)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(dependency_validation)
include(compiler_options)
include(toolchain_check)
include(board_flags)
include(flash_targets)
include(memory_analysis)

# Validate dependencies (checks toolchain, Python, flash tools, etc.)
validate_dependencies()

# ============================================================================
# Board-to-Platform Validation
# ============================================================================
# Validates that board and platform combinations are correct to prevent
# build errors from incompatible configurations.
#
# This table defines valid board → platform mappings.
# If MICROCORE_PLATFORM is not set, it will be auto-detected.
# If it IS set but doesn't match the board, emit a FATAL_ERROR.

set(MICROCORE_BOARD_PLATFORM_MAP
    "nucleo_f401re:stm32f4"
    "nucleo_f722ze:stm32f7"
    "nucleo_g071rb:stm32g0"
    "nucleo_g0b1re:stm32g0"
    "same70_xplained:same70"
    "same70_xpld:same70"
    "bluepill:stm32f1"
    "stm32f407vg:stm32f4"
    "esp32_devkit:esp32"
    "arduino_zero:samd21"
    "rp_pico:rp2040"
    "rp2040_zero:rp2040"
    "host:linux"
)

function(validate_board_platform board_name)
    # Find the expected platform for this board
    set(expected_platform "")
    foreach(mapping ${MICROCORE_BOARD_PLATFORM_MAP})
        string(REGEX MATCH "^${board_name}:(.+)$" match "${mapping}")
        if(match)
            set(expected_platform "${CMAKE_MATCH_1}")
            break()
        endif()
    endforeach()

    if(expected_platform STREQUAL "")
        message(FATAL_ERROR
            "\n"
            "===========================================================\n"
            "❌ ERROR: Unknown board '${board_name}'\n"
            "===========================================================\n"
            "\n"
            "The board '${board_name}' is not recognized.\n"
            "\n"
            "Supported boards:\n"
            "  - nucleo_f401re    (STM32F401RE Cortex-M4 84MHz)\n"
            "  - nucleo_f722ze    (STM32F722ZE Cortex-M7 216MHz)\n"
            "  - nucleo_g071rb    (STM32G071RB Cortex-M0+ 64MHz)\n"
            "  - nucleo_g0b1re    (STM32G0B1RE Cortex-M0+ 64MHz)\n"
            "  - same70_xplained  (ATSAME70Q21 Cortex-M7 300MHz)\n"
            "  - host             (Native host platform for testing)\n"
            "\n"
            "Usage:\n"
            "  cmake -DMICROCORE_BOARD=nucleo_f401re ..\n"
            "  ./ucore build nucleo_f401re blink\n"
            "\n"
            "===========================================================\n"
        )
    endif()

    # If MICROCORE_PLATFORM is explicitly set, validate it matches
    if(NOT "${MICROCORE_PLATFORM}" STREQUAL "")
        if(NOT "${MICROCORE_PLATFORM}" STREQUAL "${expected_platform}")
            message(FATAL_ERROR
                "\n"
                "===========================================================\n"
                "❌ ERROR: Invalid board/platform combination\n"
                "===========================================================\n"
                "\n"
                "Board:    ${board_name}\n"
                "Expected: ${expected_platform} platform\n"
                "Got:      ${MICROCORE_PLATFORM} platform\n"
                "\n"
                "The platform '${MICROCORE_PLATFORM}' is not compatible\n"
                "with board '${board_name}'.\n"
                "\n"
                "Valid combinations:\n"
                "  nucleo_f401re    → stm32f4\n"
                "  nucleo_f722ze    → stm32f7\n"
                "  nucleo_g071rb    → stm32g0\n"
                "  nucleo_g0b1re    → stm32g0\n"
                "  same70_xplained  → same70\n"
                "  host             → linux\n"
                "\n"
                "Solution:\n"
                "  1. Remove -DMICROCORE_PLATFORM= (auto-detect)\n"
                "  2. Or set: -DMICROCORE_PLATFORM=${expected_platform}\n"
                "\n"
                "===========================================================\n"
            )
        endif()
    else()
        # Auto-detect platform from board
        set(MICROCORE_PLATFORM "${expected_platform}" CACHE STRING "HAL platform (auto-detected)" FORCE)
        message(STATUS "Platform auto-detected: ${expected_platform} (from board: ${board_name})")
    endif()
endfunction()

# Validate board/platform if board is specified
if(NOT "${MICROCORE_BOARD}" STREQUAL "")
    validate_board_platform("${MICROCORE_BOARD}")
endif()

# Build options
option(MICROCORE_BUILD_TESTS "Build unit tests" ON)
option(MICROCORE_MINIMAL_BUILD "Optimize for size (enables -Os, -flto, --gc-sections)" OFF)

if(MICROCORE_BUILD_TESTS)
    enable_testing()

    # Fetch Catch2 v3
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(Catch2)

    # Add Catch2 CMake modules to path
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

# ESP32 boards require ESP-IDF integration
if(MICROCORE_BOARD MATCHES "^esp32")
    include(cmake/platform/esp32_integration.cmake)
endif()

# Auto-detect platform from board if not explicitly set
if(NOT MICROCORE_PLATFORM)
    if(MICROCORE_BOARD STREQUAL "same70_xpld" OR MICROCORE_BOARD STREQUAL "same70_xplained")
        set(MICROCORE_PLATFORM "same70")
    elseif(MICROCORE_BOARD STREQUAL "host")
        set(MICROCORE_PLATFORM "linux")
    elseif(MICROCORE_BOARD MATCHES "^esp32")
        set(MICROCORE_PLATFORM "esp32")
    elseif(MICROCORE_BOARD MATCHES "^stm32f4" OR MICROCORE_BOARD STREQUAL "stm32f407vg")
        set(MICROCORE_PLATFORM "stm32f4")
    elseif(MICROCORE_BOARD MATCHES "^stm32f1" OR MICROCORE_BOARD STREQUAL "bluepill")
        set(MICROCORE_PLATFORM "stm32f1")
    elseif(MICROCORE_BOARD MATCHES "samd21" OR MICROCORE_BOARD STREQUAL "arduino_zero")
        set(MICROCORE_PLATFORM "samd21")
    elseif(MICROCORE_BOARD MATCHES "rp2040" OR MICROCORE_BOARD MATCHES "rp_pico")
        set(MICROCORE_PLATFORM "rp2040")
    elseif(MICROCORE_BOARD STREQUAL "nucleo_g0b1re" OR MICROCORE_BOARD STREQUAL "nucleo_g071rb" OR MICROCORE_BOARD MATCHES "^stm32g0")
        set(MICROCORE_PLATFORM "stm32g0")
    elseif(MICROCORE_BOARD STREQUAL "nucleo_f401re" OR MICROCORE_BOARD MATCHES "^stm32f4" OR MICROCORE_BOARD MATCHES "^nucleo_f4")
        set(MICROCORE_PLATFORM "stm32f4")
    elseif(MICROCORE_BOARD STREQUAL "nucleo_f722ze" OR MICROCORE_BOARD MATCHES "^stm32f7" OR MICROCORE_BOARD MATCHES "^nucleo_f7")
        set(MICROCORE_PLATFORM "stm32f7")
    else()
        message(WARNING "Could not auto-detect MICROCORE_PLATFORM from MICROCORE_BOARD='${MICROCORE_BOARD}'. Using 'linux' as fallback.")
        set(MICROCORE_PLATFORM "linux")
    endif()
    message(STATUS "Auto-detected platform: ${MICROCORE_PLATFORM} (from board: ${MICROCORE_BOARD})")
endif()

# ============================================================================
# Board Abstraction Layer Configuration
# ============================================================================

# Map board to header file and startup code
if(MICROCORE_BOARD STREQUAL "same70_xpld" OR MICROCORE_BOARD STREQUAL "same70_xplained")
    set(BOARD_HEADER_PATH "boards/same70_xplained/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/src/hal/vendors/atmel/same70/startup_same70.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/same70_xplained/ATSAME70Q21.ld")
elseif(MICROCORE_BOARD STREQUAL "arduino_zero")
    set(BOARD_HEADER_PATH "boards/arduino_zero/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/boards/common/startup/atsamd21/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/arduino_zero/ATSAMD21G18.ld")
elseif(MICROCORE_BOARD STREQUAL "waveshare_rp2040_zero" OR MICROCORE_BOARD STREQUAL "rp_pico")
    set(BOARD_HEADER_PATH "boards/waveshare_rp2040_zero/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/boards/common/startup/rp2040/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/waveshare_rp2040_zero/RP2040.ld")
elseif(MICROCORE_BOARD STREQUAL "nucleo_g0b1re")
    set(BOARD_HEADER_PATH "boards/nucleo_g0b1re/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/nucleo_g0b1re/STM32G0B1RET6.ld")
elseif(MICROCORE_BOARD STREQUAL "nucleo_g071rb")
    set(BOARD_HEADER_PATH "boards/nucleo_g071rb/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/nucleo_g071rb/STM32G071RBT6.ld")
elseif(MICROCORE_BOARD STREQUAL "nucleo_f401re")
    set(BOARD_HEADER_PATH "boards/nucleo_f401re/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32f4/stm32f401/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/nucleo_f401re/STM32F401RET6.ld")
elseif(MICROCORE_BOARD STREQUAL "nucleo_f722ze")
    set(BOARD_HEADER_PATH "boards/nucleo_f722ze/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32f7/stm32f722/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/nucleo_f722ze/STM32F722ZET6.ld")
elseif(MICROCORE_BOARD STREQUAL "host")
    set(BOARD_HEADER_PATH "boards/linux_host/board.hpp")
    # No startup code needed for host platform
else()
    message(WARNING "Board '${MICROCORE_BOARD}' not configured for board abstraction layer")
endif()

# Define BOARD_HEADER macro for use in examples
if(DEFINED BOARD_HEADER_PATH)
    # Define BOARD_HEADER as a macro that expands to the header path
    # Use angle brackets for include: #include BOARD_HEADER expands to #include <boards/...>
    add_compile_definitions(BOARD_HEADER=<${BOARD_HEADER_PATH}>)
    message(STATUS "  Board Header: ${BOARD_HEADER_PATH}")

    # Define MICROCORE_BOARD_* for conditional compilation in examples
    string(TOUPPER "${MICROCORE_BOARD}" MICROCORE_BOARD_UPPER)
    add_compile_definitions(MICROCORE_BOARD_${MICROCORE_BOARD_UPPER})
    message(STATUS "  Board Define: MICROCORE_BOARD_${MICROCORE_BOARD_UPPER}")

    if(DEFINED STARTUP_SOURCE)
        message(STATUS "  Startup Code: ${STARTUP_SOURCE}")
    endif()
endif()

# Include platform selection module (new HAL abstraction system)
# This must come before board configuration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform_selection.cmake)
    include(cmake/platform_selection.cmake)
endif()

# Include board selection module (new HAL abstraction system)
# This creates board-specific type aliases and peripheral mappings
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/board_selection.cmake)
    include(cmake/board_selection.cmake)
endif()

# Load legacy board configuration (for backwards compatibility)
# TODO: Migrate legacy board configs to new board_selection system
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/boards/${MICROCORE_BOARD}.cmake)
    message(STATUS "Loading legacy board config: ${MICROCORE_BOARD}")
    include(cmake/boards/${MICROCORE_BOARD}.cmake)
else()
    # Board config not found in legacy location, that's OK
    # Board may use new system (boards/<board>/board.cmake)
endif()

# Validate toolchain for selected board (skip for ESP32, handled by ESP-IDF)
if(NOT MICROCORE_BOARD MATCHES "^esp32")
    alloy_validate_toolchain()
endif()

# Display configuration
message(STATUS "========================================")
message(STATUS "Alloy Framework Configuration")
message(STATUS "========================================")
message(STATUS "  Board       : ${MICROCORE_BOARD}")
message(STATUS "  Platform    : ${MICROCORE_PLATFORM}")
message(STATUS "  MCU         : ${MICROCORE_MCU}")
message(STATUS "  Architecture: ${MICROCORE_ARCH}")
message(STATUS "  Build Type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "========================================")

# Add subdirectories
# add_subdirectory(src/core)

# ==============================================================================
# HAL Library (Platform Abstraction Layer)
# ==============================================================================

# Create HAL library with platform-specific sources
# Sources are collected by platform_selection.cmake

# Common HAL sources (platform-independent, header-only)
# Note: Using explicit lists instead of GLOB for better CMake incremental builds
# These are header-only files used for type definitions and interfaces
set(MICROCORE_HAL_COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/core/concepts.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/core/types.hpp
)

# Core sources (Result, Error, etc. - header-only library)
# Note: Using explicit lists instead of GLOB for better CMake incremental builds
set(MICROCORE_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/circular_buffer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/concepts.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/error.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/error_code.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/esp_error.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/result.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/scoped_device.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/scoped_i2c.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/scoped_spi.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/units.hpp
)

# Create HAL library (INTERFACE for header-only, or STATIC if platform has .cpp files)
if(MICROCORE_PLATFORM_SOURCES OR STARTUP_SOURCE)
    # Platform has .cpp files - create STATIC library
    # Include platform sources + startup code (if board defines one)
    set(HAL_SOURCES ${MICROCORE_PLATFORM_SOURCES} ${MICROCORE_CORE_SOURCES})
    if(DEFINED STARTUP_SOURCE AND EXISTS "${STARTUP_SOURCE}")
        list(APPEND HAL_SOURCES ${STARTUP_SOURCE})
    endif()

    add_library(alloy-hal STATIC ${HAL_SOURCES})

    message(STATUS "Created STATIC alloy-hal library with platform sources")
else()
    # Platform is header-only (templates) - create INTERFACE library
    add_library(alloy-hal INTERFACE)

    message(STATUS "Created INTERFACE alloy-hal library (header-only platform)")
endif()

# Set include directories for HAL library
if(MICROCORE_PLATFORM_SOURCES)
    # STATIC library - use PUBLIC
    target_include_directories(alloy-hal
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${MICROCORE_PLATFORM_DIR}
    )
    target_compile_features(alloy-hal PUBLIC cxx_std_23)
else()
    # INTERFACE library - use INTERFACE
    target_include_directories(alloy-hal
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${MICROCORE_PLATFORM_DIR}
    )
    target_compile_features(alloy-hal INTERFACE cxx_std_23)
endif()

# Export HAL library for use by examples and tests
set(MICROCORE_HAL_LIBRARY alloy-hal CACHE INTERNAL "Alloy HAL library target")

# HAL subdirectories (conditional based on board)
if(MICROCORE_BOARD STREQUAL "host" AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/host/CMakeLists.txt)
    add_subdirectory(src/hal/host)
endif()

# add_subdirectory(src/drivers)

# ==============================================================================
# Examples - Board Abstraction Layer
# ==============================================================================

# Generic examples - work on any board with board.hpp abstraction
# The same code compiles for different boards by including the appropriate board.hpp

# Universal blink example - demonstrates board abstraction power
if(MICROCORE_BOARD STREQUAL "same70_xplained" OR MICROCORE_BOARD STREQUAL "nucleo_g0b1re" OR MICROCORE_BOARD STREQUAL "nucleo_g071rb" OR MICROCORE_BOARD STREQUAL "nucleo_f401re" OR MICROCORE_BOARD STREQUAL "nucleo_f722ze")
    add_subdirectory(examples/blink)
endif()

# Universal RTOS example - demonstrates RTOS with multi-task scheduling
if(MICROCORE_BOARD STREQUAL "same70_xplained" OR MICROCORE_BOARD STREQUAL "nucleo_g0b1re" OR MICROCORE_BOARD STREQUAL "nucleo_g071rb" OR MICROCORE_BOARD STREQUAL "nucleo_f401re" OR MICROCORE_BOARD STREQUAL "nucleo_f722ze")
    add_subdirectory(examples/rtos/simple_tasks)
endif()

# Board-specific examples
if(MICROCORE_BOARD STREQUAL "same70_xplained")
    add_subdirectory(examples/uart_logger)
endif()

# TODO: Add more boards as they implement board.hpp abstraction:
# if(MICROCORE_BOARD STREQUAL "stm32f103_bluepill")
#     add_subdirectory(examples/blink)
# endif()
#
# if(MICROCORE_BOARD STREQUAL "arduino_zero")
#     add_subdirectory(examples/blink)
# endif()

# Tests
if(MICROCORE_BUILD_TESTS AND MICROCORE_BOARD STREQUAL "host")
    add_subdirectory(tests)
endif()

# Benchmarks (embedded targets only)
option(MICROCORE_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
if(MICROCORE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Build System Validation Targets
# ==============================================================================

# Add custom target for validating source files
add_custom_target(validate-build-system
    COMMAND ${CMAKE_COMMAND} -DCMAKE_CURRENT_SOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/validate_sources.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating build system source files..."
    VERBATIM
)

# Add custom target for testing board/platform validation
# This intentionally tries invalid combinations to verify error detection
add_custom_target(test-platform-validation
    COMMAND ${CMAKE_COMMAND} -E echo "Testing Board/Platform Validation..."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Test 1: Valid combination (nucleo_g0b1re + stm32g0)"
    COMMAND ${CMAKE_COMMAND} -DMICROCORE_BOARD=nucleo_g0b1re -DMICROCORE_PLATFORM=stm32g0 -B build-validation-test1 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake > /dev/null 2>&1 && echo "✓ PASS" || echo "✗ FAIL"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Test 2: Invalid combination (nucleo_g0b1re + stm32f4) - should FAIL"
    COMMAND ${CMAKE_COMMAND} -DMICROCORE_BOARD=nucleo_g0b1re -DMICROCORE_PLATFORM=stm32f4 -B build-validation-test2 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake > /dev/null 2>&1 && echo "✗ FAIL (should have detected mismatch)" || echo "✓ PASS (correctly rejected)"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Validation test complete!"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Testing platform validation logic..."
    VERBATIM
)

# ==============================================================================
# Documentation (Doxygen)
# ==============================================================================

find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    # Custom target to generate documentation
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
    message(STATUS "  Run 'make docs' to generate documentation")
    message(STATUS "  Output: docs/api/html/index.html")

    if(DOXYGEN_DOT_FOUND)
        message(STATUS "  GraphViz/Dot found: Class diagrams enabled")
    else()
        message(STATUS "  GraphViz/Dot not found: Class diagrams disabled")
    endif()
else()
    message(STATUS "Doxygen not found (optional)")
    message(STATUS "  Install: brew install doxygen (macOS) or apt-get install doxygen (Linux)")
    message(STATUS "  Documentation generation will be disabled")
endif()
