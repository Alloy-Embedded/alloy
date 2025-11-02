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
set(ALLOY_BOARD "host" CACHE STRING "Target board (host, bluepill, esp32_devkit, stm32f407vg, arduino_zero, rp_pico)")
set(ALLOY_MCU "" CACHE STRING "Target MCU (set by board configuration)")
set(ALLOY_ARCH "" CACHE STRING "Target architecture (set by board configuration)")

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

# Build options
option(ALLOY_BUILD_TESTS "Build unit tests" ON)
option(ALLOY_MINIMAL_BUILD "Optimize for size (enables -Os, -flto, --gc-sections)" OFF)

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

# ESP32 boards require ESP-IDF integration
if(ALLOY_BOARD MATCHES "^esp32")
    include(cmake/platform/esp32_integration.cmake)
endif()

# Load board configuration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/boards/${ALLOY_BOARD}.cmake)
    message(STATUS "Loading board: ${ALLOY_BOARD}")
    include(cmake/boards/${ALLOY_BOARD}.cmake)
else()
    message(WARNING "Board '${ALLOY_BOARD}' not found, using defaults")
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

# Board-specific blink examples
if(ALLOY_BOARD STREQUAL "bluepill")
    add_subdirectory(examples/blink_stm32f103)
    add_subdirectory(examples/rtos_blink)
    add_subdirectory(examples/rtos_queue)
    add_subdirectory(examples/rtos_semaphore)
    add_subdirectory(examples/rtos_mutex)
    add_subdirectory(examples/rtos_events)
endif()

if(ALLOY_BOARD STREQUAL "esp32_devkit")
    add_subdirectory(examples/blink_esp32)
    add_subdirectory(examples/rtos_blink_esp32)
endif()

if(ALLOY_BOARD STREQUAL "stm32f407vg")
    add_subdirectory(examples/blink_stm32f407)
endif()

if(ALLOY_BOARD STREQUAL "arduino_zero")
    add_subdirectory(examples/blink_arduino_zero)
endif()

if(ALLOY_BOARD STREQUAL "rp_pico")
    add_subdirectory(examples/blink_rp_pico)
    add_subdirectory(examples/rtos_blink_pico)
endif()

if(ALLOY_BOARD STREQUAL "rp2040_zero")
    add_subdirectory(examples/blink_rp2040_zero)
    add_subdirectory(examples/blink_rp2040_zero_pio)
endif()

if(ALLOY_BOARD STREQUAL "same70_xpld")
    add_subdirectory(examples/blink_same70)
    add_subdirectory(examples/same70_blink_leds)
    add_subdirectory(examples/same70_uart_hello)
endif()

# Tests
if(ALLOY_BUILD_TESTS)
    add_subdirectory(tests/unit)
endif()

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
