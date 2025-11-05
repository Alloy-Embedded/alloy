# Blink example for Waveshare RP2040-Zero using PIO

add_executable(blink_rp2040_zero_pio
    main.c
    ../../boards/waveshare_rp2040_zero/startup.cpp
    ../../src/hal/vendors/raspberrypi/rp2040/boot2_w25q080.S
)

target_include_directories(blink_rp2040_zero_pio PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/hal/raspberry/rp2040
    ${CMAKE_SOURCE_DIR}/boards
    ${CMAKE_SOURCE_DIR}/boards/waveshare_rp2040_zero
)

# Apply board-specific flags and linker script
alloy_target_board_settings(blink_rp2040_zero_pio)

# Additional compile options
target_compile_options(blink_rp2040_zero_pio PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(blink_rp2040_zero_pio PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_rp2040_zero_pio.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(blink_rp2040_zero_pio)

# Create flash target
alloy_add_flash_target(blink_rp2040_zero_pio)
