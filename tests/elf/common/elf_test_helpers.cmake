include_guard(GLOBAL)

function(alloy_add_elf_startup_smoke TARGET_NAME)
    set(options)
    set(one_value_args
        OUTPUT_NAME
        BOARD_DIR
        FIRMWARE_SOURCE
        LINKER_SCRIPT
        TEST_NAME
        TEST_LABEL
    )
    set(multi_value_args EXTRA_SOURCES)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    foreach(required_arg IN ITEMS OUTPUT_NAME BOARD_DIR FIRMWARE_SOURCE LINKER_SCRIPT TEST_NAME TEST_LABEL)
        if(NOT ARG_${required_arg})
            message(FATAL_ERROR "alloy_add_elf_startup_smoke(${TARGET_NAME}) requires ${required_arg}")
        endif()
    endforeach()

    set(_elf_sources
        "${ARG_FIRMWARE_SOURCE}"
        "${ARG_BOARD_DIR}/board.cpp"
        ${ARG_EXTRA_SOURCES}
    )
    if(EXISTS "${ARG_BOARD_DIR}/syscalls.cpp")
        list(APPEND _elf_sources "${ARG_BOARD_DIR}/syscalls.cpp")
    endif()

    add_executable(${TARGET_NAME} ${_elf_sources})

    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/boards
    )
    target_link_libraries(${TARGET_NAME} PRIVATE
        ${ALLOY_HAL_LIBRARY}
    )
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)
    target_compile_options(${TARGET_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -ffunction-sections
        -fdata-sections
        -fno-exceptions
        -fno-rtti
        -fno-threadsafe-statics
    )
    target_link_options(${TARGET_NAME} PRIVATE
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
        --specs=nosys.specs
        -T${ARG_LINKER_SCRIPT}
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        OUTPUT_NAME "${ARG_OUTPUT_NAME}"
        FOLDER "tests/elf"
    )

    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(ALLOY_ELF_READELF "${CMAKE_READELF}")
    if(NOT ALLOY_ELF_READELF)
        get_filename_component(ALLOY_ELF_TOOL_DIR "${CMAKE_NM}" DIRECTORY)
        find_program(ALLOY_ELF_READELF NAMES arm-none-eabi-readelf readelf HINTS "${ALLOY_ELF_TOOL_DIR}" REQUIRED)
    endif()

    set(ALLOY_ELF_OBJDUMP "${CMAKE_OBJDUMP}")
    if(NOT ALLOY_ELF_OBJDUMP)
        get_filename_component(ALLOY_ELF_TOOL_DIR "${CMAKE_NM}" DIRECTORY)
        find_program(ALLOY_ELF_OBJDUMP NAMES arm-none-eabi-objdump objdump HINTS "${ALLOY_ELF_TOOL_DIR}" REQUIRED)
    endif()

    add_test(
        NAME ${ARG_TEST_NAME}
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/elf/scripts/inspect_startup_elf.py
            --elf $<TARGET_FILE:${TARGET_NAME}>
            --readelf ${ALLOY_ELF_READELF}
            --nm ${CMAKE_NM}
            --objdump ${ALLOY_ELF_OBJDUMP}
            --vector-section .isr_vector
            --entry-symbol Reset_Handler
            --required-section .isr_vector
            --required-section .text
            --required-section .data
            --required-section .bss
            --required-symbol Reset_Handler
            --required-symbol main
            --thumb-entry
    )
    set_tests_properties(${ARG_TEST_NAME} PROPERTIES
        LABELS "elf;startup;${ARG_TEST_LABEL}"
    )

    add_custom_target(run_${TARGET_NAME}
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R ^${ARG_TEST_NAME}$
        DEPENDS ${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running ${ARG_TEST_NAME}"
    )

    message(STATUS "ELF startup inspection configured:")
    message(STATUS "  - Target: ${TARGET_NAME}")
    message(STATUS "  - Test:   ${ARG_TEST_NAME}")
endfunction()
