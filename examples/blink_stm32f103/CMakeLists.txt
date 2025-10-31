# Blink example for STM32F103C8 (Blue Pill)

add_executable(blink_stm32f103
    main.cpp
    ../../boards/stm32f103c8/startup.cpp
)

target_include_directories(blink_stm32f103 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_options(blink_stm32f103 PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -O2
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

target_link_options(blink_stm32f103 PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -specs=nosys.specs
    -specs=nano.specs
    -T${CMAKE_SOURCE_DIR}/boards/stm32f103c8/STM32F103C8.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_stm32f103.map
    -Wl,--print-memory-usage
)

# Generate additional output formats
add_custom_command(TARGET blink_stm32f103 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:blink_stm32f103> ${CMAKE_CURRENT_BINARY_DIR}/blink_stm32f103.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:blink_stm32f103> ${CMAKE_CURRENT_BINARY_DIR}/blink_stm32f103.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:blink_stm32f103>
    COMMENT "Generating HEX and BIN files for STM32F103"
)
