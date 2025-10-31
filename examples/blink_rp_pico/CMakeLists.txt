# Blink example for Raspberry Pi Pico (RP2040)

add_executable(blink_rp_pico
    main.cpp
    ../../boards/raspberry_pi_pico/startup.cpp
)

target_include_directories(blink_rp_pico PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_options(blink_rp_pico PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    -O2
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

target_link_options(blink_rp_pico PRIVATE
    -mcpu=cortex-m0plus
    -mthumb
    -specs=nosys.specs
    -specs=nano.specs
    -T${CMAKE_SOURCE_DIR}/boards/raspberry_pi_pico/RP2040.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/blink_rp_pico.map
    -Wl,--print-memory-usage
)

# Generate additional output formats
add_custom_command(TARGET blink_rp_pico POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:blink_rp_pico> ${CMAKE_CURRENT_BINARY_DIR}/blink_rp_pico.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:blink_rp_pico> ${CMAKE_CURRENT_BINARY_DIR}/blink_rp_pico.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:blink_rp_pico>
    COMMENT "Generating HEX and BIN files for RP Pico"
)

# Note: For actual hardware, the .bin needs to be converted to .uf2 format
# using elf2uf2 tool from pico-sdk
