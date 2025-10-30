# Alloy Unit Tests
# Tests for core functionality, HAL implementations, and utilities

# Test executable
add_executable(alloy_unit_tests
    test_error_handling.cpp
    test_gpio_host.cpp
    test_uart_host.cpp
)

target_include_directories(alloy_unit_tests
    PRIVATE
        ${ALLOY_ROOT}/src
)

target_link_libraries(alloy_unit_tests
    PRIVATE
        GTest::gtest_main
        alloy_hal_host
)

target_compile_features(alloy_unit_tests
    PRIVATE
        cxx_std_20
)

# Apply Alloy compiler options
alloy_target_compile_options(alloy_unit_tests)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(alloy_unit_tests)
