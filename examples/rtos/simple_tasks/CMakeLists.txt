cmake_minimum_required(VERSION 3.20)

# =============================================================================
# Simple RTOS Tasks Example
# =============================================================================
#
# Demonstrates basic RTOS functionality with multiple tasks.
#
# Supported boards:
# - nucleo-f401re (STM32F401RE @ 84 MHz)
# - nucleo-f722ze (STM32F722ZE @ 180 MHz)
# - nucleo-g071rb (STM32G071RB @ 64 MHz)
# - nucleo-g0b1re (STM32G0B1RE @ 64 MHz)
# - same70-xplained (ATSAME70Q21B @ 12 MHz)
#
# Build:
#   make <board>-rtos-simple-tasks-build
#   make <board>-rtos-simple-tasks-flash
#
# Example:
#   make nucleo-f401re-rtos-simple-tasks-build
#   make nucleo-f401re-rtos-simple-tasks-flash
#
# =============================================================================

# Get board name from parent scope
if(NOT DEFINED ALLOY_BOARD)
    message(FATAL_ERROR "ALLOY_BOARD must be set. Use: cmake -DALLOY_BOARD=nucleo_f401re ..")
endif()

# Project definition
project(rtos_simple_tasks
    VERSION 1.0.0
    DESCRIPTION "Simple RTOS multi-task example"
    LANGUAGES CXX C ASM
)

# Require C++20 for concepts
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Source Files
# =============================================================================

set(EXAMPLE_SOURCES
    main.cpp
)

# =============================================================================
# Board Support Files
# =============================================================================

# Board-specific source files (board.cpp, syscalls.cpp, startup code)
set(BOARD_DIR ${CMAKE_SOURCE_DIR}/boards/${ALLOY_BOARD})

if(EXISTS ${BOARD_DIR}/board.cpp)
    list(APPEND EXAMPLE_SOURCES ${BOARD_DIR}/board.cpp)
endif()

if(EXISTS ${BOARD_DIR}/syscalls.cpp)
    list(APPEND EXAMPLE_SOURCES ${BOARD_DIR}/syscalls.cpp)
endif()

# Platform-specific startup code (set by platform config)
if(CMAKE_CROSSCOMPILING AND DEFINED STARTUP_SOURCE)
    list(APPEND EXAMPLE_SOURCES ${STARTUP_SOURCE})
endif()

# =============================================================================
# RTOS Source Files
# =============================================================================

# Add RTOS implementation files
set(RTOS_DIR ${CMAKE_SOURCE_DIR}/src/rtos)

list(APPEND EXAMPLE_SOURCES
    ${RTOS_DIR}/scheduler.cpp
)

# Platform-specific RTOS sources
# For ARM Cortex-M platforms
if(DEFINED __ARM_ARCH_6M__ OR ALLOY_PLATFORM MATCHES "stm32g0")
    # Cortex-M0/M0+ - use arm_context_m0.cpp
    if(EXISTS ${RTOS_DIR}/platform/arm_context_m0.cpp)
        list(APPEND EXAMPLE_SOURCES ${RTOS_DIR}/platform/arm_context_m0.cpp)
    endif()
else()
    # Cortex-M3/M4/M7 - use arm_context.cpp
    if(EXISTS ${RTOS_DIR}/platform/arm_context.cpp)
        list(APPEND EXAMPLE_SOURCES ${RTOS_DIR}/platform/arm_context.cpp)
    endif()
endif()

# =============================================================================
# Executable Target
# =============================================================================

add_executable(${PROJECT_NAME}
    ${EXAMPLE_SOURCES}
)

# =============================================================================
# Compile Definitions
# =============================================================================

# Enable RTOS in board SysTick handlers
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ALLOY_RTOS_ENABLED=1
)

# =============================================================================
# Include Directories
# =============================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src           # HAL API, RTOS, and platform headers
    ${CMAKE_SOURCE_DIR}/boards        # Board support packages
    ${BOARD_DIR}                      # Board-specific headers
)

# =============================================================================
# Compiler Options
# =============================================================================

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-O2>
)

# =============================================================================
# Embedded-specific linker options
# =============================================================================

if(CMAKE_CROSSCOMPILING)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -ffunction-sections
        -fdata-sections
        -fno-exceptions
        -fno-rtti
        -fno-threadsafe-statics
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
        --specs=nosys.specs
    )

    # Add linker script if specified
    if(DEFINED LINKER_SCRIPT AND EXISTS "${LINKER_SCRIPT}")
        target_link_options(${PROJECT_NAME} PRIVATE
            -T${LINKER_SCRIPT}
        )
    endif()
endif()

# =============================================================================
# Link alloy-hal (platform-specific HAL library)
# =============================================================================

if(DEFINED ALLOY_HAL_LIBRARY)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${ALLOY_HAL_LIBRARY}
    )
endif()

# =============================================================================
# Post-Build: Generate Binary Files
# =============================================================================

# Generate .hex and .bin files for flashing
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)

# =============================================================================
# Build Info
# =============================================================================

message(STATUS "==============================================")
message(STATUS "Example: ${PROJECT_NAME}")
message(STATUS "Board: ${ALLOY_BOARD}")
message(STATUS "Platform: ${ALLOY_PLATFORM}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "RTOS: ENABLED")
message(STATUS "==============================================")
