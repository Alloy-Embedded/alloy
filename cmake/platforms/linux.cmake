# ==============================================================================
# Linux Platform Configuration
# ==============================================================================
#
# Platform: Linux/POSIX (Host-based testing)
# Architecture: x86_64, ARM64, or native host architecture
# Purpose: Test HAL code on development machine without hardware
#
# This file configures CMake for building Alloy HAL on Linux.
# It sets up:
#   - Host compiler (g++, clang++)
#   - POSIX API usage (termios for UART, sysfs for GPIO)
#   - Compiler flags for testing
#   - Platform-specific definitions
#
# Key Use Cases:
#   1. Unit tests without hardware (CI/CD)
#   2. Rapid development iteration (no flashing required)
#   3. Integration tests with real peripherals (via USB adapters)
#   4. Debugging with standard tools (gdb, valgrind, sanitizers)
#
# @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
# ==============================================================================

message(STATUS "Configuring for Linux platform (POSIX/host testing)")

# ------------------------------------------------------------------------------
# Toolchain Requirements
# ------------------------------------------------------------------------------

# Linux platform uses native host compiler (g++ or clang++)
# No cross-compilation required

if(CMAKE_CROSSCOMPILING)
    message(WARNING
        "Cross-compiling Linux platform - this is unusual!\n"
        "Linux platform is typically built for the host architecture.\n"
        "If you're targeting an embedded Linux board, consider creating\n"
        "a separate platform (e.g., 'linux-arm' or use a specific board config)."
    )
endif()

# Verify we're on a Unix-like system
if(NOT UNIX)
    message(FATAL_ERROR
        "Linux platform requires a Unix-like system (Linux, macOS, BSD).\n"
        "Current system: ${CMAKE_SYSTEM_NAME}\n"
        "For Windows, consider using WSL or creating a 'windows' platform."
    )
endif()

message(STATUS "  Host OS: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Host Arch: ${CMAKE_SYSTEM_PROCESSOR}")

# ------------------------------------------------------------------------------
# Platform-Specific Compile Definitions
# ------------------------------------------------------------------------------

# POSIX feature test macros
add_compile_definitions(
    _POSIX_C_SOURCE=200809L     # POSIX.1-2008 (for termios, etc.)
    _DEFAULT_SOURCE             # Enable default features (glibc)
)

# Optional: Enable large file support on 32-bit systems
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_compile_definitions(
        _FILE_OFFSET_BITS=64    # 64-bit file offsets on 32-bit systems
    )
endif()

# ------------------------------------------------------------------------------
# Compiler Flags
# ------------------------------------------------------------------------------

# Optimization flags (can be overridden by CMAKE_BUILD_TYPE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
        -O0                     # No optimization for debugging
        -g3                     # Full debug info
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
        -O3                     # Maximum optimization
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
    -Wpedantic
    -Wno-unused-parameter       # Common in HAL code with unused params
)

# Linux-specific flags (different from embedded)
# Enable exceptions and RTTI (unlike embedded platforms)
# add_compile_options(-fexceptions -frtti)

# Optional: Enable sanitizers for debugging
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "  AddressSanitizer: ENABLED")
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
    message(STATUS "  UndefinedBehaviorSanitizer: ENABLED")
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
    message(STATUS "  ThreadSanitizer: ENABLED")
endif()

# Optional: Enable code coverage
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
    message(STATUS "  Code coverage: ENABLED")
endif()

# ------------------------------------------------------------------------------
# Platform-Specific Libraries
# ------------------------------------------------------------------------------

# Link with pthread (required for some POSIX features)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

# Link with math library
link_libraries(m)

# Optional: Link with realtime library (for clock_gettime, etc.)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    link_libraries(rt)
endif()

# ------------------------------------------------------------------------------
# Platform Source Requirements
# ------------------------------------------------------------------------------

# Set platform directory (required by platform_selection.cmake)
set(ALLOY_PLATFORM_DIR ${CMAKE_SOURCE_DIR}/src/hal/vendors/linux)

# Linux platform requires these source files (verified at build time):
#   - src/hal/vendors/linux/uart.cpp      (uses termios for serial ports)
#   - src/hal/vendors/linux/gpio.cpp      (uses sysfs or simulation)
#   - src/hal/vendors/linux/i2c.cpp       (uses /dev/i2c-* devices)
#   - src/hal/vendors/linux/spi.cpp       (uses /dev/spidev* devices)
#
# These will be automatically collected by platform_selection.cmake

# ------------------------------------------------------------------------------
# Testing Configuration
# ------------------------------------------------------------------------------

# Enable testing for Linux platform (optional)
option(ENABLE_TESTING "Enable testing on Linux platform" ON)

if(ENABLE_TESTING)
    enable_testing()
    message(STATUS "  Testing: ENABLED")

    # Find Google Test (optional)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "  Google Test: Found")
    else()
        message(STATUS "  Google Test: Not found (tests will use custom framework)")
    endif()
endif()

# ------------------------------------------------------------------------------
# Development Tools
# ------------------------------------------------------------------------------

# Check for common development tools
find_program(VALGRIND valgrind)
if(VALGRIND)
    message(STATUS "  Valgrind: Found (${VALGRIND})")
else()
    message(STATUS "  Valgrind: Not found")
endif()

find_program(GDB gdb)
if(GDB)
    message(STATUS "  GDB: Found (${GDB})")
else()
    message(STATUS "  GDB: Not found")
endif()

# ------------------------------------------------------------------------------
# Platform-Specific Notes
# ------------------------------------------------------------------------------

message(STATUS "")
message(STATUS "Linux Platform Notes:")
message(STATUS "  - UART: Uses /dev/ttyUSB*, /dev/ttyACM*, or /dev/ttyS* devices")
message(STATUS "  - GPIO: Uses /sys/class/gpio/* (sysfs) or simulation")
message(STATUS "  - I2C:  Uses /dev/i2c-* devices (requires i2c-tools)")
message(STATUS "  - SPI:  Uses /dev/spidev* devices (requires spidev kernel module)")
message(STATUS "")
message(STATUS "Permissions may be required to access devices:")
message(STATUS "  - Add user to 'dialout' group for serial ports")
message(STATUS "  - Add user to 'gpio', 'i2c', 'spi' groups for peripherals")
message(STATUS "  - Or run with sudo (not recommended for development)")
message(STATUS "")

# ------------------------------------------------------------------------------
# Platform Summary
# ------------------------------------------------------------------------------

message(STATUS "------------------------------------------------------------------------------")
message(STATUS "Linux Platform Configuration Complete")
message(STATUS "------------------------------------------------------------------------------")
