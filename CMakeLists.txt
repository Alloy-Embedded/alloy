cmake_minimum_required(VERSION 3.25)

# Export compile commands for IDE/LSP support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project definition
project(alloy
    VERSION 0.1.0
    DESCRIPTION "The modern C++20 framework for bare-metal embedded systems"
    LANGUAGES CXX C ASM
)

# Require C++23 standard (Phase 5: C++23 Enhancements)
# Enables: consteval, if consteval, deducing this, and other C++23 features
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Alloy configuration variables
set(ALLOY_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Alloy framework root directory")
set(ALLOY_BOARD "host" CACHE STRING "Target board (host, bluepill, esp32_devkit, stm32f407vg, arduino_zero, rp_pico)")
set(ALLOY_MCU "" CACHE STRING "Target MCU (set by board configuration)")
set(ALLOY_ARCH "" CACHE STRING "Target architecture (set by board configuration)")

# Platform abstraction configuration (new system)
# ALLOY_PLATFORM is the HAL platform (same70, linux, esp32, stm32f4, etc.)
# It will be auto-detected from ALLOY_BOARD if not explicitly set
set(ALLOY_PLATFORM "" CACHE STRING "HAL platform (same70, linux, esp32, stm32f4, nrf52)")

# Board options
set_property(CACHE ALLOY_BOARD PROPERTY STRINGS
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
)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(compiler_options)
include(toolchain_check)
include(board_flags)
include(flash_targets)
include(memory_analysis)
include(board_manifest)
include(alloy_devices)

# Resolve board metadata early so the descriptor-driven path does not depend on
# the legacy board/platform switch blocks below.
alloy_resolve_board_manifest(
    "${ALLOY_BOARD}"
    ALLOY_BOARD_MANIFEST_FOUND
    BOARD_HEADER_PATH
    LINKER_SCRIPT
    ALLOY_BOARD_VENDOR
    ALLOY_BOARD_FAMILY
    ALLOY_BOARD_DEVICE
    ALLOY_BOARD_DEVICE_ARCH
    ALLOY_BOARD_MCU
    ALLOY_BOARD_FLASH_SIZE_BYTES
    ALLOY_BOARD_SUPPORTS_PERIPHERAL_EXAMPLES
    ALLOY_BOARD_SUPPORTS_UART_LOGGER
    ALLOY_BOARD_SUPPORTS_DMA_PROBE
)

# Build options
option(ALLOY_BUILD_TESTS "Build unit tests" ON)
option(ALLOY_MINIMAL_BUILD "Optimize for size (enables -Os, -flto, --gc-sections)" OFF)
set(ALLOY_CATCH2_SOURCE_DIR
    ""
    CACHE PATH
    "Optional local Catch2 source checkout used for offline test configuration."
)

if(ALLOY_BUILD_TESTS)
    enable_testing()

    set(_alloy_catch2_source_dir "")
    set(_alloy_catch2_source_candidates)
    if(ALLOY_CATCH2_SOURCE_DIR)
        list(APPEND _alloy_catch2_source_candidates "${ALLOY_CATCH2_SOURCE_DIR}")
    endif()
    list(APPEND _alloy_catch2_source_candidates
        "${CMAKE_CURRENT_SOURCE_DIR}/build-tests-host/_deps/catch2-src"
        "${CMAKE_CURRENT_SOURCE_DIR}/build/presets/host-validation-debug/_deps/catch2-src"
        "${CMAKE_CURRENT_SOURCE_DIR}/build/presets/same70-analysis-debug/_deps/catch2-src"
        "${CMAKE_CURRENT_SOURCE_DIR}/build/presets/same70-renode-debug/_deps/catch2-src"
        "${CMAKE_CURRENT_SOURCE_DIR}/build/presets/stm32g0-renode-debug/_deps/catch2-src"
        "${CMAKE_CURRENT_SOURCE_DIR}/build/presets/stm32f4-renode-debug/_deps/catch2-src"
    )
    file(GLOB _alloy_catch2_glob_direct LIST_DIRECTORIES true
        "${CMAKE_CURRENT_SOURCE_DIR}/build*/_deps/catch2-src"
    )
    file(GLOB _alloy_catch2_glob_one_level LIST_DIRECTORIES true
        "${CMAKE_CURRENT_SOURCE_DIR}/build*/*/_deps/catch2-src"
    )
    file(GLOB _alloy_catch2_glob_two_level LIST_DIRECTORIES true
        "${CMAKE_CURRENT_SOURCE_DIR}/build*/*/*/_deps/catch2-src"
    )
    list(APPEND _alloy_catch2_source_candidates
        ${_alloy_catch2_glob_direct}
        ${_alloy_catch2_glob_one_level}
        ${_alloy_catch2_glob_two_level}
    )
    list(REMOVE_DUPLICATES _alloy_catch2_source_candidates)
    foreach(_alloy_catch2_candidate IN LISTS _alloy_catch2_source_candidates)
        if(EXISTS "${_alloy_catch2_candidate}/CMakeLists.txt")
            set(_alloy_catch2_source_dir "${_alloy_catch2_candidate}")
            break()
        endif()
    endforeach()

    # Fetch Catch2 v3, but prefer an existing local checkout when available so
    # host validation can configure offline.
    include(FetchContent)
    if(_alloy_catch2_source_dir)
        message(STATUS "Using local Catch2 source checkout: ${_alloy_catch2_source_dir}")
        FetchContent_Declare(
            Catch2
            SOURCE_DIR "${_alloy_catch2_source_dir}"
        )
    else()
        FetchContent_Declare(
            Catch2
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
    endif()

    FetchContent_MakeAvailable(Catch2)

    # Add Catch2 CMake modules to path
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()

# ESP32 boards require ESP-IDF integration
if(ALLOY_BOARD MATCHES "^esp32")
    include(cmake/platform/esp32_integration.cmake)
endif()

# Auto-detect platform from board if not explicitly set
if(NOT ALLOY_PLATFORM)
    if(ALLOY_BOARD_MANIFEST_FOUND)
        if(ALLOY_BOARD_DEVICE_ARCH STREQUAL "native")
            set(ALLOY_PLATFORM "linux")
        elseif(ALLOY_BOARD_FAMILY)
            set(ALLOY_PLATFORM "${ALLOY_BOARD_FAMILY}")
        endif()
    elseif(ALLOY_BOARD STREQUAL "same70_xpld" OR ALLOY_BOARD STREQUAL "same70_xplained")
        set(ALLOY_PLATFORM "same70")
    elseif(ALLOY_BOARD STREQUAL "host")
        set(ALLOY_PLATFORM "linux")
    elseif(ALLOY_BOARD MATCHES "^esp32")
        set(ALLOY_PLATFORM "esp32")
    elseif(ALLOY_BOARD MATCHES "^stm32f4" OR ALLOY_BOARD STREQUAL "stm32f407vg")
        set(ALLOY_PLATFORM "stm32f4")
    elseif(ALLOY_BOARD MATCHES "^stm32f1" OR ALLOY_BOARD STREQUAL "bluepill")
        set(ALLOY_PLATFORM "stm32f1")
    elseif(ALLOY_BOARD MATCHES "samd21" OR ALLOY_BOARD STREQUAL "arduino_zero")
        set(ALLOY_PLATFORM "samd21")
    elseif(ALLOY_BOARD MATCHES "rp2040" OR ALLOY_BOARD MATCHES "rp_pico")
        set(ALLOY_PLATFORM "rp2040")
    elseif(ALLOY_BOARD STREQUAL "nucleo_g0b1re" OR ALLOY_BOARD STREQUAL "nucleo_g071rb" OR ALLOY_BOARD MATCHES "^stm32g0")
        set(ALLOY_PLATFORM "stm32g0")
    elseif(ALLOY_BOARD STREQUAL "nucleo_f401re" OR ALLOY_BOARD MATCHES "^stm32f4" OR ALLOY_BOARD MATCHES "^nucleo_f4")
        set(ALLOY_PLATFORM "stm32f4")
    else()
        message(WARNING "Could not auto-detect ALLOY_PLATFORM from ALLOY_BOARD='${ALLOY_BOARD}'. Using 'linux' as fallback.")
        set(ALLOY_PLATFORM "linux")
    endif()
    message(STATUS "Auto-detected platform: ${ALLOY_PLATFORM} (from board: ${ALLOY_BOARD})")
endif()

# ============================================================================
# Board Abstraction Layer Configuration
# ============================================================================

if(ALLOY_BOARD_MANIFEST_FOUND)
    if(ALLOY_BOARD_MCU)
        set(ALLOY_MCU "${ALLOY_BOARD_MCU}" CACHE STRING "Target MCU (resolved from board manifest)" FORCE)
    endif()

    if(ALLOY_BOARD_DEVICE_ARCH STREQUAL "native")
        set(ALLOY_ARCH "native" CACHE STRING "Target architecture (resolved from board manifest)" FORCE)
    elseif(ALLOY_BOARD_DEVICE_ARCH)
        set(ALLOY_ARCH "arm-${ALLOY_BOARD_DEVICE_ARCH}" CACHE STRING "Target architecture (resolved from board manifest)" FORCE)
    endif()
elseif(ALLOY_BOARD STREQUAL "arduino_zero")
    set(BOARD_HEADER_PATH "boards/arduino_zero/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/boards/common/startup/atsamd21/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/arduino_zero/ATSAMD21G18.ld")
elseif(ALLOY_BOARD STREQUAL "waveshare_rp2040_zero" OR ALLOY_BOARD STREQUAL "rp_pico")
    set(BOARD_HEADER_PATH "boards/waveshare_rp2040_zero/board.hpp")
    set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/boards/common/startup/rp2040/startup.cpp")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/waveshare_rp2040_zero/RP2040.ld")
else()
    message(WARNING "Board '${ALLOY_BOARD}' not configured for board abstraction layer")
endif()

# Define BOARD_HEADER macro for use in examples
if(DEFINED BOARD_HEADER_PATH)
    get_filename_component(ALLOY_BOARD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/${BOARD_HEADER_PATH}" DIRECTORY)
    string(REPLACE "board.hpp" "startup.cpp" _board_startup_path "${BOARD_HEADER_PATH}")
    if(EXISTS "${CMAKE_SOURCE_DIR}/${_board_startup_path}")
        set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/${_board_startup_path}")
    endif()

    # Define BOARD_HEADER as a macro that expands to the header path
    # Use angle brackets for include: #include BOARD_HEADER expands to #include <boards/...>
    add_compile_definitions(BOARD_HEADER=<${BOARD_HEADER_PATH}>)
    message(STATUS "  Board Header: ${BOARD_HEADER_PATH}")

    foreach(_board_aux_header IN ITEMS uart i2c spi dma)
        string(REPLACE "board.hpp" "board_${_board_aux_header}.hpp" _board_aux_header_path "${BOARD_HEADER_PATH}")
        if(EXISTS "${CMAKE_SOURCE_DIR}/${_board_aux_header_path}")
            string(TOUPPER "${_board_aux_header}" _board_aux_header_define)
            add_compile_definitions(BOARD_${_board_aux_header_define}_HEADER=<${_board_aux_header_path}>)
            message(STATUS "  Board ${_board_aux_header} Header: ${_board_aux_header_path}")
        endif()
    endforeach()

    # Define ALLOY_BOARD_* for conditional compilation in examples
    string(TOUPPER "${ALLOY_BOARD}" ALLOY_BOARD_UPPER)
    add_compile_definitions(ALLOY_BOARD_${ALLOY_BOARD_UPPER})
    message(STATUS "  Board Define: ALLOY_BOARD_${ALLOY_BOARD_UPPER}")
endif()

# Resolve the selected descriptor contract from alloy-devices before platform and
# board modules run so they can disable legacy integration paths when possible.
alloy_configure_selected_device()

set(ALLOY_ACTIVE_STARTUP_SOURCE "")
set(ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE "")
if(ALLOY_DESCRIPTOR_RUNTIME_ENABLED
   AND ALLOY_DEVICE_GENERATED_STARTUP_SOURCE
   AND EXISTS "${ALLOY_DEVICE_GENERATED_STARTUP_SOURCE}")
    set(ALLOY_ACTIVE_STARTUP_SOURCE "${ALLOY_DEVICE_GENERATED_STARTUP_SOURCE}")
elseif(DEFINED STARTUP_SOURCE AND EXISTS "${STARTUP_SOURCE}")
    set(ALLOY_ACTIVE_STARTUP_SOURCE "${STARTUP_SOURCE}")
endif()

if(ALLOY_DEVICE_STARTUP_VECTORS_SOURCE
   AND EXISTS "${ALLOY_DEVICE_STARTUP_VECTORS_SOURCE}"
   AND NOT ALLOY_ACTIVE_STARTUP_SOURCE)
    set(ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE "${ALLOY_DEVICE_STARTUP_VECTORS_SOURCE}")
endif()

if(ALLOY_ACTIVE_STARTUP_SOURCE)
    set(STARTUP_SOURCE "${ALLOY_ACTIVE_STARTUP_SOURCE}")
    message(STATUS "  Startup Code: ${STARTUP_SOURCE}")
elseif(ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE)
    message(STATUS "  Startup Descriptors: ${ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE}")
endif()

# Include platform selection module (new HAL abstraction system)
# This must come before board configuration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform_selection.cmake)
    include(cmake/platform_selection.cmake)
endif()

# Include board selection module (new HAL abstraction system)
# This creates board-specific type aliases and peripheral mappings
if(ALLOY_DESCRIPTOR_RUNTIME_ENABLED)
    message(STATUS "Skipping legacy board_selection.cmake for descriptor-driven board: ${ALLOY_BOARD}")
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/board_selection.cmake)
    include(cmake/board_selection.cmake)
endif()

set(ALLOY_LEGACY_BOARD_CONFIG_ENABLED TRUE)
if(ALLOY_DESCRIPTOR_RUNTIME_ENABLED)
    set(ALLOY_LEGACY_BOARD_CONFIG_ENABLED FALSE)
endif()

# Load legacy board configuration (for backwards compatibility)
# TODO: Migrate legacy board configs to new board_selection system
if(ALLOY_LEGACY_BOARD_CONFIG_ENABLED AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/boards/${ALLOY_BOARD}.cmake)
    message(STATUS "Loading legacy board config: ${ALLOY_BOARD}")
    include(cmake/boards/${ALLOY_BOARD}.cmake)
elseif(ALLOY_DESCRIPTOR_RUNTIME_ENABLED)
    message(STATUS "Skipping legacy board config for descriptor-driven board: ${ALLOY_BOARD}")
else()
    # Board config not found in legacy location, that's OK
    # Board may use new system (boards/<board>/board.cmake)
endif()

# Validate toolchain for selected board (skip for ESP32, handled by ESP-IDF)
if(NOT ALLOY_BOARD MATCHES "^esp32")
    alloy_validate_toolchain()
endif()

# Display configuration
message(STATUS "========================================")
message(STATUS "Alloy Framework Configuration")
message(STATUS "========================================")
message(STATUS "  Board       : ${ALLOY_BOARD}")
message(STATUS "  Platform    : ${ALLOY_PLATFORM}")
message(STATUS "  MCU         : ${ALLOY_MCU}")
message(STATUS "  Architecture: ${ALLOY_ARCH}")
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
set(ALLOY_HAL_COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/core/concepts.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/core/types.hpp
)

# Core sources (Result, Error, etc. - header-only library)
# Note: Using explicit lists instead of GLOB for better CMake incremental builds
set(ALLOY_CORE_SOURCES
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
if(ALLOY_PLATFORM_SOURCES OR ALLOY_ACTIVE_STARTUP_SOURCE OR ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE)
    # Platform has .cpp files - create STATIC library
    # Include platform sources + startup code (if board defines one)
    set(HAL_SOURCES ${ALLOY_PLATFORM_SOURCES} ${ALLOY_CORE_SOURCES})
    if(ALLOY_ACTIVE_STARTUP_SOURCE AND EXISTS "${ALLOY_ACTIVE_STARTUP_SOURCE}")
        list(APPEND HAL_SOURCES ${ALLOY_ACTIVE_STARTUP_SOURCE})
    endif()
    if(ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE AND EXISTS "${ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE}")
        list(APPEND HAL_SOURCES ${ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE})
    endif()

    add_library(alloy-hal STATIC ${HAL_SOURCES})

    message(STATUS "Created STATIC alloy-hal library with platform sources")
else()
    # Platform is header-only (templates) - create INTERFACE library
    add_library(alloy-hal INTERFACE)

    message(STATUS "Created INTERFACE alloy-hal library (header-only platform)")
endif()

# Set include directories for HAL library
if(ALLOY_PLATFORM_SOURCES OR ALLOY_ACTIVE_STARTUP_SOURCE OR ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE)
    # STATIC library - use PUBLIC
    target_include_directories(alloy-hal
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${ALLOY_PLATFORM_DIR}
            ${ALLOY_DEVICE_INCLUDE_DIRS}
    )
    target_compile_features(alloy-hal PUBLIC cxx_std_23)
else()
    # INTERFACE library - use INTERFACE
    target_include_directories(alloy-hal
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${ALLOY_PLATFORM_DIR}
            ${ALLOY_DEVICE_INCLUDE_DIRS}
    )
    target_compile_features(alloy-hal INTERFACE cxx_std_23)
endif()

# Export HAL library for use by examples and tests
set(ALLOY_HAL_LIBRARY alloy-hal CACHE INTERNAL "Alloy HAL library target")

if(ALLOY_DEVICE_CONTRACT_AVAILABLE)
    message(STATUS "Selected device contract: ${ALLOY_DEVICE_VENDOR}/${ALLOY_DEVICE_FAMILY}/${ALLOY_DEVICE_NAME}")
else()
    message(STATUS "Selected device contract: unavailable for board '${ALLOY_BOARD}'")
endif()

if(ALLOY_DEVICE_CONTRACT_AVAILABLE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/tests/compile/contract_smoke.cmake)
    alloy_add_device_contract_smoke(alloy-device-contract-smoke)
endif()

# HAL subdirectories (conditional based on board)
if(ALLOY_BOARD STREQUAL "host" AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/host/CMakeLists.txt)
    add_subdirectory(src/hal/host)
endif()

# add_subdirectory(src/drivers)

# ==============================================================================
# Examples - Board Abstraction Layer
# ==============================================================================

# Generic examples - work on any board with board.hpp abstraction
# The same code compiles for different boards by including the appropriate board.hpp

# Universal blink example - demonstrates board abstraction power
if(ALLOY_BOARD_MANIFEST_FOUND AND NOT ALLOY_BOARD STREQUAL "host")
    add_subdirectory(examples/blink)
endif()

if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_BOARD_SUPPORTS_PERIPHERAL_EXAMPLES)
    add_subdirectory(examples/i2c_scan)
    add_subdirectory(examples/spi_probe)
endif()

# Universal RTOS example - demonstrates RTOS with multi-task scheduling
if(ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld" OR ALLOY_BOARD STREQUAL "nucleo_g0b1re" OR ALLOY_BOARD STREQUAL "nucleo_g071rb" OR ALLOY_BOARD STREQUAL "nucleo_f401re")
    add_subdirectory(examples/rtos/simple_tasks)
endif()

# Board-specific examples
if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_BOARD_SUPPORTS_UART_LOGGER)
    add_subdirectory(examples/uart_logger)
endif()

if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_BOARD_SUPPORTS_DMA_PROBE AND
   ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE)
    add_subdirectory(examples/dma_probe)
