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

# Bringup scenarios disabled — they used the old connector API which has been
# removed in favour of the lite HAL.  They will be rewritten against lite.
#
# alloy_add_host_mmio_contract_test(test_host_mmio_same70 ...)
# alloy_add_host_mmio_contract_test(test_host_mmio_stm32g0 ...)
# alloy_add_host_mmio_contract_test(test_host_mmio_stm32f4 ...)

message(STATUS "Host MMIO validation: bringup scenarios disabled (pending lite HAL port)")
