# Alloy Board: Arduino Zero
# ATSAMD21G18 ARM Cortex-M0+

# Board identification
set(MICROCORE_BOARD_NAME "Arduino Zero" CACHE STRING "Board name" FORCE)
set(MICROCORE_MCU "ATSAMD21G18" CACHE STRING "MCU model" FORCE)
set(MICROCORE_ARCH "arm-cortex-m0plus" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(MICROCORE_CLOCK_FREQ_HZ 48000000 CACHE STRING "System clock frequency (48 MHz)")

# Peripherals available
set(MICROCORE_HAS_GPIO ON)
set(MICROCORE_HAS_SERCOM ON)  # UART/I2C/SPI via SERCOM
set(MICROCORE_HAS_ADC ON)
set(MICROCORE_HAS_DAC ON)
set(MICROCORE_HAS_PWM ON)
set(MICROCORE_HAS_USB ON)

# Pinout
set(MICROCORE_LED_PIN "PA17" CACHE STRING "On-board LED pin (PA17)")

# Memory
set(MICROCORE_FLASH_SIZE "256KB" CACHE STRING "Flash size")
set(MICROCORE_RAM_SIZE "32KB" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# Include code generation module
include(codegen)

# Generate code for ATSAMD21G18A
alloy_generate_code(
    MCU ATSAMD21G18A
    VENDOR atmel
    FAMILY samd21
)

# Use generated code if available
if(MICROCORE_CODEGEN_AVAILABLE)
    include_directories(${MICROCORE_GENERATED_DIR})
    add_compile_definitions(MICROCORE_GENERATED_NAMESPACE=alloy::hal::atmel::samd21::atsamd21g18a)
endif()

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
message(STATUS "  MCU: ${MICROCORE_MCU}")
message(STATUS "  Flash: ${MICROCORE_FLASH_SIZE}, RAM: ${MICROCORE_RAM_SIZE}")
