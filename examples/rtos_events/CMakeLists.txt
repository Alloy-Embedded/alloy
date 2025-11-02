# RTOS Event Flags Example

add_executable(rtos_events
    main.cpp
    ../../boards/stm32f103c8/startup.cpp
    ../../src/rtos/scheduler.cpp
    ../../src/rtos/platform/arm_context.cpp
    ../../src/rtos/platform/arm_systick_integration.cpp
)

target_include_directories(rtos_events PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_options(rtos_events PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -O2
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -std=c++20
)

target_compile_definitions(rtos_events PRIVATE
    STM32F103
)

target_link_options(rtos_events PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${CMAKE_SOURCE_DIR}/boards/stm32f103c8/STM32F103C8.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/rtos_events.map
    -Wl,--print-memory-usage
    -specs=nosys.specs
    -specs=nano.specs
)

# Generate additional output formats
add_custom_command(TARGET rtos_events POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:rtos_events> ${CMAKE_CURRENT_BINARY_DIR}/rtos_events.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:rtos_events> ${CMAKE_CURRENT_BINARY_DIR}/rtos_events.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:rtos_events>
    COMMENT "Generating HEX and BIN files for RTOS Event Flags example"
)
