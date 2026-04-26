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
set(ALLOY_BOARD "host" CACHE STRING "Target board (host, same70_xplained, nucleo_f401re, nucleo_g071rb, nucleo_g0b1re, raspberry_pi_pico, avr128da32_curiosity_nano, esp32c3_devkitm, esp32s3_devkitc, esp32_devkit)")
set(ALLOY_MCU "" CACHE STRING "Target MCU (set by board configuration)")
set(ALLOY_ARCH "" CACHE STRING "Target architecture (set by board configuration)")

# Platform abstraction configuration
# ALLOY_PLATFORM is the HAL platform (same70, linux, stm32f4, stm32g0)
# It will be auto-detected from ALLOY_BOARD if not explicitly set
set(ALLOY_PLATFORM "" CACHE STRING "HAL platform (same70, linux, stm32f4, stm32g0)")

# Board options
set_property(CACHE ALLOY_BOARD PROPERTY STRINGS
    "host"
    "same70_xplained"             # Atmel/Microchip SAME70 Xplained Ultra
    "same70_xpld"                 # Atmel SAME70 Xplained (alias)
    "nucleo_g0b1re"               # STM32 Nucleo-G0B1RE
    "nucleo_g071rb"               # STM32 Nucleo-G071RB
    "nucleo_f401re"               # STM32 Nucleo-F401RE
    "raspberry_pi_pico"           # Raspberry Pi Pico (RP2040)
    "avr128da32_curiosity_nano"   # Microchip AVR128DA32 Curiosity Nano
    "esp32c3_devkitm"             # Espressif ESP32-C3-DevKitM-1
    "esp32s3_devkitc"             # Espressif ESP32-S3-DevKitC-1
    "esp32_devkit"                # Espressif ESP32-DevKit (Xtensa LX6)
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
include(CMakePackageConfigHelpers)

set(ALLOY_TARGET_COMPILE_DEFINITIONS "")

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
# Tests default ON when alloy is the top-level project, OFF when alloy is consumed
# via add_subdirectory() from a downstream project (the typical `alloy new` flow).
# Downstream projects that DO want alloy's tests can flip this back ON via cache.
if(PROJECT_IS_TOP_LEVEL)
    option(ALLOY_BUILD_TESTS "Build unit tests" ON)
else()
    option(ALLOY_BUILD_TESTS "Build unit tests" OFF)
endif()
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
    elseif(ALLOY_BOARD STREQUAL "nucleo_g0b1re" OR ALLOY_BOARD STREQUAL "nucleo_g071rb" OR ALLOY_BOARD MATCHES "^stm32g0")
        set(ALLOY_PLATFORM "stm32g0")
    elseif(ALLOY_BOARD STREQUAL "nucleo_f401re" OR ALLOY_BOARD MATCHES "^stm32f4" OR ALLOY_BOARD MATCHES "^nucleo_f4")
        set(ALLOY_PLATFORM "stm32f4")
    elseif(ALLOY_BOARD STREQUAL "esp32_devkit")
        set(ALLOY_PLATFORM "esp32")
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
else()
    message(WARNING "Board '${ALLOY_BOARD}' not configured for board abstraction layer")
endif()

# Define BOARD_HEADER macro for use in examples
if(DEFINED BOARD_HEADER_PATH)
    get_filename_component(ALLOY_BOARD_SOURCE_DIR "${CMAKE_SOURCE_DIR}/${BOARD_HEADER_PATH}" DIRECTORY)
    string(REPLACE "board.hpp" "startup.cpp" _board_startup_path "${BOARD_HEADER_PATH}")
    if(EXISTS "${CMAKE_SOURCE_DIR}/${_board_startup_path}")
        set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/${_board_startup_path}")
    else()
        # Some boards (Xtensa) provide startup.S instead of startup.cpp
        string(REPLACE "board.hpp" "startup.S" _board_startup_asm "${BOARD_HEADER_PATH}")
        if(EXISTS "${CMAKE_SOURCE_DIR}/${_board_startup_asm}")
            set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/${_board_startup_asm}")
        endif()
    endif()

    # Define BOARD_HEADER as a macro that expands to the header path
    # Use angle brackets for include: #include BOARD_HEADER expands to #include <boards/...>
    add_compile_definitions(BOARD_HEADER=<${BOARD_HEADER_PATH}>)
    list(APPEND ALLOY_TARGET_COMPILE_DEFINITIONS BOARD_HEADER=<${BOARD_HEADER_PATH}>)
    message(STATUS "  Board Header: ${BOARD_HEADER_PATH}")

    foreach(_board_aux_header IN ITEMS uart i2c spi dma analog watchdog)
        string(REPLACE "board.hpp" "board_${_board_aux_header}.hpp" _board_aux_header_path "${BOARD_HEADER_PATH}")
        if(EXISTS "${CMAKE_SOURCE_DIR}/${_board_aux_header_path}")
            string(TOUPPER "${_board_aux_header}" _board_aux_header_define)
            add_compile_definitions(BOARD_${_board_aux_header_define}_HEADER=<${_board_aux_header_path}>)
            list(APPEND ALLOY_TARGET_COMPILE_DEFINITIONS BOARD_${_board_aux_header_define}_HEADER=<${_board_aux_header_path}>)
            message(STATUS "  Board ${_board_aux_header} Header: ${_board_aux_header_path}")
        endif()
    endforeach()

    # Define ALLOY_BOARD_* for conditional compilation in examples
    string(TOUPPER "${ALLOY_BOARD}" ALLOY_BOARD_UPPER)
    add_compile_definitions(ALLOY_BOARD_${ALLOY_BOARD_UPPER})
    list(APPEND ALLOY_TARGET_COMPILE_DEFINITIONS ALLOY_BOARD_${ALLOY_BOARD_UPPER})
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

alloy_validate_toolchain()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/result.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/types.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/units.hpp
)

