# Alloy Board: STM32F746 Discovery
# ARM Cortex-M7 with FPU and DSP

# Board identification
set(MICROCORE_BOARD_NAME "STM32F746 Discovery" CACHE STRING "Board name" FORCE)
set(MICROCORE_MCU "STM32F746VG" CACHE STRING "MCU model" FORCE)
set(MICROCORE_ARCH "arm-cortex-m7f" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(MICROCORE_CLOCK_FREQ_HZ 216000000 CACHE STRING "System clock frequency (216 MHz)")

# Peripherals available
set(MICROCORE_HAS_GPIO ON)
set(MICROCORE_HAS_UART ON)
set(MICROCORE_HAS_I2C ON)
set(MICROCORE_HAS_SPI ON)
set(MICROCORE_HAS_ADC ON)
set(MICROCORE_HAS_DAC ON)
set(MICROCORE_HAS_PWM ON)
set(MICROCORE_HAS_USB ON)
set(MICROCORE_HAS_FPU ON)
set(MICROCORE_HAS_DSP ON)
set(MICROCORE_HAS_LCD ON)

# Pinout
set(MICROCORE_LED_PIN "PI1" CACHE STRING "On-board LED pin (PI1 - Green)")

# Memory
set(MICROCORE_FLASH_SIZE_STR "1MB" CACHE STRING "Flash size (human-readable)" FORCE)
set(MICROCORE_RAM_SIZE_STR "340KB" CACHE STRING "RAM size (human-readable)" FORCE)
set(MICROCORE_FLASH_SIZE 1048576 CACHE STRING "Flash size in bytes" FORCE)
set(MICROCORE_RAM_SIZE 348160 CACHE STRING "RAM size in bytes (256KB + 64KB + 16KB + 4KB)" FORCE)

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# Include code generation module
include(codegen)

# Generate code for STM32F746VG
alloy_generate_code(
    MCU STM32F746
    VENDOR st
    FAMILY stm32f7
)

# Use generated code if available
if(MICROCORE_CODEGEN_AVAILABLE)
    # Make generated directory available to targets
    include_directories(${MICROCORE_GENERATED_DIR})

    # Define namespace for generated code (full namespace path)
    add_compile_definitions(MICROCORE_GENERATED_NAMESPACE=alloy::generated::stm32f746)
endif()

# Define MCU macro for universal GPIO header auto-detection
add_compile_definitions(MCU_STM32F746VG)

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
message(STATUS "  MCU: ${MICROCORE_MCU}")
message(STATUS "  Flash: ${MICROCORE_FLASH_SIZE_STR}, RAM: ${MICROCORE_RAM_SIZE_STR}")
