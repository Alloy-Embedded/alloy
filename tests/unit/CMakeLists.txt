# Unit Tests for Alloy Framework
cmake_minimum_required(VERSION 3.16)

project(alloy_unit_tests CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# Test for Result<T> type
add_executable(test_result test_result.cpp)
add_test(NAME ResultTypeTest COMMAND test_result)

# Test for core types
add_executable(test_types test_types.cpp)
add_test(NAME CoreTypesTest COMMAND test_types)

# Test for memory utilities
add_executable(test_memory test_memory.cpp)
add_test(NAME MemoryUtilsTest COMMAND test_memory)

# Test for ErrorCode enum
add_executable(test_error_code test_error_code.cpp)
add_test(NAME ErrorCodeTest COMMAND test_error_code)

# Test for C++20 concepts
add_executable(test_concepts test_concepts.cpp)
add_test(NAME ConceptsTest COMMAND test_concepts)

# Test for HAL peripheral concepts (modernize-peripheral-architecture Phase 1)
add_executable(test_hal_concepts test_hal_concepts.cpp)
add_test(NAME HalConceptsTest COMMAND test_hal_concepts)

# Test for signal routing tables (modernize-peripheral-architecture Phase 2)
add_executable(test_signal_tables test_signal_tables.cpp)
add_test(NAME SignalTablesTest COMMAND test_signal_tables)

# Test for GPIO signal routing (modernize-peripheral-architecture Phase 3.1)
add_executable(test_gpio_signal_routing test_gpio_signal_routing.cpp)
add_test(NAME GpioSignalRoutingTest COMMAND test_gpio_signal_routing)

# Test for pin-signal connection API (modernize-peripheral-architecture Phase 3.2)
add_executable(test_pin_signal_connection test_pin_signal_connection.cpp)
add_test(NAME PinSignalConnectionTest COMMAND test_pin_signal_connection)

# Test for signal routing registry (modernize-peripheral-architecture Phase 3.3)
add_executable(test_signal_registry test_signal_registry.cpp)
add_test(NAME SignalRegistryTest COMMAND test_signal_registry)

# Test for UART simple API (modernize-peripheral-architecture Phase 4.1)
add_executable(test_uart_simple_api test_uart_simple_api.cpp)
add_test(NAME UartSimpleApiTest COMMAND test_uart_simple_api)

# Test for UART fluent API (modernize-peripheral-architecture Phase 4.2)
add_executable(test_uart_fluent_api test_uart_fluent_api.cpp)
add_test(NAME UartFluentApiTest COMMAND test_uart_fluent_api)

# Test for UART expert API (modernize-peripheral-architecture Phase 4.3)
add_executable(test_uart_expert_api test_uart_expert_api.cpp)
add_test(NAME UartExpertApiTest COMMAND test_uart_expert_api)

# Test for units (BaudRate)
add_executable(test_units test_units.cpp)
add_test(NAME UnitsTest COMMAND test_units)

# Test for RAII scoped device wrappers
add_executable(test_scoped_device test_scoped_device.cpp)
add_test(NAME ScopedDeviceTest COMMAND test_scoped_device)

# Test for circular buffer
add_executable(test_circular_buffer test_circular_buffer.cpp)
add_test(NAME CircularBufferTest COMMAND test_circular_buffer)

# ============================================================================
# Code Generation Tests - Test generated register files
# ============================================================================

# Test SAME70 generated code
add_executable(test_same70_codegen codegen/test_same70_codegen.cpp)
add_test(NAME SAME70CodegenTest COMMAND test_same70_codegen)

# Test STM32F1 generated code
add_executable(test_stm32f1_codegen codegen/test_stm32f1_codegen.cpp)
add_test(NAME STM32F1CodegenTest COMMAND test_stm32f1_codegen)

# Test STM32F4 generated code
add_executable(test_stm32f4_codegen codegen/test_stm32f4_codegen.cpp)
add_test(NAME STM32F4CodegenTest COMMAND test_stm32f4_codegen)

# Test for Error handling (if we create it)
# add_executable(test_error test_error.cpp)
# add_test(NAME ErrorHandlingTest COMMAND test_error)

# Add more tests here as needed

# HAL unit tests (platform-specific)
# Temporarily disabled - need to generate SAME70 register files first
# if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/hal/same70/CMakeLists.txt)
#     add_subdirectory(hal/same70)
# endif()

# Custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_result test_types test_memory test_error_code test_concepts test_hal_concepts test_signal_tables test_gpio_signal_routing test_pin_signal_connection test_signal_registry test_uart_simple_api test_uart_fluent_api test_uart_expert_api test_units test_scoped_device test_circular_buffer test_same70_codegen test_stm32f1_codegen test_stm32f4_codegen
    COMMENT "Running all unit tests..."
)
