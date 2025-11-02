# Unit Tests for CoreZero Framework
cmake_minimum_required(VERSION 3.16)

project(corezero_unit_tests CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Test for Result<T> type
add_executable(test_result test_result.cpp)
add_test(NAME ResultTypeTest COMMAND test_result)

# Test for Error handling (if we create it)
# add_executable(test_error test_error.cpp)
# add_test(NAME ErrorHandlingTest COMMAND test_error)

# Add more tests here as needed

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_result
    COMMENT "Running all unit tests..."
)
