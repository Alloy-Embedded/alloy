include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/renode_test_helpers.cmake)

function(alloy_add_stm32_renode_boot_smoke TARGET_NAME)
    set(options)
    set(one_value_args
        BOARD_NAME
        BOARD_DIR
        FIRMWARE_SOURCE
        LINKER_SCRIPT
        CPU_REPL
        SYSTICK_FREQUENCY
        ROBOT_SCRIPT
        TEST_NAME
        OUTPUT_NAME
        FAMILY_LABEL
    )
    set(multi_value_args
        EXTRA_SOURCES
        TEST_LABELS
        SYMBOL_VARS
        FILE_VARS
    )
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    foreach(required_arg IN ITEMS
            BOARD_NAME
            BOARD_DIR
            FIRMWARE_SOURCE
            LINKER_SCRIPT
            CPU_REPL
            SYSTICK_FREQUENCY
            ROBOT_SCRIPT
            TEST_NAME)
        if(NOT ARG_${required_arg})
            message(FATAL_ERROR
                "alloy_add_stm32_renode_boot_smoke(${TARGET_NAME}) requires ${required_arg}")
        endif()
    endforeach()

    if(NOT ARG_OUTPUT_NAME)
        set(ARG_OUTPUT_NAME "${TARGET_NAME}")
    endif()

    if(NOT ALLOY_BOARD STREQUAL ARG_BOARD_NAME)
        if(ARG_FAMILY_LABEL)
            string(TOUPPER "${ARG_FAMILY_LABEL}" _st_family_label)
        else()
            set(_st_family_label "STM32")
        endif()
        message(STATUS
            "Renode ${_st_family_label} scenario skipped: select ALLOY_BOARD=${ARG_BOARD_NAME}.")
        return()
    endif()

    if(NOT CMAKE_CROSSCOMPILING)
        message(STATUS "Renode ${ARG_BOARD_NAME} scenario skipped: cross toolchain is required.")
        return()
    endif()

    set(ALLOY_RENODE_ST_CPU_REPL "${ARG_CPU_REPL}")
    set(ALLOY_RENODE_ST_SYSTICK_FREQUENCY "${ARG_SYSTICK_FREQUENCY}")
    set(_st_platform_output "${CMAKE_CURRENT_BINARY_DIR}/${ARG_BOARD_NAME}_boot_smoke.repl")
    set(_st_labels emulation renode stm32)
    if(ARG_FAMILY_LABEL)
        list(APPEND _st_labels "${ARG_FAMILY_LABEL}")
    endif()
    list(APPEND _st_labels ${ARG_TEST_LABELS})

    alloy_add_renode_boot_smoke(${TARGET_NAME}
        OUTPUT_NAME
            ${ARG_OUTPUT_NAME}
        BOARD_DIR
            ${ARG_BOARD_DIR}
        FIRMWARE_SOURCE
            ${ARG_FIRMWARE_SOURCE}
        EXTRA_SOURCES
            ${ARG_EXTRA_SOURCES}
        LINKER_SCRIPT
            ${ARG_LINKER_SCRIPT}
        PLATFORM_TEMPLATE
            ${CMAKE_SOURCE_DIR}/tests/emulation/renode/common/platforms/stm32_boot_smoke.repl.in
        PLATFORM_OUTPUT
            ${_st_platform_output}
        ROBOT_SCRIPT
            ${ARG_ROBOT_SCRIPT}
        TEST_NAME
            ${ARG_TEST_NAME}
        TEST_LABELS
            ${_st_labels}
        SYMBOL_VARS
            ${ARG_SYMBOL_VARS}
        FILE_VARS
            COMMON_BOOT_RESOURCE=${CMAKE_SOURCE_DIR}/tests/emulation/renode/common/robot/boot_assertions.resource
            COMMON_STM32_BOOT_RESOURCE=${CMAKE_SOURCE_DIR}/tests/emulation/renode/common/robot/stm32_boot_assertions.resource
            ${ARG_FILE_VARS}
    )
endfunction()
