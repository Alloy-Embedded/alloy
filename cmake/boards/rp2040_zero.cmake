# Alloy Board: Waveshare RP2040-Zero
# RP2040 dual ARM Cortex-M0+ with WS2812 RGB LED

# Board identification
set(MICROCORE_BOARD_NAME "Waveshare RP2040-Zero" CACHE STRING "Board name" FORCE)
set(MICROCORE_MCU "RP2040" CACHE STRING "MCU model" FORCE)
set(MICROCORE_ARCH "arm-cortex-m0plus" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(MICROCORE_CLOCK_FREQ_HZ 125000000 CACHE STRING "System clock frequency (125 MHz)")

# Peripherals available
set(MICROCORE_HAS_GPIO ON)
set(MICROCORE_HAS_UART ON)
set(MICROCORE_HAS_I2C ON)
set(MICROCORE_HAS_SPI ON)
set(MICROCORE_HAS_ADC ON)
set(MICROCORE_HAS_PWM ON)
set(MICROCORE_HAS_PIO ON)   # Programmable IO
set(MICROCORE_HAS_USB ON)
set(MICROCORE_HAS_DUAL_CORE ON)

# Pinout
set(MICROCORE_LED_PIN "GPIO16" CACHE STRING "On-board WS2812 RGB LED pin (GPIO16)")

# Memory
set(MICROCORE_FLASH_SIZE "2MB" CACHE STRING "Flash size (external)")
set(MICROCORE_RAM_SIZE "264KB" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# Include code generation module
include(codegen)

# Generate code for RP2040
alloy_generate_code(
    MCU RP2040
    VENDOR raspberrypi
    FAMILY rp2040
)

# Use generated code if available
if(MICROCORE_CODEGEN_AVAILABLE)
    # Make generated directory available to targets
    include_directories(${MICROCORE_GENERATED_DIR})

    # Define namespace for generated code (full namespace path)
    add_compile_definitions(MICROCORE_GENERATED_NAMESPACE=alloy::generated::rp2040)
endif()

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
message(STATUS "  MCU: ${MICROCORE_MCU}")
message(STATUS "  Flash: ${MICROCORE_FLASH_SIZE}, RAM: ${MICROCORE_RAM_SIZE}")
