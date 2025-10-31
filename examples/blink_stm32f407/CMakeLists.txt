# Blink example for STM32F407VG Discovery

add_executable(blink_stm32f407
    main.cpp
    ../../boards/stm32f407vg/startup.cpp
)

target_include_directories(blink_stm32f407 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(blink_stm32f407)

# Additional compile options
target_compile_options(blink_stm32f407 PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(blink_stm32f407 PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_stm32f407.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(blink_stm32f407)

# Create flash target
alloy_add_flash_target(blink_stm32f407)
