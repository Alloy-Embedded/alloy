# Alloy Board: Blue Pill (STM32F103C8T6)
# Popular STM32 development board

# Board identification
set(MICROCORE_BOARD_NAME "Blue Pill" CACHE STRING "Board name" FORCE)
set(MICROCORE_MCU "STM32F103C8" CACHE STRING "MCU model" FORCE)
set(MICROCORE_ARCH "arm-cortex-m3" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(MICROCORE_CLOCK_FREQ_HZ 72000000 CACHE STRING "System clock frequency (72 MHz)")

# Peripherals available
set(MICROCORE_HAS_GPIO ON)
set(MICROCORE_HAS_UART ON)
set(MICROCORE_HAS_I2C ON)
set(MICROCORE_HAS_SPI ON)
set(MICROCORE_HAS_ADC ON)
set(MICROCORE_HAS_PWM ON)

# Pinout
set(MICROCORE_LED_PIN "PC13" CACHE STRING "On-board LED pin (PC13)")

# Memory
set(MICROCORE_FLASH_SIZE_STR "64KB" CACHE STRING "Flash size (human-readable)" FORCE)
set(MICROCORE_RAM_SIZE_STR "20KB" CACHE STRING "RAM size (human-readable)" FORCE)
set(MICROCORE_FLASH_SIZE 65536 CACHE STRING "Flash size in bytes" FORCE)
set(MICROCORE_RAM_SIZE 20480 CACHE STRING "RAM size in bytes" FORCE)

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# Include code generation module
include(codegen)

# Generate code for STM32F103C8
alloy_generate_code(
    MCU STM32F103C8
    VENDOR st
    FAMILY stm32f1
)

# Use generated code if available
if(MICROCORE_CODEGEN_AVAILABLE)
    # Make generated directory available to targets
    include_directories(${MICROCORE_GENERATED_DIR})

    # Define namespace for generated code (full namespace path)
    add_compile_definitions(MICROCORE_GENERATED_NAMESPACE=alloy::generated::stm32f103c8)
endif()

# Define MCU macro for universal GPIO header auto-detection
add_compile_definitions(MCU_STM32F103C8)

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
message(STATUS "  MCU: ${MICROCORE_MCU}")
message(STATUS "  Flash: ${MICROCORE_FLASH_SIZE_STR}, RAM: ${MICROCORE_RAM_SIZE_STR}")
