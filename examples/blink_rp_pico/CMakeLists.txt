# Blink example for Raspberry Pi Pico (RP2040)

add_executable(blink_rp_pico
    main.cpp
    ../../boards/raspberry_pi_pico/startup.cpp
)

target_include_directories(blink_rp_pico PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(blink_rp_pico)

# Additional compile options
target_compile_options(blink_rp_pico PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(blink_rp_pico PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_rp_pico.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(blink_rp_pico)

# Create flash target (will use picotool if available, otherwise manual instructions)
alloy_add_flash_target(blink_rp_pico)
