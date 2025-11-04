# Blink example for SAME70 Xplained

add_executable(blink_same70
    main.cpp
)

target_include_directories(blink_same70 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(blink_same70)

# Additional compile options
target_compile_options(blink_same70 PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(blink_same70 PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_same70.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(blink_same70)

# Create flash target
alloy_add_flash_target(blink_same70)
