include(${CMAKE_SOURCE_DIR}/tests/alloy_test_helpers.cmake)

function(alloy_add_host_mmio_contract_test TARGET_NAME SELECTED_CONFIG_TEMPLATE)
    set(options)
    set(one_value_args TEST_PREFIX)
    set(multi_value_args SOURCES LABELS)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "alloy_add_host_mmio_contract_test(${TARGET_NAME}) requires SOURCES")
    endif()

    set(ALLOY_HOST_MMIO_SELECTED_CONFIG_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/contracts/${TARGET_NAME}/generated/alloy/device"
    )
    file(MAKE_DIRECTORY "${ALLOY_HOST_MMIO_SELECTED_CONFIG_DIR}")
    configure_file(
        "${CMAKE_SOURCE_DIR}/tests/host_mmio/${SELECTED_CONFIG_TEMPLATE}"
        "${ALLOY_HOST_MMIO_SELECTED_CONFIG_DIR}/selected_config.hpp"
        @ONLY
    )

    alloy_add_catch2_test(${TARGET_NAME}
        SOURCES
            ${ARG_SOURCES}
        LABELS
            ${ARG_LABELS}
        INCLUDES
            "${CMAKE_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/contracts/${TARGET_NAME}/generated"
        COMPILE_DEFINITIONS
            ALLOY_ENABLE_HOST_MMIO_RUNTIME_HOOKS=1
        TEST_PREFIX
            ${ARG_TEST_PREFIX}
    )
endfunction()

alloy_add_host_mmio_contract_test(test_host_mmio_same70
    selected_config_same70.hpp.in
    TEST_PREFIX
        "host_mmio.same70::"
    SOURCES
        scenarios/test_mmio_trace.cpp
        scenarios/test_same70_bringup.cpp
    LABELS
        host-mmio
        bring-up
        same70
)

alloy_add_host_mmio_contract_test(test_host_mmio_stm32g0
    selected_config_stm32g071rb.hpp.in
    TEST_PREFIX
        "host_mmio.stm32g0::"
    SOURCES
        ${CMAKE_SOURCE_DIR}/src/runtime/interrupt_bridge.cpp
        scenarios/test_stm32g0_bringup.cpp
    LABELS
        host-mmio
        bring-up
        stm32g0
)

alloy_add_host_mmio_contract_test(test_host_mmio_stm32f4
    selected_config_stm32f401re.hpp.in
    TEST_PREFIX
        "host_mmio.stm32f4::"
    SOURCES
        scenarios/test_stm32f4_bringup.cpp
    LABELS
        host-mmio
        bring-up
        stm32f4
)

message(STATUS "Host MMIO validation configured:")
message(STATUS "  - MMIO trace framework smoke")
message(STATUS "  - SAME70-style watchdog/clock/reset/GPIO/UART bring-up scenarios")
message(STATUS "  - STM32G0-style clock/reset/GPIO/UART bring-up via nucleo_g071rb board connectors")
message(STATUS "  - STM32F4-style clock/reset/GPIO/UART bring-up via nucleo_f401re board connectors")
message(STATUS "  - Runtime contract overrides: SAME70, STM32G071RB, and STM32F401RE runtime on native host")
