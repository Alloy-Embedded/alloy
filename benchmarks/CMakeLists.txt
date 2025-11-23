# MicroCore Benchmarks
#
# Performance benchmarks for MicroCore HAL components.
# These benchmarks measure real-world performance metrics and compare
# against Arduino, mbed, and other embedded frameworks.

cmake_minimum_required(VERSION 3.25)

# Only build benchmarks for embedded targets
if(MICROCORE_PLATFORM STREQUAL "linux" OR MICROCORE_PLATFORM STREQUAL "host")
    message(STATUS "Skipping benchmarks for host platform")
    return()
endif()

# ============================================================================
# GPIO Toggle Benchmark
# ============================================================================

add_executable(gpio_toggle_bench
    gpio_toggle_bench.cpp
)

target_include_directories(gpio_toggle_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MICROCORE_ROOT}/src
)

target_link_libraries(gpio_toggle_bench PRIVATE
    microcore_hal
)

target_compile_definitions(gpio_toggle_bench PRIVATE
    PLATFORM_${MICROCORE_PLATFORM}
)

target_compile_options(gpio_toggle_bench PRIVATE
    -O3                    # Maximum optimization for benchmarks
    -flto                  # Link-time optimization
    -fno-exceptions        # No exceptions overhead
    -fno-rtti              # No RTTI overhead
)

# ============================================================================
# Interrupt Latency Benchmark
# ============================================================================

add_executable(interrupt_latency_bench
    interrupt_latency_bench.cpp
)

target_include_directories(interrupt_latency_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MICROCORE_ROOT}/src
)

target_link_libraries(interrupt_latency_bench PRIVATE
    microcore_hal
)

target_compile_definitions(interrupt_latency_bench PRIVATE
    PLATFORM_${MICROCORE_PLATFORM}
)

target_compile_options(interrupt_latency_bench PRIVATE
    -O3
    -flto
    -fno-exceptions
    -fno-rtti
)

# ============================================================================
# Benchmark Results Target
# ============================================================================

# Custom target to build all benchmarks
add_custom_target(benchmarks
    DEPENDS
        gpio_toggle_bench
        interrupt_latency_bench
)

# Custom target to flash and run benchmarks
add_custom_target(run-benchmarks
    COMMAND ${CMAKE_COMMAND} -E echo "Flash and run benchmarks manually"
    COMMAND ${CMAKE_COMMAND} -E echo "  1. Flash gpio_toggle_bench"
    COMMAND ${CMAKE_COMMAND} -E echo "  2. Connect oscilloscope to measure frequency"
    COMMAND ${CMAKE_COMMAND} -E echo "  3. Flash interrupt_latency_bench"
    COMMAND ${CMAKE_COMMAND} -E echo "  4. Measure interrupt response time"
    DEPENDS benchmarks
)

# ============================================================================
# Benchmark Documentation
# ============================================================================

# Print benchmark targets
message(STATUS "")
message(STATUS "Benchmark targets:")
message(STATUS "  gpio_toggle_bench          - GPIO toggle frequency")
message(STATUS "  interrupt_latency_bench    - Interrupt response time")
message(STATUS "  benchmarks                 - Build all benchmarks")
message(STATUS "  run-benchmarks             - Instructions for running")
message(STATUS "")
