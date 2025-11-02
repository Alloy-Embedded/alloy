cmake_minimum_required(VERSION 3.15)

# Example: SysTick Host Test
project(systick_host_test LANGUAGES CXX)

# Must be built for host platform
if(NOT "${ALLOY_BOARD}" STREQUAL "host")
    message(FATAL_ERROR "This example requires ALLOY_BOARD=host")
endif()

# Source files
set(SOURCES
    main.cpp
    ${CMAKE_SOURCE_DIR}/src/hal/host/gpio.cpp
)

# Create executable
add_executable(systick_host_test ${SOURCES})

# Include Alloy headers
target_include_directories(systick_host_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link required libraries (minimal for host)
target_link_libraries(systick_host_test PRIVATE
    # No additional libraries needed for host
)

# C++20 required
target_compile_features(systick_host_test PRIVATE cxx_std_20)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(systick_host_test PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

message(STATUS "Configured example: systick_host_test (host platform)")
