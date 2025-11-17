# Alloy RTOS Tests

cmake_minimum_required(VERSION 3.20)

# Test executable
add_executable(rtos_tests
    test_main.cpp
    test_queue.cpp
    test_notification.cpp
    test_pool.cpp
)

# Link with Alloy HAL
target_link_libraries(rtos_tests PRIVATE alloy-hal)

# Include test framework
target_include_directories(rtos_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# C++23 required
target_compile_features(rtos_tests PRIVATE cxx_std_23)

# Enable warnings
target_compile_options(rtos_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Define board name for tests
target_compile_definitions(rtos_tests PRIVATE
    BOARD_NAME="${BOARD_NAME}"
)
