# Alloy Board: Host (Native development machine)
# For development and testing without hardware

# Board identification
set(ALLOY_BOARD_NAME "Host (Native)" CACHE STRING "Board name")
set(ALLOY_MCU "native" CACHE STRING "MCU model")
set(ALLOY_ARCH "native" CACHE STRING "CPU architecture")

# Clock configuration (not applicable for host)
set(ALLOY_CLOCK_FREQ_HZ 0 CACHE STRING "System clock frequency (N/A for host)")

# Peripherals available (all simulated)
set(ALLOY_HAS_GPIO ON)
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_I2C ON)
set(ALLOY_HAS_SPI ON)
set(ALLOY_HAS_ADC ON)
set(ALLOY_HAS_PWM ON)

# Pinout (simulated pins)
set(ALLOY_LED_PIN 25 CACHE STRING "Simulated LED pin")

# Memory (not applicable for host)
set(ALLOY_FLASH_SIZE "N/A" CACHE STRING "Flash size")
set(ALLOY_RAM_SIZE "N/A" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/host.cmake" CACHE FILEPATH "Toolchain file")

message(STATUS "Board configured: ${ALLOY_BOARD_NAME}")
