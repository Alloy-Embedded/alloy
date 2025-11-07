# ==============================================================================
# ESP32 Platform Configuration
# ==============================================================================
#
# Platform: Espressif ESP32 (Xtensa LX6 + ESP-IDF)
# Architecture: Xtensa LX6 dual-core @ up to 240 MHz
# SDK: ESP-IDF (Espressif IoT Development Framework)
# Flash: Up to 16 MB external SPI flash
# RAM: 520 KB SRAM
#
# This file configures CMake for building Alloy HAL on ESP32 using ESP-IDF.
# It sets up:
#   - ESP-IDF integration and toolchain
#   - Compiler flags (CPU, optimization)
#   - Platform-specific definitions
#   - ESP-IDF component dependencies
#
# Prerequisites:
#   - ESP-IDF must be installed and sourced (export.sh / export.bat)
#   - IDF_PATH environment variable must be set
#   - xtensa-esp32-elf toolchain must be in PATH
#
# Usage:
#   source $IDF_PATH/export.sh
#   cmake -DALLOY_PLATFORM=esp32 -B build-esp32
#
# @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
# ==============================================================================

message(STATUS "Configuring for ESP32 platform (Xtensa LX6 + ESP-IDF)")

# ------------------------------------------------------------------------------
# ESP-IDF Verification
# ------------------------------------------------------------------------------

# Check if IDF_PATH is set
if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR
        "IDF_PATH environment variable is not set!\n"
        "ESP32 platform requires ESP-IDF to be installed and sourced.\n"
        "Please run:\n"
        "  source <path-to-esp-idf>/export.sh  (Linux/macOS)\n"
        "  <path-to-esp-idf>\\export.bat       (Windows)\n"
        "Then retry the build."
    )
endif()

set(IDF_PATH $ENV{IDF_PATH})
message(STATUS "  ESP-IDF path: ${IDF_PATH}")

# Verify ESP-IDF version (optional, but recommended)
if(EXISTS "${IDF_PATH}/version.txt")
    file(READ "${IDF_PATH}/version.txt" ESP_IDF_VERSION)
    string(STRIP "${ESP_IDF_VERSION}" ESP_IDF_VERSION)
    message(STATUS "  ESP-IDF version: ${ESP_IDF_VERSION}")

    # Minimum required ESP-IDF version (v4.4 or later recommended)
    if(ESP_IDF_VERSION VERSION_LESS "4.4")
        message(WARNING
            "ESP-IDF version ${ESP_IDF_VERSION} is older than recommended (v4.4+).\n"
            "Some features may not work correctly."
        )
    endif()
else()
    message(STATUS "  ESP-IDF version: Unknown (version.txt not found)")
endif()

# ------------------------------------------------------------------------------
# Toolchain Requirements
# ------------------------------------------------------------------------------

# ESP32 requires Xtensa toolchain (xtensa-esp32-elf-gcc)
# This should be set up automatically by ESP-IDF's export script

if(NOT CMAKE_CROSSCOMPILING)
    message(WARNING
        "Building ESP32 platform without cross-compilation!\n"
        "This platform requires Xtensa toolchain (xtensa-esp32-elf-gcc).\n"
        "Please ensure ESP-IDF is properly sourced."
    )
endif()

# Verify we're using the correct architecture
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "xtensa")
    message(WARNING
        "CMAKE_SYSTEM_PROCESSOR='${CMAKE_SYSTEM_PROCESSOR}' does not match expected 'xtensa'\n"
        "Ensure you're using the correct ESP-IDF toolchain"
    )
endif()

# ------------------------------------------------------------------------------
# Platform-Specific Compile Definitions
# ------------------------------------------------------------------------------

# ESP32-specific defines
add_compile_definitions(
    ESP_PLATFORM                # ESP-IDF platform identifier
    ESP32                       # ESP32 variant
)

# Optional: Enable ESP32-specific features
# add_compile_definitions(CONFIG_FREERTOS_UNICORE)  # Single-core mode

# ------------------------------------------------------------------------------
# Compiler Flags
# ------------------------------------------------------------------------------

# Optimization flags (can be overridden by CMAKE_BUILD_TYPE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
        -Og                     # Optimize for debugging
        -g3                     # Full debug info
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
        -O2                     # Optimize for speed
        -g                      # Keep debug symbols for profiling
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    add_compile_options(
        -Os                     # Optimize for size
        -g                      # Keep debug symbols
    )
