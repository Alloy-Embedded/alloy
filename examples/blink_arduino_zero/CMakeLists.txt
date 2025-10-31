# Blink example for Arduino Zero (ATSAMD21G18)

add_executable(blink_arduino_zero
    main.cpp
    ../../boards/arduino_zero/startup.cpp
)

target_include_directories(blink_arduino_zero PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_options(blink_arduino_zero PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    -O2
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

target_link_options(blink_arduino_zero PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    -specs=nosys.specs
    -specs=nano.specs
    -T${CMAKE_SOURCE_DIR}/boards/arduino_zero/ATSAMD21G18.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_arduino_zero.map
    -Wl,--print-memory-usage
)

# Generate additional output formats
add_custom_command(TARGET blink_arduino_zero POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:blink_arduino_zero> ${CMAKE_CURRENT_BINARY_DIR}/blink_arduino_zero.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:blink_arduino_zero> ${CMAKE_CURRENT_BINARY_DIR}/blink_arduino_zero.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:blink_arduino_zero>
    COMMENT "Generating HEX and BIN files for Arduino Zero"
)
