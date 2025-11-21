# Alloy Board: ESP32 DevKit
# ESP32 dual-core Xtensa LX6 development board

# Board identification
set(MICROCORE_BOARD_NAME "ESP32 DevKit" CACHE STRING "Board name")
set(MICROCORE_MCU "ESP32" CACHE STRING "MCU model")
set(MICROCORE_ARCH "xtensa-lx6" CACHE STRING "CPU architecture")

# Clock configuration
set(MICROCORE_CLOCK_FREQ_HZ 160000000 CACHE STRING "System clock frequency (160 MHz)")

# Peripherals available
set(MICROCORE_HAS_GPIO ON)
set(MICROCORE_HAS_UART ON)
set(MICROCORE_HAS_I2C ON)
set(MICROCORE_HAS_SPI ON)
set(MICROCORE_HAS_ADC ON)
set(MICROCORE_HAS_PWM ON)
set(MICROCORE_HAS_WIFI ON)
set(MICROCORE_HAS_BLUETOOTH ON)

# Pinout
set(MICROCORE_LED_PIN "GPIO2" CACHE STRING "On-board LED pin (GPIO2)")

# Memory
set(MICROCORE_FLASH_SIZE "4MB" CACHE STRING "Flash size")
set(MICROCORE_RAM_SIZE "320KB" CACHE STRING "RAM size")

# Toolchain
# Note: Xtensa toolchain needs to be installed separately
# For now, we assume xtensa-esp32-elf-gcc is in PATH
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/xtensa-esp32-elf.cmake" CACHE FILEPATH "Toolchain file")

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
message(STATUS "  MCU: ${MICROCORE_MCU}")
message(STATUS "  Flash: ${MICROCORE_FLASH_SIZE}, RAM: ${MICROCORE_RAM_SIZE}")