# Create HAL library (INTERFACE for header-only, or STATIC if platform has .cpp files)
if(ALLOY_PLATFORM_SOURCES OR ALLOY_ACTIVE_STARTUP_SOURCE OR ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE)
    # Platform has .cpp files - create STATIC library
    # Include platform sources + startup code (if board defines one)
    set(HAL_SOURCES ${ALLOY_PLATFORM_SOURCES} ${ALLOY_CORE_SOURCES})
    list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/interrupt_bridge.cpp)

    # Cooperative coroutine scheduler (runtime-tasks). avr-gcc 13.x does not
    # implement C++20 coroutines, so the scheduler refuses to build on that
    # architecture. See openspec/specs/runtime-tasks/spec.md.
    if(NOT ALLOY_BOARD_DEVICE_ARCH STREQUAL "avr")
        list(APPEND HAL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/tasks/scheduler.cpp)
    endif()
    if(ALLOY_ACTIVE_STARTUP_SOURCE AND EXISTS "${ALLOY_ACTIVE_STARTUP_SOURCE}")
        list(APPEND HAL_SOURCES ${ALLOY_ACTIVE_STARTUP_SOURCE})
    endif()
    if(ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE AND EXISTS "${ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE}")
        list(APPEND HAL_SOURCES ${ALLOY_ACTIVE_STARTUP_DESCRIPTOR_SOURCE})
    endif()

    add_library(alloy-hal STATIC ${HAL_SOURCES})

    # Force interrupt_bridge.cpp.obj to be extracted from the archive at link time.
    # startup.cpp's weak ISR stubs would otherwise satisfy all references, causing
    # the linker to skip interrupt_bridge.cpp.obj and leave the strong ISR overrides unused.
    target_link_options(alloy-hal INTERFACE -u _alloy_interrupt_bridge_anchor)

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
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${ALLOY_PLATFORM_DIR}
            ${ALLOY_DEVICE_INCLUDE_DIRS}
    )
    target_compile_features(alloy-hal PUBLIC cxx_std_23)
    if(ALLOY_TARGET_COMPILE_DEFINITIONS)
        target_compile_definitions(alloy-hal PUBLIC ${ALLOY_TARGET_COMPILE_DEFINITIONS})
    endif()
