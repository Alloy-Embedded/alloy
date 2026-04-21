cmake_minimum_required(VERSION 3.25)

set(_uart_probe_source ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
set(_uart_probe_board_dir ${ALLOY_BOARD_SOURCE_DIR})

function(alloy_add_same70_uart_probe target_name define_name)
    add_executable(${target_name}
        ${_uart_probe_source}
        ${_uart_probe_board_dir}/board.cpp
        ${_uart_probe_board_dir}/syscalls.cpp
    )

    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/boards
    )
    target_link_libraries(${target_name} PRIVATE ${ALLOY_HAL_LIBRARY})

    target_compile_definitions(${target_name} PRIVATE ${define_name})
    target_compile_options(${target_name} PRIVATE
        -Wall -Wextra -Wpedantic -O0 -g3
        -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics
    )
    target_link_options(${target_name} PRIVATE
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        --specs=nano.specs
        --specs=nosys.specs
        -T${LINKER_SCRIPT}
    )

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target_name}> ${target_name}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> ${target_name}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target_name}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HEX and BIN files for ${target_name}"
    )
endfunction()

alloy_add_same70_uart_probe(same70_uart_probe_usart0_pb01 ALLOY_UART_PROBE_USART0_PB01)
alloy_add_same70_uart_probe(same70_uart_probe_uart1_pa56 ALLOY_UART_PROBE_UART1_PA56)
alloy_add_same70_uart_probe(same70_uart_probe_usart1_pb4_pa21 ALLOY_UART_PROBE_USART1_PB4_PA21)
