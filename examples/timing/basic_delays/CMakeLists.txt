cmake_minimum_required(VERSION 3.20)

# =============================================================================
# Basic Delays Timing Example
# =============================================================================
#
# Demonstrates accurate millisecond and microsecond delays using SysTick.
#
# Supported boards:
# - nucleo-f401re (STM32F401RE @ 84 MHz)
# - nucleo-f722ze (STM32F722ZE @ 180 MHz)
# - nucleo-g071rb (STM32G071RB @ 64 MHz)
# - nucleo-g0b1re (STM32G0B1RE @ 64 MHz)
# - same70-xplained (ATSAME70Q21B @ 300 MHz)
#
# Build:
#   make <board>-basic-delays-build
#   make <board>-basic-delays-flash
#
# Example:
#   make nucleo-f401re-basic-delays-build
#   make nucleo-f401re-basic-delays-flash
#
# =============================================================================

# Get board name from parent scope
if(NOT DEFINED ALLOY_BOARD)
    message(FATAL_ERROR "ALLOY_BOARD must be set. Use: cmake -DALLOY_BOARD=nucleo_f401re ..")
endif()

# Project definition
project(basic_delays
    VERSION 1.0.0
    DESCRIPTION "Basic timing delays example using SysTick"
    LANGUAGES CXX C ASM
)

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
if(DEFINED ALLOY_STARTUP_CODE AND EXISTS ${ALLOY_STARTUP_CODE})
    list(APPEND EXAMPLE_SOURCES ${ALLOY_STARTUP_CODE})
endif()

# =============================================================================
# Executable Target
# =============================================================================

add_executable(${PROJECT_NAME}
    ${EXAMPLE_SOURCES}
)

# =============================================================================
# Include Directories
# =============================================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src           # HAL API and platform headers
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
# Link alloy-hal (platform-specific HAL library)
# =============================================================================

if(TARGET alloy-hal)
    target_link_libraries(${PROJECT_NAME} PRIVATE alloy-hal)
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
message(STATUS "==============================================")
