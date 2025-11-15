# ==============================================================================
# SAME70 Platform Configuration
# ==============================================================================
#
# Platform: Atmel/Microchip SAME70 (ARM Cortex-M7)
# Architecture: ARMv7E-M with FPU
# Core: ARM Cortex-M7 @ up to 300 MHz
# Flash: Up to 2 MB
# RAM: Up to 384 KB
#
# This file configures CMake for building Alloy HAL on SAME70.
# It sets up:
#   - Toolchain requirements (arm-none-eabi-gcc)
#   - Compiler flags (CPU, FPU, optimization)
#   - Linker flags and scripts
#   - Platform-specific definitions
#
# @see openspec/changes/platform-abstraction/specs/platform-interface-layer/spec.md
# ==============================================================================

message(STATUS "Configuring for SAME70 platform (ARM Cortex-M7)")

# ------------------------------------------------------------------------------
# Platform Directory
# ------------------------------------------------------------------------------

# Set platform directory for this family
set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/vendors/atmel/same70)

# ------------------------------------------------------------------------------
# Toolchain Requirements
# ------------------------------------------------------------------------------

# SAME70 requires ARM bare-metal toolchain (arm-none-eabi-gcc)
# The toolchain file should be specified with: -DCMAKE_TOOLCHAIN_FILE=...

if(NOT CMAKE_CROSSCOMPILING)
    message(WARNING
        "Building SAME70 platform without cross-compilation!\n"
        "This platform requires ARM bare-metal toolchain (arm-none-eabi-gcc).\n"
        "Please specify toolchain file:\n"
        "  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-none-eabi.cmake"
    )
endif()

# Verify we're using the correct architecture
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    message(WARNING
        "CMAKE_SYSTEM_PROCESSOR='${CMAKE_SYSTEM_PROCESSOR}' does not match expected 'arm'\n"
        "Ensure you're using the correct toolchain file for ARM Cortex-M"
    )
endif()

# ------------------------------------------------------------------------------
# CPU and FPU Configuration
# ------------------------------------------------------------------------------

# Cortex-M7 CPU flags
set(SAME70_CPU_FLAGS
    -mcpu=cortex-m7
    -mthumb
    -mfloat-abi=hard           # Hardware FPU
    -mfpu=fpv5-d16             # FPv5 double-precision FPU (16 registers)
)

# Add CPU flags to compile and link
add_compile_options(${SAME70_CPU_FLAGS})
add_link_options(${SAME70_CPU_FLAGS})

message(STATUS "  CPU: ARM Cortex-M7")
message(STATUS "  FPU: FPv5 double-precision (hard ABI)")

# ------------------------------------------------------------------------------
# Platform-Specific Compile Definitions
# ------------------------------------------------------------------------------

# Microchip/Atmel specific defines (if using vendor headers)
add_compile_definitions(
    __SAME70Q21B__              # Specific variant (adjust as needed)
    ARM_MATH_CM7                # ARM CMSIS DSP library support
)

# Optional: Enable ARM CMSIS if available
# add_compile_definitions(ARM_CMSIS)

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
    -Wpedantic
    -Wno-unused-parameter       # Common in HAL code with unused params
)

# Embedded-specific flags
add_compile_options(
    -ffunction-sections         # Each function in its own section (for --gc-sections)
    -fdata-sections             # Each data item in its own section
    -fno-exceptions             # No C++ exceptions (saves space)
    -fno-rtti                   # No RTTI (saves space)
    -fno-threadsafe-statics     # No thread-safe static init (single-threaded)
)

# ------------------------------------------------------------------------------
# Linker Flags
# ------------------------------------------------------------------------------

# Linker script (should be provided by board configuration or user)
# Expected location: boards/<board>/linker.ld or similar
# User should specify: -DLINKER_SCRIPT=path/to/linker.ld

if(DEFINED LINKER_SCRIPT)
    if(EXISTS "${LINKER_SCRIPT}")
        add_link_options(-T${LINKER_SCRIPT})
        message(STATUS "  Linker script: ${LINKER_SCRIPT}")
    else()
        message(WARNING "Linker script not found: ${LINKER_SCRIPT}")
    endif()
else()
    message(STATUS "  Linker script: Not specified (use -DLINKER_SCRIPT=...)")
endif()

# Linker optimization flags
add_link_options(
    -Wl,--gc-sections           # Remove unused sections
    -Wl,--print-memory-usage    # Print memory usage summary
    --specs=nano.specs          # Use newlib-nano (smaller libc)
    --specs=nosys.specs         # No syscalls (bare-metal)
)

# Optional: Generate map file
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_link_options(-Wl,-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map)
    message(STATUS "  Map file: ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map")
endif()

# ------------------------------------------------------------------------------
# Platform-Specific Libraries
# ------------------------------------------------------------------------------

# Link with math library (required for some floating-point operations)
link_libraries(m)

# Optional: Link with ARM CMSIS DSP library
# link_libraries(arm_cortexM7l_math)

# ------------------------------------------------------------------------------
# Platform Source Requirements
# ------------------------------------------------------------------------------

# SAME70 platform requires these source files (verified at build time):
#   - src/hal/platform/same70/uart.cpp
#   - src/hal/platform/same70/gpio.cpp
#   - src/hal/platform/same70/i2c.cpp
#   - src/hal/platform/same70/spi.cpp
#
# These will be automatically collected by platform_selection.cmake

# ------------------------------------------------------------------------------
# Board Configuration (Optional)
# ------------------------------------------------------------------------------

# If a specific board is selected (e.g., SAME70-XPLAINED), include board config
if(DEFINED ALLOY_BOARD)
    set(BOARD_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/boards/${ALLOY_BOARD}.cmake")
    if(EXISTS "${BOARD_CONFIG_FILE}")
        include("${BOARD_CONFIG_FILE}")
        message(STATUS "  Board: ${ALLOY_BOARD}")
    else()
        message(WARNING "Board configuration file not found: ${BOARD_CONFIG_FILE}")
    endif()
endif()

# ------------------------------------------------------------------------------
# Toolchain Verification
# ------------------------------------------------------------------------------

# Verify compiler is arm-none-eabi-gcc (or compatible)
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    execute_process(
        COMMAND ${CMAKE_C_COMPILER} --version
        OUTPUT_VARIABLE GCC_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GCC_VERSION_OUTPUT MATCHES "arm-none-eabi")
        message(STATUS "  Toolchain: ARM bare-metal GCC (arm-none-eabi-gcc)")
    else()
        message(WARNING
            "Compiler does not appear to be arm-none-eabi-gcc:\n"
            "${GCC_VERSION_OUTPUT}"
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# Platform Summary
# ------------------------------------------------------------------------------

message(STATUS "------------------------------------------------------------------------------")
message(STATUS "SAME70 Platform Configuration Complete")
message(STATUS "------------------------------------------------------------------------------")
