# Alloy RTOS Tests
cmake_minimum_required(VERSION 3.20)

# Catch2 is already fetched by parent CMakeLists.txt
# Include Catch2 test discovery
include(CTest)
include(Catch)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# ============================================================================
# Unit Tests - Catch2 Framework
# ============================================================================

# Test: Task Management (Compile-time tests only)
add_executable(test_task
    unit/rtos/test_task.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_task PRIVATE Catch2::Catch2WithMain pthread)
target_compile_options(test_task PRIVATE -std=c++20)
catch_discover_tests(test_task)

# Test: Mutex (Full tests using std::thread)
add_executable(test_mutex
    unit/rtos/test_mutex.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_mutex PRIVATE Catch2::Catch2WithMain pthread)
target_compile_options(test_mutex PRIVATE -std=c++20)
catch_discover_tests(test_mutex)

# Test: Semaphore (Full tests using std::thread)
add_executable(test_semaphore
    unit/rtos/test_semaphore.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_semaphore PRIVATE Catch2::Catch2WithMain pthread)
target_compile_options(test_semaphore PRIVATE -std=c++20)
catch_discover_tests(test_semaphore)

# Test: Event Flags (Full tests using std::thread)
add_executable(test_event
    unit/rtos/test_event.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/scheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/host_context.cpp
    ${CMAKE_SOURCE_DIR}/src/rtos/platform/critical_section.cpp
)
target_link_libraries(test_event PRIVATE Catch2::Catch2WithMain pthread)
target_compile_options(test_event PRIVATE -std=c++20)
catch_discover_tests(test_event)

# ============================================================================
# Unit Tests (non-RTOS)
# ============================================================================

# Include unit test subdirectory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/unit/CMakeLists.txt)
    add_subdirectory(unit)
endif()

# ============================================================================
# Test Suite Summary
# ============================================================================

# Custom target to run all tests with Catch2's beautiful output
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS
        test_task
        test_mutex
        test_semaphore
        test_event
    COMMENT "Running all RTOS tests with Catch2..."
)

# Display test summary
message(STATUS "========================================")
message(STATUS "RTOS Test Suite - Catch2 Framework")
message(STATUS "========================================")
message(STATUS "Unit Tests:")
message(STATUS "  ✓ Task Management (compile-time)")
message(STATUS "  ✓ Mutex (runtime with std::thread)")
message(STATUS "  ✓ Semaphore (runtime with std::thread)")
message(STATUS "  ✓ Event Flags (runtime with std::thread)")
message(STATUS "")
message(STATUS "Commands:")
message(STATUS "  Build all:     make")
message(STATUS "  Run all:       ctest --output-on-failure")
message(STATUS "  Run one:       ./tests/test_mutex")
message(STATUS "  Compact:       ./tests/test_mutex -r compact")
message(STATUS "  List tags:     ./tests/test_mutex --list-tags")
message(STATUS "  Run by tag:    ./tests/test_mutex [mutex][basic]")
message(STATUS "========================================")
