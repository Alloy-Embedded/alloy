# SPI Multi-Level API Demo Example
cmake_minimum_required(VERSION 3.16)

project(spi_multi_level_api_demo CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# Define platform (can be overridden for specific targets)
if(NOT DEFINED ALLOY_PLATFORM)
    set(ALLOY_PLATFORM "LINUX")
endif()

add_compile_definitions(ALLOY_PLATFORM_${ALLOY_PLATFORM})

# Create executable
add_executable(spi_multi_level_api_demo
    ../spi_multi_level_api_demo.cpp
)

# Compiler flags
target_compile_options(spi_multi_level_api_demo PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
)

# If building for embedded target (SAME70)
if(ALLOY_PLATFORM STREQUAL "SAME70")
    target_compile_options(spi_multi_level_api_demo PRIVATE
        -mcpu=cortex-m7
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv5-d16
        -ffunction-sections
        -fdata-sections
    )

    target_link_options(spi_multi_level_api_demo PRIVATE
        -mcpu=cortex-m7
        -mthumb
        -mfloat-abi=hard
        -mfpu=fpv5-d16
        -Wl,--gc-sections
        -specs=nano.specs
        -specs=nosys.specs
    )
endif()
