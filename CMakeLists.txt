cmake_minimum_required(VERSION 3.25)

# Export compile commands for IDE/LSP support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project definition
project(microcore
    VERSION 0.1.0
    DESCRIPTION "The modern C++20 framework for bare-metal embedded systems"
    LANGUAGES CXX C ASM
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Require C++23 standard (Phase 5: C++23 Enhancements)
# Enables: consteval, if consteval, deducing this, and other C++23 features
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MicroCore configuration variables
set(MICROCORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "MicroCore framework root directory")

# Legacy compatibility defaults (ALLOY_* -> MICROCORE_*)
set(_MICROCORE_DEFAULT_BOARD "host")
if(DEFINED ALLOY_BOARD AND NOT DEFINED MICROCORE_BOARD)
    set(_MICROCORE_DEFAULT_BOARD "${ALLOY_BOARD}")
    set(_MICROCORE_USED_LEGACY_ALLOY_BOARD ON)
endif()

set(_MICROCORE_DEFAULT_PLATFORM "")
if(DEFINED ALLOY_PLATFORM AND NOT DEFINED MICROCORE_PLATFORM)
    set(_MICROCORE_DEFAULT_PLATFORM "${ALLOY_PLATFORM}")
    set(_MICROCORE_USED_LEGACY_ALLOY_PLATFORM ON)
endif()

set(_MICROCORE_DEFAULT_BUILD_TESTS ON)
if(DEFINED ALLOY_BUILD_TESTS AND NOT DEFINED MICROCORE_BUILD_TESTS)
    set(_MICROCORE_DEFAULT_BUILD_TESTS "${ALLOY_BUILD_TESTS}")
    set(_MICROCORE_USED_LEGACY_ALLOY_BUILD_TESTS ON)
endif()

set(_MICROCORE_DEFAULT_MINIMAL_BUILD OFF)
if(DEFINED ALLOY_MINIMAL_BUILD AND NOT DEFINED MICROCORE_MINIMAL_BUILD)
    set(_MICROCORE_DEFAULT_MINIMAL_BUILD "${ALLOY_MINIMAL_BUILD}")
    set(_MICROCORE_USED_LEGACY_ALLOY_MINIMAL_BUILD ON)
endif()

set(MICROCORE_BOARD "${_MICROCORE_DEFAULT_BOARD}" CACHE STRING "Target board (host, bluepill, esp32_devkit, stm32f407vg, arduino_zero, rp_pico)")
set(MICROCORE_MCU "" CACHE STRING "Target MCU (set by board configuration)")
set(MICROCORE_ARCH "" CACHE STRING "Target architecture (set by board configuration)")

# Platform abstraction configuration (new system)
# MICROCORE_PLATFORM is the HAL platform (same70, linux, esp32, stm32f4, etc.)
# It will be auto-detected from MICROCORE_BOARD if not explicitly set
set(MICROCORE_PLATFORM "${_MICROCORE_DEFAULT_PLATFORM}" CACHE STRING "HAL platform (same70, linux, esp32, stm32f4, nrf52)")

# Delivery profile controls dependency expectations and packaging behavior.
# - contributor: full development profile (default)
# - consumer: packaged SDK consumption profile (no runtime codegen dependency)
set(MICROCORE_DELIVERY_PROFILE "contributor" CACHE STRING "Delivery profile (contributor|consumer)")
set_property(CACHE MICROCORE_DELIVERY_PROFILE PROPERTY STRINGS "contributor" "consumer")
if(NOT MICROCORE_DELIVERY_PROFILE STREQUAL "contributor" AND
   NOT MICROCORE_DELIVERY_PROFILE STREQUAL "consumer")
    message(FATAL_ERROR
        "Invalid MICROCORE_DELIVERY_PROFILE='${MICROCORE_DELIVERY_PROFILE}'. "
        "Supported: contributor, consumer"
    )
endif()
if(MICROCORE_DELIVERY_PROFILE STREQUAL "consumer")
    set(MICROCORE_CONSUMER_MODE ON)
else()
    set(MICROCORE_CONSUMER_MODE OFF)
endif()

option(
    MICROCORE_ENFORCE_CODEGEN_CONTRACT
    "Fail configure if generated board artifacts are incompatible with framework version"
    ON
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/codegen_contract.cmake)

# Generated board metadata (canonical source for supported embedded boards).
set(MICROCORE_GENERATED_BOARD_METADATA_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generated/board_metadata.cmake")
if(EXISTS "${MICROCORE_GENERATED_BOARD_METADATA_FILE}")
    include("${MICROCORE_GENERATED_BOARD_METADATA_FILE}")
    if(MICROCORE_ENFORCE_CODEGEN_CONTRACT)
        microcore_validate_codegen_artifact_contract()
    else()
        message(STATUS "MICROCORE_ENFORCE_CODEGEN_CONTRACT=OFF: skipping artifact compatibility checks")
    endif()
else()
    message(WARNING
        "Generated board metadata not found: ${MICROCORE_GENERATED_BOARD_METADATA_FILE}\n"
        "Run: python3 tools/codegen/scripts/generate_board_artifacts.py"
    )
endif()

# Emit compatibility diagnostics when legacy variables are used.
if(DEFINED ALLOY_BOARD)
    if(DEFINED MICROCORE_BOARD AND NOT "${MICROCORE_BOARD}" STREQUAL "${ALLOY_BOARD}" AND NOT _MICROCORE_USED_LEGACY_ALLOY_BOARD)
        message(FATAL_ERROR
            "Both ALLOY_BOARD ('${ALLOY_BOARD}') and MICROCORE_BOARD ('${MICROCORE_BOARD}') are set with different values.\n"
            "Use only MICROCORE_BOARD."
        )
    endif()
    message(DEPRECATION "CMake variable ALLOY_BOARD is deprecated; use MICROCORE_BOARD.")
endif()

if(DEFINED ALLOY_PLATFORM)
    if(DEFINED MICROCORE_PLATFORM AND NOT "${MICROCORE_PLATFORM}" STREQUAL "" AND NOT "${MICROCORE_PLATFORM}" STREQUAL "${ALLOY_PLATFORM}" AND NOT _MICROCORE_USED_LEGACY_ALLOY_PLATFORM)
        message(FATAL_ERROR
            "Both ALLOY_PLATFORM ('${ALLOY_PLATFORM}') and MICROCORE_PLATFORM ('${MICROCORE_PLATFORM}') are set with different values.\n"
            "Use only MICROCORE_PLATFORM."
        )
    endif()
    message(DEPRECATION "CMake variable ALLOY_PLATFORM is deprecated; use MICROCORE_PLATFORM.")
endif()

if(DEFINED ALLOY_BUILD_TESTS)
    if(DEFINED MICROCORE_BUILD_TESTS AND NOT "${MICROCORE_BUILD_TESTS}" STREQUAL "${ALLOY_BUILD_TESTS}" AND NOT _MICROCORE_USED_LEGACY_ALLOY_BUILD_TESTS)
        message(FATAL_ERROR
            "Both ALLOY_BUILD_TESTS ('${ALLOY_BUILD_TESTS}') and MICROCORE_BUILD_TESTS ('${MICROCORE_BUILD_TESTS}') differ.\n"
            "Use only MICROCORE_BUILD_TESTS."
        )
    endif()
    message(DEPRECATION "CMake variable ALLOY_BUILD_TESTS is deprecated; use MICROCORE_BUILD_TESTS.")
endif()

if(DEFINED ALLOY_MINIMAL_BUILD)
    if(DEFINED MICROCORE_MINIMAL_BUILD AND NOT "${MICROCORE_MINIMAL_BUILD}" STREQUAL "${ALLOY_MINIMAL_BUILD}" AND NOT _MICROCORE_USED_LEGACY_ALLOY_MINIMAL_BUILD)
        message(FATAL_ERROR
            "Both ALLOY_MINIMAL_BUILD ('${ALLOY_MINIMAL_BUILD}') and MICROCORE_MINIMAL_BUILD ('${MICROCORE_MINIMAL_BUILD}') differ.\n"
            "Use only MICROCORE_MINIMAL_BUILD."
        )
    endif()
    message(DEPRECATION "CMake variable ALLOY_MINIMAL_BUILD is deprecated; use MICROCORE_MINIMAL_BUILD.")
endif()

# Board options
set(_MICROCORE_LEGACY_SUPPORTED_BOARDS
    "host"
    "bluepill"           # STM32F103C8 Blue Pill
    "esp32_devkit"       # ESP32 DevKit
    "stm32f407vg"        # STM32F407VG Discovery
    "arduino_zero"       # ATSAMD21G18 Arduino Zero
    "rp_pico"            # RP2040 Raspberry Pi Pico
    "rp2040_zero"        # Waveshare RP2040-Zero
    "same70_xplained"    # Atmel SAME70 Xplained
    "same70_xpld"        # Atmel SAME70 Xplained
    "nucleo_g0b1re"      # STM32 Nucleo-G0B1RE
    "nucleo_g071rb"      # STM32 Nucleo-G071RB
    "nucleo_f401re"      # STM32 Nucleo-F401RE
    "nucleo_f722ze"      # STM32 Nucleo-F722ZE
)

set(_MICROCORE_SUPPORTED_BOARD_STRINGS "")
if(DEFINED MICROCORE_GENERATED_SUPPORTED_BOARDS)
    list(APPEND _MICROCORE_SUPPORTED_BOARD_STRINGS ${MICROCORE_GENERATED_SUPPORTED_BOARDS})
endif()
list(APPEND _MICROCORE_SUPPORTED_BOARD_STRINGS ${_MICROCORE_LEGACY_SUPPORTED_BOARDS})
list(REMOVE_DUPLICATES _MICROCORE_SUPPORTED_BOARD_STRINGS)
set_property(CACHE MICROCORE_BOARD PROPERTY STRINGS ${_MICROCORE_SUPPORTED_BOARD_STRINGS})

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

set(MICROCORE_BOARD_PLATFORM_MAP)
if(DEFINED MICROCORE_GENERATED_BOARD_PLATFORM_MAP)
    list(APPEND MICROCORE_BOARD_PLATFORM_MAP ${MICROCORE_GENERATED_BOARD_PLATFORM_MAP})
endif()
list(APPEND MICROCORE_BOARD_PLATFORM_MAP
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
list(REMOVE_DUPLICATES MICROCORE_BOARD_PLATFORM_MAP)

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

    set(_known_boards "")
    foreach(mapping ${MICROCORE_BOARD_PLATFORM_MAP})
        string(REPLACE ":" ";" mapping_parts "${mapping}")
        list(LENGTH mapping_parts mapping_parts_len)
        if(mapping_parts_len GREATER 1)
            list(GET mapping_parts 0 mapped_board)
            list(APPEND _known_boards "${mapped_board}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _known_boards)
    list(SORT _known_boards)
    string(JOIN ", " _known_boards_csv ${_known_boards})

    if(expected_platform STREQUAL "")
        message(FATAL_ERROR
            "\n"
            "===========================================================\n"
            "❌ ERROR: Unknown board '${board_name}'\n"
            "===========================================================\n"
            "\n"
            "The board '${board_name}' is not recognized.\n"
            "\n"
            "Supported boards: ${_known_boards_csv}\n"
            "\n"
            "Usage:\n"
            "  cmake -DMICROCORE_BOARD=<board> ..\n"
            "  ./ucore list boards\n"
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
                "Known boards: ${_known_boards_csv}\n"
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
option(MICROCORE_BUILD_TESTS "Build unit tests" ${_MICROCORE_DEFAULT_BUILD_TESTS})
option(MICROCORE_MINIMAL_BUILD "Optimize for size (enables -Os, -flto, --gc-sections)" ${_MICROCORE_DEFAULT_MINIMAL_BUILD})

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
    set(_detected_platform "")
    foreach(mapping ${MICROCORE_BOARD_PLATFORM_MAP})
        string(REGEX MATCH "^${MICROCORE_BOARD}:(.+)$" match "${mapping}")
        if(match)
            set(_detected_platform "${CMAKE_MATCH_1}")
            break()
        endif()
    endforeach()

    if(_detected_platform STREQUAL "")
        message(WARNING "Could not auto-detect MICROCORE_PLATFORM from MICROCORE_BOARD='${MICROCORE_BOARD}'. Using 'linux' as fallback.")
        set(MICROCORE_PLATFORM "linux")
    else()
        set(MICROCORE_PLATFORM "${_detected_platform}")
    endif()
    message(STATUS "Auto-detected platform: ${MICROCORE_PLATFORM} (from board: ${MICROCORE_BOARD})")
endif()

# Legacy CMake variable aliases (for migration period).
# Keep them synchronized with canonical MICROCORE_* values.
set(ALLOY_ROOT "${MICROCORE_ROOT}")
set(ALLOY_BOARD "${MICROCORE_BOARD}")
set(ALLOY_PLATFORM "${MICROCORE_PLATFORM}")
set(ALLOY_MCU "${MICROCORE_MCU}")
set(ALLOY_ARCH "${MICROCORE_ARCH}")
set(ALLOY_BUILD_TESTS "${MICROCORE_BUILD_TESTS}")
set(ALLOY_MINIMAL_BUILD "${MICROCORE_MINIMAL_BUILD}")

# ============================================================================
# Board Abstraction Layer Configuration
# ============================================================================

# Map board to header/startup/linker using generated metadata first.
set(_MICROCORE_METADATA_BOARD_ID "${MICROCORE_BOARD}")
if(MICROCORE_BOARD STREQUAL "same70_xpld")
    set(_MICROCORE_METADATA_BOARD_ID "same70_xplained")
endif()

if(DEFINED MICROCORE_GENERATED_BOARD_HEADER_${_MICROCORE_METADATA_BOARD_ID})
    set(BOARD_HEADER_PATH "${MICROCORE_GENERATED_BOARD_HEADER_${_MICROCORE_METADATA_BOARD_ID}}")
endif()

if(DEFINED MICROCORE_GENERATED_BOARD_STARTUP_SOURCE_${_MICROCORE_METADATA_BOARD_ID})
    set(_MICROCORE_STARTUP_REL "${MICROCORE_GENERATED_BOARD_STARTUP_SOURCE_${_MICROCORE_METADATA_BOARD_ID}}")
    if(NOT "${_MICROCORE_STARTUP_REL}" STREQUAL "")
        set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/${_MICROCORE_STARTUP_REL}")
    endif()
endif()

if(DEFINED MICROCORE_GENERATED_BOARD_LINKER_SCRIPT_${_MICROCORE_METADATA_BOARD_ID})
    set(_MICROCORE_LINKER_REL "${MICROCORE_GENERATED_BOARD_LINKER_SCRIPT_${_MICROCORE_METADATA_BOARD_ID}}")
    if(NOT "${_MICROCORE_LINKER_REL}" STREQUAL "")
        set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/${_MICROCORE_LINKER_REL}")
    endif()
endif()

if(DEFINED MICROCORE_GENERATED_BOARD_MCU_${_MICROCORE_METADATA_BOARD_ID})
    set(MICROCORE_MCU "${MICROCORE_GENERATED_BOARD_MCU_${_MICROCORE_METADATA_BOARD_ID}}" CACHE STRING "Target MCU (set by board configuration)" FORCE)
endif()

if(DEFINED MICROCORE_GENERATED_BOARD_ARCH_${_MICROCORE_METADATA_BOARD_ID})
    set(MICROCORE_ARCH "${MICROCORE_GENERATED_BOARD_ARCH_${_MICROCORE_METADATA_BOARD_ID}}" CACHE STRING "Target architecture (set by board configuration)" FORCE)
endif()

# Legacy fallback for boards not yet in generated metadata.
if(NOT DEFINED BOARD_HEADER_PATH)
    if(MICROCORE_BOARD STREQUAL "arduino_zero")
        set(BOARD_HEADER_PATH "boards/arduino_zero/board.hpp")
        set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/boards/common/startup/atsamd21/startup.cpp")
        set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/arduino_zero/ATSAMD21G18.ld")
    elseif(MICROCORE_BOARD STREQUAL "waveshare_rp2040_zero" OR MICROCORE_BOARD STREQUAL "rp_pico")
        set(BOARD_HEADER_PATH "boards/waveshare_rp2040_zero/board.hpp")
        set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/boards/common/startup/rp2040/startup.cpp")
        set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/waveshare_rp2040_zero/RP2040.ld")
    elseif(MICROCORE_BOARD STREQUAL "host")
        set(BOARD_HEADER_PATH "boards/linux_host/board.hpp")
        # No startup code needed for host platform
    else()
        message(WARNING "Board '${MICROCORE_BOARD}' not configured for board abstraction layer")
    endif()
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
    add_compile_definitions(ALLOY_BOARD_${MICROCORE_BOARD_UPPER})  # Legacy compatibility
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

# Populate board memory sizes for memory-report tooling when board metadata
# has not yet provided explicit values.
if((NOT DEFINED MICROCORE_RAM_SIZE OR MICROCORE_RAM_SIZE EQUAL 0) OR
   (NOT DEFINED MICROCORE_FLASH_SIZE OR MICROCORE_FLASH_SIZE EQUAL 0))
    if(MICROCORE_BOARD STREQUAL "nucleo_f401re")
        set(MICROCORE_RAM_SIZE 98304 CACHE STRING "Target RAM size in bytes" FORCE)
        set(MICROCORE_FLASH_SIZE 524288 CACHE STRING "Target Flash size in bytes" FORCE)
    elseif(MICROCORE_BOARD STREQUAL "nucleo_f722ze")
        set(MICROCORE_RAM_SIZE 262144 CACHE STRING "Target RAM size in bytes" FORCE)
        set(MICROCORE_FLASH_SIZE 524288 CACHE STRING "Target Flash size in bytes" FORCE)
    elseif(MICROCORE_BOARD STREQUAL "nucleo_g071rb")
        set(MICROCORE_RAM_SIZE 36864 CACHE STRING "Target RAM size in bytes" FORCE)
        set(MICROCORE_FLASH_SIZE 131072 CACHE STRING "Target Flash size in bytes" FORCE)
    elseif(MICROCORE_BOARD STREQUAL "nucleo_g0b1re")
        set(MICROCORE_RAM_SIZE 147456 CACHE STRING "Target RAM size in bytes" FORCE)
        set(MICROCORE_FLASH_SIZE 524288 CACHE STRING "Target Flash size in bytes" FORCE)
    elseif(MICROCORE_BOARD STREQUAL "same70_xplained" OR MICROCORE_BOARD STREQUAL "same70_xpld")
        set(MICROCORE_RAM_SIZE 393216 CACHE STRING "Target RAM size in bytes" FORCE)
        set(MICROCORE_FLASH_SIZE 2097152 CACHE STRING "Target Flash size in bytes" FORCE)
    elseif(MICROCORE_BOARD STREQUAL "host")
        # Host values are placeholders for report formatting only.
        set(MICROCORE_RAM_SIZE 67108864 CACHE STRING "Target RAM size in bytes" FORCE)
        set(MICROCORE_FLASH_SIZE 268435456 CACHE STRING "Target Flash size in bytes" FORCE)
    endif()
endif()

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

    add_library(microcore_hal STATIC ${HAL_SOURCES})

    message(STATUS "Created STATIC microcore_hal library with platform sources")
else()
    # Platform is header-only (templates) - create INTERFACE library
    add_library(microcore_hal INTERFACE)

    message(STATUS "Created INTERFACE microcore_hal library (header-only platform)")
endif()

set_target_properties(microcore_hal PROPERTIES EXPORT_NAME hal)

# Set include directories for HAL library
if(MICROCORE_PLATFORM_SOURCES)
    # STATIC library - use PUBLIC
    target_include_directories(microcore_hal
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<BUILD_INTERFACE:${MICROCORE_PLATFORM_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/microcore/src>
    )
    target_compile_features(microcore_hal PUBLIC cxx_std_23)
else()
    # INTERFACE library - use INTERFACE
    target_include_directories(microcore_hal
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
            $<BUILD_INTERFACE:${MICROCORE_PLATFORM_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/microcore/src>
    )
    target_compile_features(microcore_hal INTERFACE cxx_std_23)
endif()

# Public and legacy aliases for migration compatibility.
if(NOT TARGET microcore::hal)
    add_library(microcore::hal ALIAS microcore_hal)
endif()
if(NOT TARGET alloy::hal)
    add_library(alloy::hal ALIAS microcore_hal)
endif()
if(NOT TARGET alloy-hal)
    add_library(alloy-hal ALIAS microcore_hal)
endif()

# Export HAL target variables for use by examples and tests.
set(MICROCORE_HAL_LIBRARY microcore_hal CACHE INTERNAL "MicroCore HAL library target")
set(ALLOY_HAL_LIBRARY alloy-hal CACHE INTERNAL "Legacy HAL library target alias")

# Public board target (for selected board) - available as microcore::board::<board>.
if(DEFINED MICROCORE_BOARD AND NOT "${MICROCORE_BOARD}" STREQUAL "")
    set(_MICROCORE_BOARD_TARGET "microcore_board_${MICROCORE_BOARD}")
    set(_MICROCORE_BOARD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/boards/${MICROCORE_BOARD}")
    set(_MICROCORE_BOARD_CPP "${_MICROCORE_BOARD_DIR}/board.cpp")
    set(_MICROCORE_SYSCALLS_CPP "${_MICROCORE_BOARD_DIR}/syscalls.cpp")

    if(EXISTS "${_MICROCORE_BOARD_CPP}")
        add_library(${_MICROCORE_BOARD_TARGET} STATIC "${_MICROCORE_BOARD_CPP}")
        if(CMAKE_CROSSCOMPILING AND EXISTS "${_MICROCORE_SYSCALLS_CPP}")
            target_sources(${_MICROCORE_BOARD_TARGET} PRIVATE "${_MICROCORE_SYSCALLS_CPP}")
        endif()
        target_include_directories(${_MICROCORE_BOARD_TARGET}
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/boards>
                $<BUILD_INTERFACE:${_MICROCORE_BOARD_DIR}>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/microcore/src>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/microcore>
        )
        target_compile_features(${_MICROCORE_BOARD_TARGET} PUBLIC cxx_std_23)
        target_link_libraries(${_MICROCORE_BOARD_TARGET} PUBLIC microcore::hal)
        set(_MICROCORE_BOARD_VISIBILITY PUBLIC)
    else()
        add_library(${_MICROCORE_BOARD_TARGET} INTERFACE)
        target_include_directories(${_MICROCORE_BOARD_TARGET}
            INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/boards>
                $<BUILD_INTERFACE:${_MICROCORE_BOARD_DIR}>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/microcore/src>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/microcore>
        )
        target_compile_features(${_MICROCORE_BOARD_TARGET} INTERFACE cxx_std_23)
        target_link_libraries(${_MICROCORE_BOARD_TARGET} INTERFACE microcore::hal)
        set(_MICROCORE_BOARD_VISIBILITY INTERFACE)
    endif()

    set_target_properties(${_MICROCORE_BOARD_TARGET} PROPERTIES EXPORT_NAME "board_${MICROCORE_BOARD}")

    if(DEFINED MICROCORE_BOARD_UPPER)
        target_compile_definitions(${_MICROCORE_BOARD_TARGET} ${_MICROCORE_BOARD_VISIBILITY}
            MICROCORE_BOARD_${MICROCORE_BOARD_UPPER}
            ALLOY_BOARD_${MICROCORE_BOARD_UPPER}
        )
    endif()

    if(NOT TARGET microcore::board::${MICROCORE_BOARD})
        add_library(microcore::board::${MICROCORE_BOARD} ALIAS ${_MICROCORE_BOARD_TARGET})
    endif()
    if(NOT TARGET alloy::board::${MICROCORE_BOARD})
        add_library(alloy::board::${MICROCORE_BOARD} ALIAS ${_MICROCORE_BOARD_TARGET})
    endif()
endif()

# Public RTOS integration target (header/source users still control concrete source linkage).
if(NOT TARGET microcore_rtos)
    add_library(microcore_rtos INTERFACE)
    target_include_directories(microcore_rtos INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/microcore/src>
    )
    target_compile_features(microcore_rtos INTERFACE cxx_std_23)
    target_compile_definitions(microcore_rtos INTERFACE
        UCORE_RTOS_ENABLED=1
        ALLOY_RTOS_ENABLED=1
    )
    target_link_libraries(microcore_rtos INTERFACE microcore::hal)
endif()

set_target_properties(microcore_rtos PROPERTIES EXPORT_NAME rtos)

if(NOT TARGET microcore::rtos)
    add_library(microcore::rtos ALIAS microcore_rtos)
endif()
if(NOT TARGET alloy::rtos)
    add_library(alloy::rtos ALIAS microcore_rtos)
endif()

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

function(microcore_board_supports_example example_id out_result)
    set(_supports OFF)
    set(_metadata_board_id "${MICROCORE_BOARD}")
    if(_metadata_board_id STREQUAL "same70_xpld")
        set(_metadata_board_id "same70_xplained")
    endif()

    if(DEFINED MICROCORE_GENERATED_BOARD_SUPPORTED_EXAMPLES_${_metadata_board_id})
        set(_supported_examples "${MICROCORE_GENERATED_BOARD_SUPPORTED_EXAMPLES_${_metadata_board_id}}")
        if(NOT "${_supported_examples}" STREQUAL "")
            list(FIND _supported_examples "${example_id}" _example_index)
            if(NOT _example_index EQUAL -1)
                set(_supports ON)
            endif()
        endif()
    else()
        # Legacy fallback when generated metadata is unavailable.
        if(example_id STREQUAL "blink")
            if(MICROCORE_BOARD STREQUAL "same70_xplained" OR MICROCORE_BOARD STREQUAL "nucleo_g0b1re" OR MICROCORE_BOARD STREQUAL "nucleo_g071rb" OR MICROCORE_BOARD STREQUAL "nucleo_f401re" OR MICROCORE_BOARD STREQUAL "nucleo_f722ze")
                set(_supports ON)
            endif()
        elseif(example_id STREQUAL "rtos/simple_tasks")
            if(MICROCORE_BOARD STREQUAL "nucleo_f401re" OR MICROCORE_BOARD STREQUAL "nucleo_f722ze")
                set(_supports ON)
            endif()
        elseif(example_id STREQUAL "api_tiers/simple_gpio_blink")
            if(MICROCORE_BOARD STREQUAL "same70_xplained" OR MICROCORE_BOARD STREQUAL "nucleo_g0b1re" OR MICROCORE_BOARD STREQUAL "nucleo_g071rb" OR MICROCORE_BOARD STREQUAL "nucleo_f401re" OR MICROCORE_BOARD STREQUAL "nucleo_f722ze")
                set(_supports ON)
            endif()
        elseif(example_id STREQUAL "uart_logger")
            if(MICROCORE_BOARD STREQUAL "same70_xplained")
                set(_supports ON)
            endif()
        endif()
    endif()

    set(${out_result} "${_supports}" PARENT_SCOPE)
endfunction()

# Universal blink example - demonstrates board abstraction power
microcore_board_supports_example("blink" _MICROCORE_SUPPORTS_BLINK)
if(_MICROCORE_SUPPORTS_BLINK)
    add_subdirectory(examples/blink)
endif()

# Universal RTOS example - demonstrates RTOS with multi-task scheduling
microcore_board_supports_example("rtos/simple_tasks" _MICROCORE_SUPPORTS_RTOS_SIMPLE_TASKS)
if(_MICROCORE_SUPPORTS_RTOS_SIMPLE_TASKS)
    add_subdirectory(examples/rtos/simple_tasks)
endif()

# API Tier examples - demonstrates Simple/Fluent/Expert API system
microcore_board_supports_example("api_tiers/simple_gpio_blink" _MICROCORE_SUPPORTS_API_TIERS)
if(_MICROCORE_SUPPORTS_API_TIERS)
    add_subdirectory(examples/api_tiers)
endif()

# Board-specific examples
microcore_board_supports_example("uart_logger" _MICROCORE_SUPPORTS_UART_LOGGER)
if(_MICROCORE_SUPPORTS_UART_LOGGER)
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

# ==============================================================================
# Memory Analysis and Minimal-Build Integration
# ==============================================================================

function(microcore_enable_memory_features target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    if(COMMAND alloy_add_memory_analysis)
        alloy_add_memory_analysis(${target_name})
    endif()

    if(MICROCORE_MINIMAL_BUILD AND COMMAND alloy_apply_minimal_flags)
        alloy_apply_minimal_flags(${target_name})
    endif()
endfunction()

set(_MICROCORE_MEMORY_TARGETS
    blink
    rtos_simple_tasks
    simple_gpio_blink
    simple_gpio_button
    fluent_gpio_blink
    expert_gpio_blink
    simple_uart_echo
    fluent_uart_config
    expert_uart_lowpower
    uart_logger
)

foreach(_target ${_MICROCORE_MEMORY_TARGETS})
    microcore_enable_memory_features(${_target})
endforeach()

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
# SDK Packaging (install + find_package)
# ==============================================================================

option(MICROCORE_INSTALL_SDK "Install MicroCore SDK package for external consumers" ON)
if(MICROCORE_CONSUMER_MODE)
    set(_MICROCORE_DEFAULT_INSTALL_CODEGEN_TOOLING OFF)
else()
    set(_MICROCORE_DEFAULT_INSTALL_CODEGEN_TOOLING ON)
endif()
option(
    MICROCORE_INSTALL_CODEGEN_TOOLING
    "Install codegen boundary artifacts (contracts/scripts/docs) for contributor SDK bundles"
    ${_MICROCORE_DEFAULT_INSTALL_CODEGEN_TOOLING}
)

if(MICROCORE_INSTALL_SDK)
    set(_MICROCORE_INSTALL_EXPORT_TARGETS
        microcore_hal
        microcore_rtos
    )

    if(DEFINED _MICROCORE_BOARD_TARGET AND TARGET ${_MICROCORE_BOARD_TARGET})
        list(APPEND _MICROCORE_INSTALL_EXPORT_TARGETS ${_MICROCORE_BOARD_TARGET})
    endif()

    install(
        TARGETS ${_MICROCORE_INSTALL_EXPORT_TARGETS}
        EXPORT microcoreTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT runtime
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT runtime
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/microcore/src
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/microcore/src
        COMPONENT runtime
        FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.h"
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/boards/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/microcore/boards
        COMPONENT runtime
        FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.h"
            PATTERN "*.ld"
            PATTERN "board.yaml"
    )

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/boards/board-schema.json")
        install(
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/boards/board-schema.json
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/microcore/boards
            COMPONENT runtime
        )
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/boards/generated")
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/boards/generated/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/microcore/boards/generated
            COMPONENT runtime
            FILES_MATCHING
                PATTERN "*.hpp"
                PATTERN "*.json"
        )
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generated/board_metadata.cmake")
        install(
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generated/board_metadata.cmake
            DESTINATION ${CMAKE_INSTALL_DATADIR}/microcore/cmake/generated
            COMPONENT runtime
        )
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generated/boards")
        install(
            DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake/generated/boards/
            DESTINATION ${CMAKE_INSTALL_DATADIR}/microcore/cmake/generated/boards
            COMPONENT runtime
            FILES_MATCHING
                PATTERN "*.cmake"
        )
    endif()

    set(MICROCORE_PACKAGE_HAS_CODEGEN_TOOLING OFF)
    if(MICROCORE_INSTALL_CODEGEN_TOOLING)
        set(MICROCORE_PACKAGE_HAS_CODEGEN_TOOLING ON)

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/contracts")
            install(
                DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/contracts/
                DESTINATION ${CMAKE_INSTALL_DATADIR}/microcore/codegen/contracts
                COMPONENT codegen
                FILES_MATCHING
                    PATTERN "*.json"
            )
        endif()

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/scripts/generate_board_artifacts.py")
            install(
                FILES
                    ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/scripts/generate_board_artifacts.py
                    ${CMAKE_CURRENT_SOURCE_DIR}/tools/codegen/scripts/validate_codegen_contract.py
                DESTINATION ${CMAKE_INSTALL_DATADIR}/microcore/codegen/scripts
                COMPONENT codegen
            )
        endif()

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/docs/CODEGEN_INTERFACE_CONTRACT.md")
            install(
                FILES
                    ${CMAKE_CURRENT_SOURCE_DIR}/docs/CODEGEN_INTERFACE_CONTRACT.md
                    ${CMAKE_CURRENT_SOURCE_DIR}/docs/CODEGEN_EXTRACTION_PLAN.md
                DESTINATION ${CMAKE_INSTALL_DATADIR}/microcore/docs
                COMPONENT codegen
            )
        endif()
    endif()

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/microcoreConfig.cmake.template
        ${CMAKE_CURRENT_BINARY_DIR}/microcoreConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microcore
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/microcoreConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/microcoreConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/microcoreConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microcore
        COMPONENT runtime
    )

    install(
        EXPORT microcoreTargets
        FILE microcoreTargets.cmake
        NAMESPACE microcore::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/microcore
        COMPONENT runtime
    )
endif()

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
