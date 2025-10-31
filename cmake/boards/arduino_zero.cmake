# Alloy Board: Arduino Zero
# ATSAMD21G18 ARM Cortex-M0+

# Board identification
set(ALLOY_BOARD_NAME "Arduino Zero" CACHE STRING "Board name" FORCE)
set(ALLOY_MCU "ATSAMD21G18" CACHE STRING "MCU model" FORCE)
set(ALLOY_ARCH "arm-cortex-m0plus" CACHE STRING "CPU architecture" FORCE)

# Clock configuration
set(ALLOY_CLOCK_FREQ_HZ 48000000 CACHE STRING "System clock frequency (48 MHz)")

# Peripherals available
set(ALLOY_HAS_GPIO ON)
set(ALLOY_HAS_SERCOM ON)  # UART/I2C/SPI via SERCOM
set(ALLOY_HAS_ADC ON)
set(ALLOY_HAS_DAC ON)
set(ALLOY_HAS_PWM ON)
set(ALLOY_HAS_USB ON)

# Pinout
set(ALLOY_LED_PIN "PA17" CACHE STRING "On-board LED pin (PA17)")

# Memory
set(ALLOY_FLASH_SIZE "256KB" CACHE STRING "Flash size")
set(ALLOY_RAM_SIZE "32KB" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# Include code generation module
include(codegen)

# Generate code for ATSAMD21G18
alloy_generate_code(
    MCU ATSAMD21J18A
    VENDOR microchip_technology_inc
    FAMILY atsamd21j18a
)

# Use generated code if available
if(ALLOY_CODEGEN_AVAILABLE)
    # Make generated directory available to targets
    include_directories(${ALLOY_GENERATED_DIR})

    # Define namespace for generated code (full namespace path)
    add_compile_definitions(ALLOY_GENERATED_NAMESPACE=alloy::generated::atsamd21j18a)
endif()

message(STATUS "Board configured: ${ALLOY_BOARD_NAME}")
message(STATUS "  MCU: ${ALLOY_MCU}")
message(STATUS "  Flash: ${ALLOY_FLASH_SIZE}, RAM: ${ALLOY_RAM_SIZE}")
