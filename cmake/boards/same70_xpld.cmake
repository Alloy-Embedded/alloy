# Alloy Board: Atmel SAME70 Xplained
# ARM Cortex-M7 @ 300MHz with FPU and DSP

# Board identification
set(ALLOY_BOARD_NAME "Atmel SAME70 Xplained" CACHE STRING "Board name" FORCE)
set(ALLOY_MCU "ATSAME70Q21" CACHE STRING "MCU model" FORCE)
set(ALLOY_ARCH "arm-cortex-m7" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(ALLOY_CLOCK_FREQ_HZ 300000000 CACHE STRING "System clock frequency (300 MHz)")

# Peripherals available
set(ALLOY_HAS_GPIO ON)
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_USART ON)
set(ALLOY_HAS_I2C ON)
set(ALLOY_HAS_SPI ON)
set(ALLOY_HAS_ADC ON)
set(ALLOY_HAS_DAC ON)
set(ALLOY_HAS_PWM ON)
set(ALLOY_HAS_CAN ON)
set(ALLOY_HAS_USB ON)
set(ALLOY_HAS_ETHERNET ON)
set(ALLOY_HAS_SDIO ON)

# Pinout
set(ALLOY_LED_PIN "PC8" CACHE STRING "On-board LED pin (LED0)")
set(ALLOY_UART_TX_PIN "PA10" CACHE STRING "UART0 TX pin")
set(ALLOY_UART_RX_PIN "PA9" CACHE STRING "UART0 RX pin")

# Memory
set(ALLOY_FLASH_SIZE "2MB" CACHE STRING "Flash size")
set(ALLOY_RAM_SIZE "384KB" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# Include code generation module
include(codegen)

# Generate code for SAME70
alloy_generate_code(
    MCU ATSAME70Q21
    VENDOR atmel
    FAMILY same70
)

# Use generated code if available
if(ALLOY_CODEGEN_AVAILABLE)
    include_directories(${ALLOY_GENERATED_DIR})
    add_compile_definitions(ALLOY_GENERATED_NAMESPACE=alloy::generated::same70)
endif()

message(STATUS "Board configured: ${ALLOY_BOARD_NAME}")
message(STATUS "  MCU: ${ALLOY_MCU} (${ALLOY_ARCH})")
message(STATUS "  Flash: ${ALLOY_FLASH_SIZE}, RAM: ${ALLOY_RAM_SIZE}")
message(STATUS "  Clock: ${ALLOY_CLOCK_FREQ_HZ} Hz")
