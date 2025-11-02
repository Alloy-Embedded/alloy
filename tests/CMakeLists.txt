# Alloy RTOS Tests
cmake_minimum_required(VERSION 3.20)

# Google Test is already fetched by parent CMakeLists.txt
# Just link against it

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# ============================================================================
# Unit Tests
# ============================================================================

# Test: Task Management
add_executable(test_task
    unit/rtos/test_task.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_task gtest gtest_main pthread)
target_compile_options(test_task PRIVATE -std=c++20)
add_test(NAME test_task COMMAND test_task)

# Test: Queue
add_executable(test_queue
    unit/rtos/test_queue.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_queue gtest gtest_main pthread)
target_compile_options(test_queue PRIVATE -std=c++20)
add_test(NAME test_queue COMMAND test_queue)

# Test: Mutex
add_executable(test_mutex
    unit/rtos/test_mutex.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_mutex gtest gtest_main pthread)
target_compile_options(test_mutex PRIVATE -std=c++20)
add_test(NAME test_mutex COMMAND test_mutex)

# Test: Semaphore
add_executable(test_semaphore
    unit/rtos/test_semaphore.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_semaphore gtest gtest_main pthread)
target_compile_options(test_semaphore PRIVATE -std=c++20)
add_test(NAME test_semaphore COMMAND test_semaphore)

# Test: Event Flags
add_executable(test_event
    unit/rtos/test_event.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_event gtest gtest_main pthread)
target_compile_options(test_event PRIVATE -std=c++20)
add_test(NAME test_event COMMAND test_event)

# Test: Scheduler
add_executable(test_scheduler
    unit/rtos/test_scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_scheduler gtest gtest_main pthread)
target_compile_options(test_scheduler PRIVATE -std=c++20)
add_test(NAME test_scheduler COMMAND test_scheduler)

# ============================================================================
# Integration Tests
# ============================================================================

add_executable(rtos_integration_test
    integration/rtos_integration_test.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(rtos_integration_test gtest gtest_main pthread)
target_compile_options(rtos_integration_test PRIVATE -std=c++20)
add_test(NAME rtos_integration_test COMMAND rtos_integration_test)

# ============================================================================
# Test Suite Summary
# ============================================================================

# Enable testing
enable_testing()

# Custom target to run all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS
        test_task
        test_queue
        test_mutex
        test_semaphore
        test_event
        test_scheduler
        rtos_integration_test
    COMMENT "Running all RTOS tests..."
)

# Display test summary
message(STATUS "========================================")
message(STATUS "RTOS Test Suite Configuration")
message(STATUS "========================================")
message(STATUS "Unit Tests:")
message(STATUS "  - Task Management Tests")
message(STATUS "  - Queue Tests")
message(STATUS "  - Mutex Tests")
message(STATUS "  - Semaphore Tests")
message(STATUS "  - Event Flags Tests")
message(STATUS "  - Scheduler Tests")
message(STATUS "")
message(STATUS "Integration Tests:")
message(STATUS "  - RTOS Integration Test Suite")
message(STATUS "")
message(STATUS "To run all tests: make run_all_tests")
message(STATUS "To run individual test: make test_<name>")
message(STATUS "========================================")
