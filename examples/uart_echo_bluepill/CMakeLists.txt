# UART Echo Example for Blue Pill (STM32F103C8T6)

cmake_minimum_required(VERSION 3.25)

# This example requires the BluePill board
if(NOT ALLOY_BOARD STREQUAL "bluepill")
    message(WARNING "uart_echo_bluepill requires ALLOY_BOARD=bluepill")
    return()
endif()

# Define the executable
add_executable(uart_echo_bluepill
    main.cpp
)

# Link with STM32F1 HAL
target_link_libraries(uart_echo_bluepill PRIVATE
    alloy::hal::stm32f1
)

# Use generated startup code if available
if(ALLOY_CODEGEN_AVAILABLE AND ALLOY_GENERATED_SOURCES)
    target_sources(uart_echo_bluepill PRIVATE
        ${ALLOY_GENERATED_SOURCES}
    )
endif()

message(STATUS "Example: uart_echo_bluepill configured")
