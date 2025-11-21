# Alloy Board: Atmel SAMV71 Xplained Ultra
# ARM Cortex-M7 @ 300MHz with FPU, DSP and Cache

# Board identification
set(MICROCORE_BOARD_NAME "Atmel SAMV71 Xplained Ultra" CACHE STRING "Board name" FORCE)
set(MICROCORE_MCU "ATSAMV71Q21" CACHE STRING "MCU model" FORCE)
set(MICROCORE_ARCH "arm-cortex-m7" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(MICROCORE_CLOCK_FREQ_HZ 300000000 CACHE STRING "System clock frequency (300 MHz)")

# Peripherals available
set(MICROCORE_HAS_GPIO ON)
set(MICROCORE_HAS_UART ON)
set(MICROCORE_HAS_USART ON)
set(MICROCORE_HAS_I2C ON)
set(MICROCORE_HAS_SPI ON)
set(MICROCORE_HAS_ADC ON)
set(MICROCORE_HAS_DAC ON)
set(MICROCORE_HAS_PWM ON)
set(MICROCORE_HAS_CAN ON)
set(MICROCORE_HAS_USB ON)
set(MICROCORE_HAS_ETHERNET ON)
set(MICROCORE_HAS_SDIO ON)
set(MICROCORE_HAS_CAMERA ON)

# Pinout
set(MICROCORE_LED_PIN "PA23" CACHE STRING "On-board LED pin (LED0)")
set(MICROCORE_UART_TX_PIN "PB4" CACHE STRING "UART0 TX pin")
set(MICROCORE_UART_RX_PIN "PB0" CACHE STRING "UART0 RX pin")

# Memory
set(MICROCORE_FLASH_SIZE "2MB" CACHE STRING "Flash size")
set(MICROCORE_RAM_SIZE "384KB" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# Include code generation module
include(codegen)

# Generate code for SAMV71
alloy_generate_code(
    MCU ATSAMV71Q21
    VENDOR atmel
    FAMILY samv71
)

# Use generated code if available
if(MICROCORE_CODEGEN_AVAILABLE)
    include_directories(${MICROCORE_GENERATED_DIR})
    add_compile_definitions(MICROCORE_GENERATED_NAMESPACE=alloy::hal::atmel::samv71::atsamv71q21)
endif()

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
message(STATUS "  MCU: ${MICROCORE_MCU} (${MICROCORE_ARCH})")
message(STATUS "  Flash: ${MICROCORE_FLASH_SIZE}, RAM: ${MICROCORE_RAM_SIZE}")
message(STATUS "  Clock: ${MICROCORE_CLOCK_FREQ_HZ} Hz")
