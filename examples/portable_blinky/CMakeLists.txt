# Portable Blinky Example
# This example compiles for ANY MCU - just change ALLOY_MCU

# Board-specific LED pin configuration
# The LED_PIN is passed as a compile definition
# Applications use the same code, just different pin numbers

if(ALLOY_MCU MATCHES "STM32F103")
    # BluePill: PC13 = 45 (port C * 16 + pin 13)
    set(LED_PIN 45)
    set(BOARD_NAME "BluePill (STM32F103)")

elseif(ALLOY_MCU MATCHES "STM32F446")
    # Nucleo-F446RE: PA5 = 5
    set(LED_PIN 5)
    set(BOARD_NAME "Nucleo-F446RE")

elseif(ALLOY_MCU MATCHES "nRF52")
    # nRF52840 DK: LED1 = P0.13 = 13
    set(LED_PIN 13)
    set(BOARD_NAME "nRF52840-DK")

elseif(ALLOY_MCU MATCHES "RP2040")
    # Raspberry Pi Pico: GPIO25 = 25
    set(LED_PIN 25)
    set(BOARD_NAME "Raspberry Pi Pico")

elseif(ALLOY_MCU MATCHES "ESP32")
    # ESP32 DevKit: GPIO2 = 2
    set(LED_PIN 2)
    set(BOARD_NAME "ESP32 DevKit")

else()
    # Default
    set(LED_PIN 13)
    set(BOARD_NAME "Unknown Board")
endif()

message(STATUS "=== Portable Blinky Configuration ===")
message(STATUS "  Board:   ${BOARD_NAME}")
message(STATUS "  MCU:     ${ALLOY_MCU}")
message(STATUS "  LED Pin: ${LED_PIN}")
message(STATUS "======================================")

# Create executable
add_executable(portable_blinky
    main.cpp
)

# Link HAL library (provides GPIO, UART, etc)
target_link_libraries(portable_blinky PRIVATE
    alloy_hal
    alloy_core
)

# Pass LED pin to application
target_compile_definitions(portable_blinky PRIVATE
    LED_PIN=${LED_PIN}
)

# Optional: Enable UART debug output
# Uncomment to enable serial messages
# target_compile_definitions(portable_blinky PRIVATE USE_UART)

# Set linker script (vendor-specific)
# This is set automatically by the board configuration
if(ALLOY_LINKER_SCRIPT)
    target_link_options(portable_blinky PRIVATE
        -T${ALLOY_LINKER_SCRIPT}
    )
endif()
