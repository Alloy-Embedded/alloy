# MicroCore API Tier Examples
#
# Demonstrates the three-tier API system:
# - Simple tier: Factory methods (gpio_simple.hpp, uart_simple.hpp, etc.)
# - Fluent tier: Builder pattern (gpio_fluent.hpp, uart_fluent.hpp, etc.)
# - Expert tier: Full control (gpio_expert.hpp, uart_expert.hpp, etc.)

# ============================================================================
# GPIO Examples (All Platforms)
# ============================================================================

# Simple Tier Examples
add_executable(simple_gpio_blink
    simple_gpio_blink.cpp
)
target_link_libraries(simple_gpio_blink PRIVATE
    ucore_hal
    board_config
)

add_executable(simple_gpio_button
    simple_gpio_button.cpp
)
target_link_libraries(simple_gpio_button PRIVATE
    ucore_hal
    board_config
)

# Fluent Tier Examples
add_executable(fluent_gpio_blink
    fluent_gpio_blink.cpp
)
target_link_libraries(fluent_gpio_blink PRIVATE
    ucore_hal
    board_config
)

# Expert Tier Examples
add_executable(expert_gpio_blink
    expert_gpio_blink.cpp
)
target_link_libraries(expert_gpio_blink PRIVATE
    ucore_hal
    board_config
)

# ============================================================================
# UART Examples (All Platforms)
# ============================================================================

# Simple Tier UART - Echo server
add_executable(simple_uart_echo
    simple_uart_echo.cpp
)
target_link_libraries(simple_uart_echo PRIVATE
    ucore_hal
    board_config
)

# Fluent Tier UART - Advanced configuration
add_executable(fluent_uart_config
    fluent_uart_config.cpp
)
target_link_libraries(fluent_uart_config PRIVATE
    ucore_hal
    board_config
)

# Expert Tier UART - Low-power operation (STM32G0 only)
add_executable(expert_uart_lowpower
    expert_uart_lowpower.cpp
)
target_link_libraries(expert_uart_lowpower PRIVATE
    ucore_hal
    board_config
)

# Generate bin/hex files for all examples (embedded targets only)
if(NOT MICROCORE_PLATFORM STREQUAL "host")
    foreach(TARGET
            simple_gpio_blink simple_gpio_button
            fluent_gpio_blink
            expert_gpio_blink
            simple_uart_echo
            fluent_uart_config
            expert_uart_lowpower)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET}.elf ${TARGET}.bin
            COMMAND ${CMAKE_OBJCOPY} -O ihex ${TARGET}.elf ${TARGET}.hex
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating ${TARGET}.bin and ${TARGET}.hex"
        )
    endforeach()
endif()
