# Alloy Board: Host (Native development machine)
# For development and testing without hardware

# Board identification
set(MICROCORE_BOARD_NAME "Host (Native)" CACHE STRING "Board name")
set(MICROCORE_MCU "native" CACHE STRING "MCU model")
set(MICROCORE_ARCH "native" CACHE STRING "CPU architecture")

# Clock configuration (not applicable for host)
set(MICROCORE_CLOCK_FREQ_HZ 0 CACHE STRING "System clock frequency (N/A for host)")

# Peripherals available (all simulated)
set(MICROCORE_HAS_GPIO ON)
set(MICROCORE_HAS_UART ON)
set(MICROCORE_HAS_I2C ON)
set(MICROCORE_HAS_SPI ON)
set(MICROCORE_HAS_ADC ON)
set(MICROCORE_HAS_PWM ON)

# Pinout (simulated pins)
set(MICROCORE_LED_PIN 25 CACHE STRING "Simulated LED pin")

# Memory (not applicable for host)
set(MICROCORE_FLASH_SIZE "N/A" CACHE STRING "Flash size")
set(MICROCORE_RAM_SIZE "N/A" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/host.cmake" CACHE FILEPATH "Toolchain file")

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
