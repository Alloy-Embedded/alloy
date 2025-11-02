# Basic Logger example for STM32F103C8 (Blue Pill)

add_executable(logger_basic
    main.cpp
    ../../boards/stm32f103c8/startup.cpp
)

target_include_directories(logger_basic PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(logger_basic)

# Additional compile options
target_compile_options(logger_basic PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(logger_basic PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/logger_basic.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(logger_basic)

# Create flash target
alloy_add_flash_target(logger_basic)
