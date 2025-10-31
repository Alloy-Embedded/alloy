# Blink example for ESP32 DevKit

add_executable(blink_esp32
    main.cpp
    ../../boards/esp32_devkit/startup.cpp
)

target_include_directories(blink_esp32 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_options(blink_esp32 PRIVATE
    -mlongcalls
    -mtext-section-literals
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -O2
)

target_link_options(blink_esp32 PRIVATE
    -mlongcalls
    -mtext-section-literals
    -T${CMAKE_SOURCE_DIR}/boards/esp32_devkit/esp32.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_esp32.map
    -Wl,--print-memory-usage
    -nostdlib
    -lgcc
    -lstdc++
)

# Generate additional output formats
add_custom_command(TARGET blink_esp32 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:blink_esp32> ${CMAKE_CURRENT_BINARY_DIR}/blink_esp32.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:blink_esp32>
    COMMENT "Generating BIN file for ESP32"
)
