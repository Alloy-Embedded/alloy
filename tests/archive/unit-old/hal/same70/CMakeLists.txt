# ==============================================================================
# SAME70 HAL Unit Tests
# ==============================================================================
#
# Unit tests for SAME70 platform HAL implementation.
# Uses mock registers to test without hardware.
#
# Build:
#   cmake -DALLOY_BOARD=host -DALLOY_BUILD_TESTS=ON -B build-test
#   make -C build-test
#   ctest --test-dir build-test
#
# ==============================================================================

# UART tests
add_executable(uart_test
    uart_test.cpp
    uart_mock.hpp
)

target_link_libraries(uart_test
    PRIVATE
        Catch2::Catch2WithMain
        alloy-hal
)

target_include_directories(uart_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Register test with CTest
include(CTest)
include(Catch)
catch_discover_tests(uart_test)

# Set test properties
set_target_properties(uart_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# GPIO tests
add_executable(gpio_test
    gpio_test.cpp
    gpio_mock.hpp
)

target_link_libraries(gpio_test
    PRIVATE
        Catch2::Catch2WithMain
        alloy-hal
)

target_include_directories(gpio_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Register test with CTest
catch_discover_tests(gpio_test)

# Set test properties
set_target_properties(gpio_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Print test info
message(STATUS "SAME70 HAL Unit Tests:")
message(STATUS "  - uart_test")
message(STATUS "  - gpio_test")