endif()

if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE AND
   ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE)
    add_subdirectory(examples/timer_pwm_probe)
endif()

# TODO: Add more boards as they implement board.hpp abstraction:
# if(ALLOY_BOARD STREQUAL "stm32f103_bluepill")
#     add_subdirectory(examples/blink)
# endif()
#
# if(ALLOY_BOARD STREQUAL "arduino_zero")
#     add_subdirectory(examples/blink)
# endif()

# Tests
if(ALLOY_BUILD_TESTS)
    add_subdirectory(tests)
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

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_custom_target(validate-runtime-boundary
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/check_runtime_device_boundary.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking descriptor-runtime boundary imports..."
        VERBATIM
    )
endif()

# Add custom target for testing board/platform validation
# This intentionally tries invalid combinations to verify error detection
add_custom_target(test-platform-validation
    COMMAND ${CMAKE_COMMAND} -E echo "Testing Board/Platform Validation..."
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Test 1: Valid combination (nucleo_g0b1re + stm32g0)"
    COMMAND ${CMAKE_COMMAND} -DALLOY_BOARD=nucleo_g0b1re -DALLOY_PLATFORM=stm32g0 -B build-validation-test1 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake > /dev/null 2>&1 && echo "✓ PASS" || echo "✗ FAIL"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Test 2: Invalid combination (nucleo_g0b1re + stm32f4) - should FAIL"
    COMMAND ${CMAKE_COMMAND} -DALLOY_BOARD=nucleo_g0b1re -DALLOY_PLATFORM=stm32f4 -B build-validation-test2 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake > /dev/null 2>&1 && echo "✗ FAIL (should have detected mismatch)" || echo "✓ PASS (correctly rejected)"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Validation test complete!"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Testing platform validation logic..."
    VERBATIM
)
