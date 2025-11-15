# ==============================================================================
# Linux HAL Unit Tests
# ==============================================================================
#
# Unit tests for Linux platform HAL implementation.
# Tests run on host without requiring hardware.
#
# Build:
#   cmake -DALLOY_PLATFORM=linux -DALLOY_BUILD_TESTS=ON -B build-test
#   make -C build-test
#   ctest --test-dir build-test
#
# ==============================================================================

# UART tests
add_executable(linux_uart_test
    uart_test.cpp
)

target_link_libraries(linux_uart_test
    PRIVATE
        Catch2::Catch2WithMain
        alloy-hal
)

target_include_directories(linux_uart_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Register test with CTest
include(CTest)
include(Catch)
catch_discover_tests(linux_uart_test)

# Set test properties
set_target_properties(linux_uart_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Print test info
message(STATUS "Linux HAL Unit Tests:")
message(STATUS "  - linux_uart_test")
