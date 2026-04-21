cmake_minimum_required(VERSION 3.25)

add_executable(manual_uart_probe
    main.cpp
    ${ALLOY_BOARD_SOURCE_DIR}/board.cpp
    ${ALLOY_BOARD_SOURCE_DIR}/syscalls.cpp
)

target_include_directories(manual_uart_probe PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_link_libraries(manual_uart_probe PRIVATE ${ALLOY_HAL_LIBRARY})

target_compile_options(manual_uart_probe PRIVATE
    -Wall -Wextra -Wpedantic -O0 -g3
    -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-threadsafe-statics
)

target_link_options(manual_uart_probe PRIVATE
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    --specs=nosys.specs
    -T${LINKER_SCRIPT}
)

add_custom_command(TARGET manual_uart_probe POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:manual_uart_probe> manual_uart_probe.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:manual_uart_probe> manual_uart_probe.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:manual_uart_probe>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating HEX and BIN files for manual_uart_probe"
)
