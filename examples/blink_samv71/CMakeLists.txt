# Blink example for SAMV71 Xplained Ultra

add_executable(blink_samv71
    main.cpp
)

target_include_directories(blink_samv71 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(blink_samv71)

# Additional compile options
target_compile_options(blink_samv71 PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(blink_samv71 PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_samv71.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(blink_samv71)

# Create flash target
alloy_add_flash_target(blink_samv71)
