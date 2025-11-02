# RTOS Queue Example - Producer/Consumer Pattern

add_executable(rtos_queue
    main.cpp
    ../../boards/stm32f103c8/startup.cpp
    ../../src/rtos/scheduler.cpp
    ../../src/rtos/platform/arm_context.cpp
    ../../src/rtos/platform/arm_systick_integration.cpp
)

target_include_directories(rtos_queue PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/boards
)

target_compile_options(rtos_queue PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -O2
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -std=c++20
)

target_compile_definitions(rtos_queue PRIVATE
    STM32F103
)

target_link_options(rtos_queue PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -T${CMAKE_SOURCE_DIR}/boards/stm32f103c8/STM32F103C8.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/rtos_queue.map
    -Wl,--print-memory-usage
    -specs=nosys.specs
    -specs=nano.specs
)

# Generate additional output formats
add_custom_command(TARGET rtos_queue POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:rtos_queue> ${CMAKE_CURRENT_BINARY_DIR}/rtos_queue.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:rtos_queue> ${CMAKE_CURRENT_BINARY_DIR}/rtos_queue.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:rtos_queue>
    COMMENT "Generating HEX and BIN files for RTOS Queue example"
)
