cmake_minimum_required(VERSION 3.15)

# Example: RTOS Host Demo
project(rtos_host_demo LANGUAGES CXX)

# Must be built for host platform
if(NOT "${ALLOY_BOARD}" STREQUAL "host")
    message(FATAL_ERROR "This example requires ALLOY_BOARD=host")
endif()

# Source files
set(SOURCES
    main.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
    ${CMAKE_SOURCE_DIR}/src/hal/host/gpio.cpp
)

# Create executable
add_executable(rtos_host_demo ${SOURCES})

# Include Alloy headers
target_include_directories(rtos_host_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Link threading library (required for std::thread)
find_package(Threads REQUIRED)
target_link_libraries(rtos_host_demo PRIVATE
    Threads::Threads
)

# C++20 required
target_compile_features(rtos_host_demo PRIVATE cxx_std_20)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(rtos_host_demo PRIVATE
        -Wall -Wextra -Wpedantic
    )
endif()

message(STATUS "Configured example: rtos_host_demo (host platform)")