else()
    # INTERFACE library - use INTERFACE
    target_include_directories(alloy-hal
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${ALLOY_PLATFORM_DIR}
            ${ALLOY_DEVICE_INCLUDE_DIRS}
    )
    target_compile_features(alloy-hal INTERFACE cxx_std_23)
    if(ALLOY_TARGET_COMPILE_DEFINITIONS)
        target_compile_definitions(alloy-hal INTERFACE ${ALLOY_TARGET_COMPILE_DEFINITIONS})
    endif()
endif()

add_library(Alloy::hal ALIAS alloy-hal)
add_library(alloy::alloy ALIAS alloy-hal)   # lowercase alias used by drivers/examples
set_target_properties(alloy-hal PROPERTIES EXPORT_NAME hal)

# Export HAL library for use by examples and tests
set(ALLOY_HAL_LIBRARY alloy-hal CACHE INTERNAL "Alloy HAL library target")

if(ALLOY_DEVICE_CONTRACT_AVAILABLE)
    message(STATUS "Selected device contract: ${ALLOY_DEVICE_VENDOR}/${ALLOY_DEVICE_FAMILY}/${ALLOY_DEVICE_NAME}")
else()
    message(STATUS "Selected device contract: unavailable for board '${ALLOY_BOARD}'")
endif()

# The contract smoke is an alloy-internal compile-time check; downstream
# projects consuming alloy via add_subdirectory() should not pay for it (and
# its compile_tests/ assume a specific board context that may not match).
if(ALLOY_DEVICE_CONTRACT_AVAILABLE AND PROJECT_IS_TOP_LEVEL)
    include(${CMAKE_CURRENT_SOURCE_DIR}/tests/compile/contract_smoke.cmake)
    alloy_add_device_contract_smoke(alloy-device-contract-smoke)
endif()

# HAL subdirectories (conditional based on board)
if(ALLOY_BOARD STREQUAL "host" AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/host/CMakeLists.txt)
    add_subdirectory(src/hal/host)
endif()

option(ALLOY_BUILD_DRIVERS "Build the alloy seed driver library interface target" ON)
if(ALLOY_BUILD_DRIVERS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/drivers/CMakeLists.txt)
    add_subdirectory(drivers)
endif()

# ==============================================================================
# Examples - Board Abstraction Layer
# ==============================================================================
#
# Examples build only when alloy is the top-level project. Downstream projects
# consuming alloy via add_subdirectory() do not want alloy's example targets
# (which assume specific in-tree board names) to compile alongside their own
# code. Override with -DALLOY_BUILD_EXAMPLES=ON if needed.

if(PROJECT_IS_TOP_LEVEL)
    option(ALLOY_BUILD_EXAMPLES "Build alloy in-tree example targets" ON)
else()
    option(ALLOY_BUILD_EXAMPLES "Build alloy in-tree example targets" OFF)
endif()

# Generic examples - work on any board with board.hpp abstraction
# The same code compiles for different boards by including the appropriate board.hpp

