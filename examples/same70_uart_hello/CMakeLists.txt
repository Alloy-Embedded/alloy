# UART Hello World for ATSAME70 Xplained
# Demonstrates UART initialization and serial communication

add_executable(same70_uart_hello
    main.cpp
    ${CMAKE_SOURCE_DIR}/src/generated/atmel/same70/atsame70q21/startup.cpp
)

target_include_directories(same70_uart_hello PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/generated/atmel/same70/atsame70q21
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(same70_uart_hello)

# Additional compile options
target_compile_options(same70_uart_hello PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(same70_uart_hello PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/same70_uart_hello.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(same70_uart_hello)

# Create flash target
alloy_add_flash_target(same70_uart_hello)
