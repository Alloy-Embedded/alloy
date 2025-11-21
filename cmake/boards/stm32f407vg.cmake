# Alloy Board: STM32F407VG Discovery
# ARM Cortex-M4F with FPU

# Board identification
set(MICROCORE_BOARD_NAME "STM32F407VG Discovery" CACHE STRING "Board name" FORCE)
set(MICROCORE_MCU "STM32F407VG" CACHE STRING "MCU model" FORCE)
set(MICROCORE_ARCH "arm-cortex-m4f" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(MICROCORE_CLOCK_FREQ_HZ 168000000 CACHE STRING "System clock frequency (168 MHz)")

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

# Pinout
set(MICROCORE_LED_PIN "PD12" CACHE STRING "On-board LED pin (PD12 - Green)")

# Memory
set(MICROCORE_FLASH_SIZE "1MB" CACHE STRING "Flash size")
set(MICROCORE_RAM_SIZE "192KB" CACHE STRING "RAM size (128KB + 64KB CCM)")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# Include code generation module
include(codegen)

# Generate code for STM32F407VG
alloy_generate_code(
    MCU STM32F407
    VENDOR unknown
    FAMILY stm32f4
)

# Use generated code if available
if(MICROCORE_CODEGEN_AVAILABLE)
    # Make generated directory available to targets
    include_directories(${MICROCORE_GENERATED_DIR})

    # Define namespace for generated code (full namespace path)
    add_compile_definitions(MICROCORE_GENERATED_NAMESPACE=alloy::generated::stm32f407)
endif()

# Define MCU macro for universal GPIO header auto-detection
add_compile_definitions(MCU_STM32F407VG)

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
message(STATUS "  MCU: ${MICROCORE_MCU}")
message(STATUS "  Flash: ${MICROCORE_FLASH_SIZE}, RAM: ${MICROCORE_RAM_SIZE}")
