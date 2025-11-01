# RTOS Blink example for ESP32 DevKit

add_executable(rtos_blink_esp32
    main.cpp
    ../../boards/esp32_devkit/startup.cpp
    ../../src/rtos/scheduler.cpp
    ../../src/rtos/platform/xtensa_context.cpp
    ../../src/rtos/platform/xtensa_systick_integration.cpp
)

target_include_directories(rtos_blink_esp32 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

# Apply board-specific flags and linker script
alloy_target_board_settings(rtos_blink_esp32)

# ESP32-specific compile options
target_compile_options(rtos_blink_esp32 PRIVATE
    -mtext-section-literals
    -O2
    -Wall
    -Wextra
    -DESP32
)

# ESP32-specific link options
target_link_options(rtos_blink_esp32 PRIVATE
    -mtext-section-literals
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/rtos_blink_esp32.map
    -Wl,--print-memory-usage
    -nostartfiles
    -lc
    -lgcc
    -lstdc++
)

# Generate binary outputs (.bin, .hex) and print size
alloy_add_binary_outputs(rtos_blink_esp32)

# Create flash target (default port: /dev/ttyUSB0, baud: 115200)
alloy_add_flash_target(rtos_blink_esp32 PORT /dev/ttyUSB0 BAUD 115200)
