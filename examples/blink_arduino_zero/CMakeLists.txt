# Blink example for Arduino Zero

add_executable(blink_arduino_zero
    main.cpp
)

target_include_directories(blink_arduino_zero PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(blink_arduino_zero)

# Additional compile options
target_compile_options(blink_arduino_zero PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(blink_arduino_zero PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_arduino_zero.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(blink_arduino_zero)

# Create flash target
alloy_add_flash_target(blink_arduino_zero)
