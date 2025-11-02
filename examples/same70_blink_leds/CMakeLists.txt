# Advanced LED patterns for ATSAME70 Xplained
# Demonstrates multiple LED effects: blink, breathing, morse code, heartbeat

add_executable(same70_blink_leds
    main.cpp
    ${CMAKE_SOURCE_DIR}/src/generated/atmel/same70/atsame70q21/startup.cpp
)

target_include_directories(same70_blink_leds PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/generated/atmel/same70/atsame70q21
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(same70_blink_leds)

# Additional compile options
target_compile_options(same70_blink_leds PRIVATE
    -O2
    -Wall
    -Wextra
)

# Additional link options
target_link_options(same70_blink_leds PRIVATE
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/same70_blink_leds.map
    -Wl,--print-memory-usage
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(same70_blink_leds)

# Create flash target
alloy_add_flash_target(same70_blink_leds)
