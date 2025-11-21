# Alloy Board: STM32 Nucleo-G0B1RE
# ARM Cortex-M0+ @ 64MHz

# Board identification
set(MICROCORE_BOARD_NAME "STM32 Nucleo-G0B1RE" CACHE STRING "Board name" FORCE)
set(MICROCORE_MCU "STM32G0B1RET6" CACHE STRING "MCU model" FORCE)
set(MICROCORE_ARCH "arm-cortex-m0plus" CACHE STRING "CPU architecture" FORCE)
set(MICROCORE_BOARD "nucleo_g0b1re" CACHE STRING "Board directory name" FORCE)

# Clock configuration
set(MICROCORE_CLOCK_FREQ_HZ 64000000 CACHE STRING "System clock frequency (64 MHz)")

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

# Pinout
set(MICROCORE_LED_PIN "PA5" CACHE STRING "On-board LED pin (LD4)")
set(MICROCORE_UART_TX_PIN "PA2" CACHE STRING "USART2 TX pin (ST-Link VCP)")
set(MICROCORE_UART_RX_PIN "PA3" CACHE STRING "USART2 RX pin (ST-Link VCP)")

# Memory
set(MICROCORE_FLASH_SIZE "512KB" CACHE STRING "Flash size")
set(MICROCORE_RAM_SIZE "144KB" CACHE STRING "RAM size")

# Toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../toolchains/arm-none-eabi.cmake" CACHE FILEPATH "Toolchain file")

# ARM Cortex-M0+ specific flags
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb -mfloat-abi=soft" CACHE STRING "C compiler flags")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb -mfloat-abi=soft" CACHE STRING "C++ compiler flags")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb" CACHE STRING "ASM compiler flags")

# Linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/boards/${MICROCORE_BOARD}/STM32G0B1RET6.ld" CACHE FILEPATH "Linker script")

# Startup code
set(STARTUP_SOURCE "${CMAKE_SOURCE_DIR}/src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp" CACHE FILEPATH "Startup code")

# Board header path (for examples)
set(BOARD_HEADER_PATH "${CMAKE_SOURCE_DIR}/boards/${MICROCORE_BOARD}/board.hpp" CACHE FILEPATH "Board header")

# Flash command (using ST-Link)
set(FLASH_COMMAND "st-flash write ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin 0x08000000" CACHE STRING "Flash command")

message(STATUS "Board configured: ${MICROCORE_BOARD_NAME}")
message(STATUS "  MCU: ${MICROCORE_MCU} (${MICROCORE_ARCH})")
message(STATUS "  Flash: ${MICROCORE_FLASH_SIZE}, RAM: ${MICROCORE_RAM_SIZE}")
message(STATUS "  Clock: ${MICROCORE_CLOCK_FREQ_HZ} Hz")
message(STATUS "  Linker script: ${LINKER_SCRIPT}")
message(STATUS "  Startup code: ${STARTUP_SOURCE}")
