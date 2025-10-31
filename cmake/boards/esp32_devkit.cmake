# Alloy Board: ESP32 DevKit
# ESP32 dual-core Xtensa LX6 development board

# Board identification
set(ALLOY_BOARD_NAME "ESP32 DevKit" CACHE STRING "Board name")
set(ALLOY_MCU "ESP32" CACHE STRING "MCU model")
set(ALLOY_ARCH "xtensa-lx6" CACHE STRING "CPU architecture")

# Clock configuration
set(ALLOY_CLOCK_FREQ_HZ 160000000 CACHE STRING "System clock frequency (160 MHz)")

# Peripherals available
set(ALLOY_HAS_GPIO ON)
set(ALLOY_HAS_UART ON)
set(ALLOY_HAS_I2C ON)
set(ALLOY_HAS_SPI ON)
set(ALLOY_HAS_ADC ON)
set(ALLOY_HAS_PWM ON)
set(ALLOY_HAS_WIFI ON)
set(ALLOY_HAS_BLUETOOTH ON)

# Pinout
set(ALLOY_LED_PIN "GPIO2" CACHE STRING "On-board LED pin (GPIO2)")

# Memory
set(ALLOY_FLASH_SIZE "4MB" CACHE STRING "Flash size")
set(ALLOY_RAM_SIZE "320KB" CACHE STRING "RAM size")

# Toolchain
# Note: Xtensa toolchain needs to be installed separately
# For now, we assume xtensa-esp32-elf-gcc is in PATH
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/xtensa-esp32-elf.cmake" CACHE FILEPATH "Toolchain file")

message(STATUS "Board configured: ${ALLOY_BOARD_NAME}")
message(STATUS "  MCU: ${ALLOY_MCU}")
message(STATUS "  Flash: ${ALLOY_FLASH_SIZE}, RAM: ${ALLOY_RAM_SIZE}")
