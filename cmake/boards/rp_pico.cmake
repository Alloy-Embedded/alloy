# Alloy Board: Raspberry Pi Pico
# RP2040 dual ARM Cortex-M0+

# Board identification
set(ALLOY_BOARD_NAME "Raspberry Pi Pico" CACHE STRING "Board name" FORCE)
set(ALLOY_MCU "RP2040" CACHE STRING "MCU model" FORCE)
set(ALLOY_ARCH "arm-cortex-m0plus" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(ALLOY_CLOCK_FREQ_HZ 125000000 CACHE STRING "System clock frequency (125 MHz)")

# Peripherals available
set(ALLOY_HAS_GPIO ON)
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_I2C ON)
set(ALLOY_HAS_SPI ON)
set(ALLOY_HAS_ADC ON)
set(ALLOY_HAS_PWM ON)
set(ALLOY_HAS_PIO ON)   # Programmable IO
set(ALLOY_HAS_USB ON)
set(ALLOY_HAS_DUAL_CORE ON)

# Pinout
set(ALLOY_LED_PIN "GPIO25" CACHE STRING "On-board LED pin (GPIO25)")

# Memory
set(ALLOY_FLASH_SIZE "2MB" CACHE STRING "Flash size (external)")
set(ALLOY_RAM_SIZE "264KB" CACHE STRING "RAM size")

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
if(ALLOY_CODEGEN_AVAILABLE)
    # Make generated directory available to targets
    include_directories(${ALLOY_GENERATED_DIR})

    # Define namespace for generated code (full namespace path)
    add_compile_definitions(ALLOY_GENERATED_NAMESPACE=alloy::generated::rp2040)
endif()

message(STATUS "Board configured: ${ALLOY_BOARD_NAME}")
message(STATUS "  MCU: ${ALLOY_MCU}")
message(STATUS "  Flash: ${ALLOY_FLASH_SIZE}, RAM: ${ALLOY_RAM_SIZE}")