if(ALLOY_BUILD_EXAMPLES)
    # Universal blink example - demonstrates board abstraction power
    if(ALLOY_BOARD_MANIFEST_FOUND AND NOT ALLOY_BOARD STREQUAL "host")
        add_subdirectory(examples/blink)
        add_subdirectory(examples/time_probe)
        # Cooperative coroutine scheduler demo. avr-gcc 13.x lacks C++20
        # coroutines, so skip on that arch (the runtime-tasks library refuses
        # to build there too).
        if(NOT ALLOY_BOARD_DEVICE_ARCH STREQUAL "avr")
            add_subdirectory(examples/tasks_blink_uart)
        endif()
    endif()

    if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_DEVICE_ADC_SEMANTICS_AVAILABLE AND ALLOY_BOARD_SUPPORTS_PERIPHERAL_EXAMPLES)
        add_subdirectory(examples/analog_probe)
    endif()

    if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_BOARD_SUPPORTS_PERIPHERAL_EXAMPLES)
        add_subdirectory(examples/i2c_scan)
        add_subdirectory(examples/spi_probe)
    endif()

    # Board-specific examples
    if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_BOARD_SUPPORTS_UART_LOGGER)
        add_subdirectory(examples/uart_logger)
    endif()

    if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_BOARD_SUPPORTS_DMA_PROBE AND
       ALLOY_DEVICE_DMA_BINDINGS_AVAILABLE)
        add_subdirectory(examples/dma_probe)
        add_subdirectory(examples/async_uart_timeout)
    endif()

    if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_DEVICE_TIMER_SEMANTICS_AVAILABLE AND
       ALLOY_DEVICE_PWM_SEMANTICS_AVAILABLE AND ALLOY_BOARD_SUPPORTS_PERIPHERAL_EXAMPLES)
        add_subdirectory(examples/timer_pwm_probe)
    endif()

    if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_DEVICE_RTC_SEMANTICS_AVAILABLE AND
       ALLOY_BOARD_SUPPORTS_PERIPHERAL_EXAMPLES)
        add_subdirectory(examples/rtc_probe)
    endif()

    if(ALLOY_BOARD_MANIFEST_FOUND AND ALLOY_DEVICE_WATCHDOG_SEMANTICS_AVAILABLE AND
       ALLOY_BOARD_SUPPORTS_PERIPHERAL_EXAMPLES)
        add_subdirectory(examples/watchdog_probe)
    endif()

    if((ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld") AND
       ALLOY_DEVICE_CAN_SEMANTICS_AVAILABLE)
        add_subdirectory(examples/can_probe)
    endif()

    if(ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld")
        add_subdirectory(examples/uart_path_probe)
        add_subdirectory(examples/manual_uart_probe)
        add_subdirectory(examples/driver_at24mac402_probe)
        add_subdirectory(examples/driver_ksz8081_probe)
        add_subdirectory(examples/driver_is42s16100f_probe)
        add_subdirectory(examples/driver_sht4x_probe)
        add_subdirectory(examples/driver_aht20_probe)
        add_subdirectory(examples/driver_lps22hh_probe)
        add_subdirectory(examples/driver_icm42688p_probe)
        add_subdirectory(examples/driver_mpu6050_probe)
        add_subdirectory(examples/driver_lsm6dsox_probe)
        add_subdirectory(examples/driver_lis3mdl_probe)
        add_subdirectory(examples/driver_nmea_parser_probe)
        add_subdirectory(examples/filesystem_littlefs)
        add_subdirectory(examples/filesystem_fatfs_sdcard)
    endif()

    if(ALLOY_BOARD STREQUAL "nucleo_g071rb")
        add_subdirectory(examples/early_boot_probe)
        add_subdirectory(examples/clock_boot_probe)
    endif()

    if(ALLOY_BOARD STREQUAL "esp32c3_devkitm")
        add_subdirectory(examples/hello_esp32c3)
    endif()

    if(ALLOY_BOARD STREQUAL "esp32_devkit")
        add_subdirectory(examples/hello_esp32)
    endif()
endif()

# Tests
if(ALLOY_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Build-Tree CMake Package Export
# ============================================================================== 

set(ALLOY_PACKAGE_CONFIG_DIR "${CMAKE_BINARY_DIR}/generated/cmake")
file(MAKE_DIRECTORY "${ALLOY_PACKAGE_CONFIG_DIR}")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/AlloyConfig.cmake.in"
    "${ALLOY_PACKAGE_CONFIG_DIR}/AlloyConfig.cmake"
    INSTALL_DESTINATION "${ALLOY_PACKAGE_CONFIG_DIR}"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/AlloyHelpers.cmake.in"
    "${ALLOY_PACKAGE_CONFIG_DIR}/AlloyHelpers.cmake"
    @ONLY
)

write_basic_package_version_file(
    "${ALLOY_PACKAGE_CONFIG_DIR}/AlloyConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

export(TARGETS alloy-hal
    NAMESPACE Alloy::
    FILE "${ALLOY_PACKAGE_CONFIG_DIR}/AlloyTargets.cmake"
)

message(STATUS "Alloy build-tree package config: ${ALLOY_PACKAGE_CONFIG_DIR}")

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
