set(ALLOY_ASSEMBLY_SAME70_BOARD_MATCH FALSE)
if(ALLOY_BOARD STREQUAL "same70_xplained" OR ALLOY_BOARD STREQUAL "same70_xpld")
    set(ALLOY_ASSEMBLY_SAME70_BOARD_MATCH TRUE)
endif()

if(NOT ALLOY_ASSEMBLY_SAME70_BOARD_MATCH)
    message(STATUS "Assembly SAME70 scenario skipped: select ALLOY_BOARD=same70_xplained (or same70_xpld).")
    return()
endif()

if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Assembly SAME70 scenario skipped: cross toolchain is required.")
    return()
endif()

set(ALLOY_ASSEMBLY_SAME70_TARGET same70_runtime_zero_overhead)

add_library(${ALLOY_ASSEMBLY_SAME70_TARGET} STATIC
    runtime_zero_overhead.cpp
)

target_link_libraries(${ALLOY_ASSEMBLY_SAME70_TARGET} PRIVATE
    ${ALLOY_HAL_LIBRARY}
)

target_include_directories(${ALLOY_ASSEMBLY_SAME70_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(${ALLOY_ASSEMBLY_SAME70_TARGET} PRIVATE cxx_std_23)
target_compile_options(${ALLOY_ASSEMBLY_SAME70_TARGET} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O2
    -g0
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
)

set_target_properties(${ALLOY_ASSEMBLY_SAME70_TARGET} PROPERTIES
    OUTPUT_NAME "same70_runtime_zero_overhead"
    FOLDER "tests/perf/assembly"
)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(ALLOY_ASSEMBLY_OBJDUMP "${CMAKE_OBJDUMP}")
if(NOT ALLOY_ASSEMBLY_OBJDUMP)
    get_filename_component(ALLOY_ASSEMBLY_TOOL_DIR "${CMAKE_NM}" DIRECTORY)
    find_program(ALLOY_ASSEMBLY_OBJDUMP
        NAMES arm-none-eabi-objdump objdump
        HINTS "${ALLOY_ASSEMBLY_TOOL_DIR}"
        REQUIRED
    )
endif()

set(ALLOY_ASSEMBLY_NM "${CMAKE_NM}")
if(NOT ALLOY_ASSEMBLY_NM)
    get_filename_component(ALLOY_ASSEMBLY_TOOL_DIR "${CMAKE_OBJDUMP}" DIRECTORY)
    find_program(ALLOY_ASSEMBLY_NM
        NAMES arm-none-eabi-nm nm
        HINTS "${ALLOY_ASSEMBLY_TOOL_DIR}"
        REQUIRED
    )
endif()

add_test(
    NAME assembly.same70.runtime_zero_overhead
    COMMAND
        ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tests/perf/assembly/scripts/verify_zero_overhead.py
        --artifact $<TARGET_FILE:${ALLOY_ASSEMBLY_SAME70_TARGET}>
        --objdump ${ALLOY_ASSEMBLY_OBJDUMP}
        --nm ${ALLOY_ASSEMBLY_NM}
        --pair manual_disable_same70_watchdog:runtime_disable_same70_watchdog
        --pair manual_set_same70_pioc8_high:runtime_set_same70_pioc8_high
        --pair manual_set_same70_usart0_chrl:runtime_set_same70_usart0_chrl
        --pair manual_set_same70_tc0_period:runtime_set_same70_tc0_period
        --pair manual_set_same70_pwm0_period:runtime_set_same70_pwm0_period
        --pair manual_set_same70_pwm0_duty:runtime_set_same70_pwm0_duty
)
set_tests_properties(assembly.same70.runtime_zero_overhead PROPERTIES
    LABELS "perf;assembly;same70;zero-overhead"
)

add_custom_target(run_same70_runtime_zero_overhead
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R ^assembly.same70.runtime_zero_overhead$
    DEPENDS ${ALLOY_ASSEMBLY_SAME70_TARGET}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running SAME70 runtime zero-overhead validation"
)

message(STATUS "Assembly SAME70 zero-overhead validation configured:")
message(STATUS "  - Target: ${ALLOY_ASSEMBLY_SAME70_TARGET}")
message(STATUS "  - Test:   assembly.same70.runtime_zero_overhead")
