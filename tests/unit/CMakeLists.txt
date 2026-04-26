# ==============================================================================
# Alloy HAL Unit Tests - Catch2 Framework
# ==============================================================================
#
# Modern C++20 unit tests using Catch2 v3
#
# Test Organization:
#   - core/      : Core utilities (Result<T,E>, error handling)
#   - hal/       : HAL interface tests (concepts, APIs)
#   - platform/  : Platform-specific tests
#
# Running Tests:
#   cmake -B build-tests -DALLOY_BOARD=host -DALLOY_BUILD_TESTS=ON
#   cmake --build build-tests
#   cd build-tests && ctest --output-on-failure
#
# ==============================================================================

cmake_minimum_required(VERSION 3.20)

include(${CMAKE_SOURCE_DIR}/tests/alloy_test_helpers.cmake)

# ==============================================================================
# Core Tests
# ==============================================================================

# Test: Result<T, E> Type
alloy_add_catch2_test(test_result
    SOURCES test_result.cpp
    LABELS unit
)

# Test: Error Handling
alloy_add_catch2_test(test_error
    SOURCES test_error.cpp
    LABELS unit
)

# ==============================================================================
# HAL Concept Tests (C++20)
# ==============================================================================

# Test: GpioPin Concept
alloy_add_catch2_test(test_gpio_concept
    SOURCES test_gpio_concept.cpp
    LABELS unit
)

# Test: ClockPlatform Concept
alloy_add_catch2_test(test_clock_concept
    SOURCES test_clock_concept.cpp
    LABELS unit
)

# Test: Logger Core and Sink Integration
alloy_add_catch2_test(test_logger
    SOURCES test_logger.cpp
    LABELS unit
)

# Test: Cooperative coroutine scheduler (runtime-tasks)
# scheduler.cpp is included directly because alloy-hal is INTERFACE on host
# builds and would not otherwise compile it.
alloy_add_catch2_test(test_tasks
    SOURCES
        test_tasks.cpp
        ${CMAKE_SOURCE_DIR}/src/runtime/tasks/scheduler.cpp
    LABELS unit tasks
)

# Test: Runtime async-model canonical completion+timeout primitive
alloy_add_catch2_test(test_async_completion_timeout
    SOURCES test_async_completion_timeout.cpp
    LABELS unit
)

# Test: Channel<T,N> memory ordering (two threads, simulates cross-core)
alloy_add_catch2_test(test_memory_ordering
    SOURCES test_memory_ordering.cpp
    LABELS unit smp
)

# Test: CrossCoreChannel<T,N> — TSan-compatible acquire/release SPSC ring
alloy_add_catch2_test(test_cross_core_channel
    SOURCES test_cross_core_channel.cpp
    LABELS unit smp
)

# Test: SharedScheduler + SharedEvent — dual-core cooperative scheduling
alloy_add_catch2_test(test_shared_scheduler
    SOURCES
        test_shared_scheduler.cpp
        ${CMAKE_SOURCE_DIR}/src/runtime/tasks/scheduler.cpp
    LABELS unit smp
)

# ==============================================================================
# Test Summary
# ==============================================================================

message(STATUS "========================================")
message(STATUS "Unit Tests Configured:")
message(STATUS "  ✓ Core: Result<T,E>, Error")
message(STATUS "  ✓ HAL:  GPIO, Clock Concepts")
message(STATUS "  ✓ Utility: Logger core")
message(STATUS "  ✓ SMP:  Channel ordering, CrossCoreChannel")
message(STATUS "========================================")