endif()

# Warning flags
add_compile_options(
    -Wall
    -Wextra
    -Wno-unused-parameter       # Common in HAL code with unused params
    -Wno-sign-compare           # ESP-IDF headers generate these
)

# ESP32-specific flags
add_compile_options(
    -ffunction-sections         # Each function in its own section (for --gc-sections)
    -fdata-sections             # Each data item in its own section
    -fno-exceptions             # No C++ exceptions (saves space)
    -fno-rtti                   # No RTTI (saves space)
)

# ------------------------------------------------------------------------------
# Linker Flags
# ------------------------------------------------------------------------------

# Linker optimization flags
add_link_options(
    -Wl,--gc-sections           # Remove unused sections
    -Wl,--cref                  # Output cross-reference table
)

# ------------------------------------------------------------------------------
# ESP-IDF Component Dependencies
# ------------------------------------------------------------------------------

# List of ESP-IDF components required by Alloy HAL
# These will be linked into the final application

set(ALLOY_ESP32_IDF_COMPONENTS
    driver          # Hardware driver components (UART, GPIO, I2C, SPI, etc.)
    esp_common      # Common ESP32 functionality
    esp_system      # System initialization
    freertos        # FreeRTOS (ESP32 requires an RTOS)
    hal             # ESP32 hardware abstraction layer
    soc             # SoC-specific definitions
    newlib          # Standard C library implementation
)

# Export components for use by ESP-IDF build system
set(ALLOY_ESP32_IDF_COMPONENTS ${ALLOY_ESP32_IDF_COMPONENTS} PARENT_SCOPE)

message(STATUS "  Required ESP-IDF components: ${ALLOY_ESP32_IDF_COMPONENTS}")

# ------------------------------------------------------------------------------
# Platform Source Requirements
# ------------------------------------------------------------------------------

# ESP32 platform requires these source files (verified at build time):
#   - src/hal/platform/esp32/uart.cpp      (wrapper around ESP-IDF UART driver)
#   - src/hal/platform/esp32/gpio.cpp      (wrapper around ESP-IDF GPIO driver)
#   - src/hal/platform/esp32/i2c.cpp       (wrapper around ESP-IDF I2C driver)
#   - src/hal/platform/esp32/spi.cpp       (wrapper around ESP-IDF SPI driver)
#
# These will be automatically collected by platform_selection.cmake

# ------------------------------------------------------------------------------
# ESP-IDF Integration Notes
# ------------------------------------------------------------------------------

message(STATUS "")
message(STATUS "ESP32 Platform Notes:")
message(STATUS "  - ESP-IDF integration: Alloy HAL wraps ESP-IDF drivers")
message(STATUS "  - RTOS: FreeRTOS is required (ESP32 architecture constraint)")
message(STATUS "  - Memory: Uses ESP32 SRAM (520 KB) and external flash")
message(STATUS "  - Peripherals: UART, GPIO, I2C, SPI are ESP-IDF driver wrappers")
message(STATUS "")
message(STATUS "Build Process:")
message(STATUS "  1. Alloy HAL sources are compiled as an ESP-IDF component")
message(STATUS "  2. Link with ESP-IDF components (driver, freertos, etc.)")
message(STATUS "  3. Generate ESP32 firmware image (.bin)")
message(STATUS "  4. Flash to ESP32 using esptool.py or idf.py")
message(STATUS "")

# ------------------------------------------------------------------------------
# Toolchain Verification
# ------------------------------------------------------------------------------

# Verify compiler is xtensa-esp32-elf-gcc
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} --version
        OUTPUT_VARIABLE GCC_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GCC_VERSION_OUTPUT MATCHES "xtensa-esp32-elf")
        message(STATUS "  Toolchain: Xtensa GCC for ESP32 (xtensa-esp32-elf-gcc)")
    else()
        message(WARNING
            "Compiler does not appear to be xtensa-esp32-elf-gcc:\n"
            "${GCC_VERSION_OUTPUT}"
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# Platform Summary
# ------------------------------------------------------------------------------

message(STATUS "------------------------------------------------------------------------------")
message(STATUS "ESP32 Platform Configuration Complete")
message(STATUS "------------------------------------------------------------------------------")
