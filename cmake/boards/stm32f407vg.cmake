# Alloy Board: STM32F407VG Discovery
# ARM Cortex-M4F with FPU

# Board identification
set(ALLOY_BOARD_NAME "STM32F407VG Discovery" CACHE STRING "Board name" FORCE)
set(ALLOY_MCU "STM32F407VG" CACHE STRING "MCU model" FORCE)
set(ALLOY_ARCH "arm-cortex-m4f" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(ALLOY_CLOCK_FREQ_HZ 168000000 CACHE STRING "System clock frequency (168 MHz)")

# Peripherals available
set(ALLOY_HAS_GPIO ON)
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_I2C ON)
set(ALLOY_HAS_SPI ON)
set(ALLOY_HAS_ADC ON)
set(ALLOY_HAS_DAC ON)
set(ALLOY_HAS_PWM ON)
set(ALLOY_HAS_USB ON)
set(ALLOY_HAS_FPU ON)

# Pinout
set(ALLOY_LED_PIN "PD12" CACHE STRING "On-board LED pin (PD12 - Green)")

# Memory
set(ALLOY_FLASH_SIZE "1MB" CACHE STRING "Flash size")
set(ALLOY_RAM_SIZE "192KB" CACHE STRING "RAM size (128KB + 64KB CCM)")

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
if(ALLOY_CODEGEN_AVAILABLE)
    # Make generated directory available to targets
    include_directories(${ALLOY_GENERATED_DIR})

    # Define namespace for generated code (full namespace path)
    add_compile_definitions(ALLOY_GENERATED_NAMESPACE=alloy::generated::stm32f407)
endif()

message(STATUS "Board configured: ${ALLOY_BOARD_NAME}")
message(STATUS "  MCU: ${ALLOY_MCU}")
message(STATUS "  Flash: ${ALLOY_FLASH_SIZE}, RAM: ${ALLOY_RAM_SIZE}")
