# SAME70 UART Multi-Level API Example
cmake_minimum_required(VERSION 3.16)

project(same70_uart_multi_level CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# ARM toolchain setup (if cross-compiling)
if(CMAKE_CROSSCOMPILING)
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR arm)
endif()

# Define platform
add_compile_definitions(ALLOY_PLATFORM_SAME70)

# Create executable
add_executable(uart_multi_level
    main.cpp
)

# Link with required libraries (if any)
# target_link_libraries(uart_multi_level alloy_core alloy_hal)

# Compiler flags for ARM Cortex-M7
if(CMAKE_CROSSCOMPILING)
    target_compile_options(uart_multi_level PRIVATE
        -mcpu=cortex-m7
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv5-d16
        -ffunction-sections
        -fdata-sections
        -Wall
        -Wextra
        -O2
    )

    target_link_options(uart_multi_level PRIVATE
        -mcpu=cortex-m7
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv5-d16
        -Wl,--gc-sections
        -specs=nano.specs
        -specs=nosys.specs
    )
endif()
