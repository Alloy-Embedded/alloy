# Blink example for STM32F407VG Discovery

add_executable(blink_stm32f407
    main.cpp
    ../../boards/stm32f407vg/startup.cpp
)

target_include_directories(blink_stm32f407 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_options(blink_stm32f407 PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -O2
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

target_link_options(blink_stm32f407 PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -specs=nosys.specs
    -specs=nano.specs
    -T${CMAKE_SOURCE_DIR}/boards/stm32f407vg/STM32F407VG.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_stm32f407.map
    -Wl,--print-memory-usage
)

# Generate additional output formats
add_custom_command(TARGET blink_stm32f407 POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:blink_stm32f407> ${CMAKE_CURRENT_BINARY_DIR}/blink_stm32f407.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:blink_stm32f407> ${CMAKE_CURRENT_BINARY_DIR}/blink_stm32f407.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:blink_stm32f407>
    COMMENT "Generating HEX and BIN files for STM32F407"
)
