# Auto-generated from boards/*/board.yaml
# DO NOT EDIT MANUALLY.

set(MICROCORE_GENERATED_CODEGEN_CONTRACT_ID "microcore-board-artifacts")
set(MICROCORE_GENERATED_CODEGEN_CONTRACT_VERSION "1.0.0")
set(MICROCORE_GENERATED_CODEGEN_VERSION "1.1.0")
set(MICROCORE_GENERATED_BOARD_SCHEMA_VERSION "1.0")
set(MICROCORE_GENERATED_FRAMEWORK_VERSION "0.1.0")
set(MICROCORE_GENERATED_FRAMEWORK_VERSION_MAJOR "0")
set(MICROCORE_GENERATED_FRAMEWORK_VERSION_MINOR "1")
set(MICROCORE_GENERATED_FRAMEWORK_VERSION_PATCH "0")
set(MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MAJOR "0")
set(MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MIN "1")
set(MICROCORE_GENERATED_COMPATIBLE_FRAMEWORK_MINOR_MAX "1")

set(MICROCORE_GENERATED_SUPPORTED_BOARDS
    "nucleo_f401re"
    "nucleo_f722ze"
    "nucleo_g071rb"
    "nucleo_g0b1re"
    "same70_xplained"
)

set(MICROCORE_GENERATED_BOARD_PLATFORM_MAP
    "nucleo_f401re:stm32f4"
    "nucleo_f722ze:stm32f7"
    "nucleo_g071rb:stm32g0"
    "nucleo_g0b1re:stm32g0"
    "same70_xplained:same70"
)

# Auto-generated from boards/*/board.yaml
# Board: nucleo_f401re

set(MICROCORE_GENERATED_BOARD_PLATFORM_nucleo_f401re "stm32f4")
set(MICROCORE_GENERATED_BOARD_NAME_nucleo_f401re "Nucleo-F401RE")
set(MICROCORE_GENERATED_BOARD_VENDOR_nucleo_f401re "STMicroelectronics")
set(MICROCORE_GENERATED_BOARD_MCU_nucleo_f401re "STM32F401RET6")
set(MICROCORE_GENERATED_BOARD_ARCH_nucleo_f401re "cortex-m4")
set(MICROCORE_GENERATED_BOARD_HEADER_nucleo_f401re "boards/nucleo_f401re/board.hpp")
set(MICROCORE_GENERATED_BOARD_LINKER_SCRIPT_nucleo_f401re "boards/nucleo_f401re/STM32F401RET6.ld")
set(MICROCORE_GENERATED_BOARD_STARTUP_SOURCE_nucleo_f401re "src/hal/vendors/st/stm32f4/stm32f401/startup.cpp")
set(MICROCORE_GENERATED_BOARD_TOOLCHAIN_nucleo_f401re "cmake/toolchains/arm-none-eabi.cmake")
set(MICROCORE_GENERATED_BOARD_SUPPORTED_EXAMPLES_nucleo_f401re "blink;rtos/simple_tasks;api_tiers/simple_gpio_blink;api_tiers/simple_gpio_button")
set(MICROCORE_GENERATED_BOARD_SUPPORTS_RTOS_nucleo_f401re "ON")
set(MICROCORE_GENERATED_BOARD_FLASH_COMMAND_nucleo_f401re "st-flash")
set(MICROCORE_GENERATED_BOARD_FLASH_ADDRESS_nucleo_f401re "0x08000000")
set(MICROCORE_GENERATED_BOARD_OPENOCD_TARGET_nucleo_f401re "stm32f4x")
set(MICROCORE_GENERATED_BOARD_OPENOCD_INTERFACE_nucleo_f401re "stlink")

# Auto-generated from boards/*/board.yaml
# Board: nucleo_f722ze

set(MICROCORE_GENERATED_BOARD_PLATFORM_nucleo_f722ze "stm32f7")
set(MICROCORE_GENERATED_BOARD_NAME_nucleo_f722ze "Nucleo-F722ZE")
set(MICROCORE_GENERATED_BOARD_VENDOR_nucleo_f722ze "STMicroelectronics")
set(MICROCORE_GENERATED_BOARD_MCU_nucleo_f722ze "STM32F722ZET6")
set(MICROCORE_GENERATED_BOARD_ARCH_nucleo_f722ze "cortex-m7")
set(MICROCORE_GENERATED_BOARD_HEADER_nucleo_f722ze "boards/nucleo_f722ze/board.hpp")
set(MICROCORE_GENERATED_BOARD_LINKER_SCRIPT_nucleo_f722ze "boards/nucleo_f722ze/STM32F722ZET6.ld")
set(MICROCORE_GENERATED_BOARD_STARTUP_SOURCE_nucleo_f722ze "src/hal/vendors/st/stm32f7/stm32f722/startup.cpp")
set(MICROCORE_GENERATED_BOARD_TOOLCHAIN_nucleo_f722ze "cmake/toolchains/arm-none-eabi.cmake")
set(MICROCORE_GENERATED_BOARD_SUPPORTED_EXAMPLES_nucleo_f722ze "blink;rtos/simple_tasks;api_tiers/simple_gpio_blink;api_tiers/simple_gpio_button")
set(MICROCORE_GENERATED_BOARD_SUPPORTS_RTOS_nucleo_f722ze "ON")
set(MICROCORE_GENERATED_BOARD_FLASH_COMMAND_nucleo_f722ze "st-flash")
set(MICROCORE_GENERATED_BOARD_FLASH_ADDRESS_nucleo_f722ze "0x08000000")
set(MICROCORE_GENERATED_BOARD_OPENOCD_TARGET_nucleo_f722ze "stm32f7x")
set(MICROCORE_GENERATED_BOARD_OPENOCD_INTERFACE_nucleo_f722ze "stlink")

# Auto-generated from boards/*/board.yaml
# Board: nucleo_g071rb

set(MICROCORE_GENERATED_BOARD_PLATFORM_nucleo_g071rb "stm32g0")
set(MICROCORE_GENERATED_BOARD_NAME_nucleo_g071rb "Nucleo-G071RB")
set(MICROCORE_GENERATED_BOARD_VENDOR_nucleo_g071rb "STMicroelectronics")
set(MICROCORE_GENERATED_BOARD_MCU_nucleo_g071rb "STM32G071RBT6")
set(MICROCORE_GENERATED_BOARD_ARCH_nucleo_g071rb "cortex-m0+")
set(MICROCORE_GENERATED_BOARD_HEADER_nucleo_g071rb "boards/nucleo_g071rb/board.hpp")
set(MICROCORE_GENERATED_BOARD_LINKER_SCRIPT_nucleo_g071rb "boards/nucleo_g071rb/STM32G071RBT6.ld")
set(MICROCORE_GENERATED_BOARD_STARTUP_SOURCE_nucleo_g071rb "src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp")
set(MICROCORE_GENERATED_BOARD_TOOLCHAIN_nucleo_g071rb "cmake/toolchains/arm-none-eabi.cmake")
set(MICROCORE_GENERATED_BOARD_SUPPORTED_EXAMPLES_nucleo_g071rb "blink;api_tiers/simple_gpio_blink;api_tiers/simple_gpio_button")
set(MICROCORE_GENERATED_BOARD_SUPPORTS_RTOS_nucleo_g071rb "OFF")
set(MICROCORE_GENERATED_BOARD_FLASH_COMMAND_nucleo_g071rb "st-flash")
set(MICROCORE_GENERATED_BOARD_FLASH_ADDRESS_nucleo_g071rb "0x08000000")
set(MICROCORE_GENERATED_BOARD_OPENOCD_TARGET_nucleo_g071rb "stm32g0x")
set(MICROCORE_GENERATED_BOARD_OPENOCD_INTERFACE_nucleo_g071rb "stlink")

# Auto-generated from boards/*/board.yaml
# Board: nucleo_g0b1re

set(MICROCORE_GENERATED_BOARD_PLATFORM_nucleo_g0b1re "stm32g0")
set(MICROCORE_GENERATED_BOARD_NAME_nucleo_g0b1re "Nucleo-G0B1RE")
set(MICROCORE_GENERATED_BOARD_VENDOR_nucleo_g0b1re "STMicroelectronics")
set(MICROCORE_GENERATED_BOARD_MCU_nucleo_g0b1re "STM32G0B1RET6")
set(MICROCORE_GENERATED_BOARD_ARCH_nucleo_g0b1re "cortex-m0+")
set(MICROCORE_GENERATED_BOARD_HEADER_nucleo_g0b1re "boards/nucleo_g0b1re/board.hpp")
set(MICROCORE_GENERATED_BOARD_LINKER_SCRIPT_nucleo_g0b1re "boards/nucleo_g0b1re/STM32G0B1RET6.ld")
set(MICROCORE_GENERATED_BOARD_STARTUP_SOURCE_nucleo_g0b1re "src/hal/vendors/st/stm32g0/stm32g0b1/startup.cpp")
set(MICROCORE_GENERATED_BOARD_TOOLCHAIN_nucleo_g0b1re "cmake/toolchains/arm-none-eabi.cmake")
set(MICROCORE_GENERATED_BOARD_SUPPORTED_EXAMPLES_nucleo_g0b1re "blink;api_tiers/simple_gpio_blink;api_tiers/simple_gpio_button")
set(MICROCORE_GENERATED_BOARD_SUPPORTS_RTOS_nucleo_g0b1re "OFF")
set(MICROCORE_GENERATED_BOARD_FLASH_COMMAND_nucleo_g0b1re "st-flash")
set(MICROCORE_GENERATED_BOARD_FLASH_ADDRESS_nucleo_g0b1re "0x08000000")
set(MICROCORE_GENERATED_BOARD_OPENOCD_TARGET_nucleo_g0b1re "stm32g0x")
set(MICROCORE_GENERATED_BOARD_OPENOCD_INTERFACE_nucleo_g0b1re "stlink")

# Auto-generated from boards/*/board.yaml
# Board: same70_xplained

set(MICROCORE_GENERATED_BOARD_PLATFORM_same70_xplained "same70")
set(MICROCORE_GENERATED_BOARD_NAME_same70_xplained "SAME70 Xplained Ultra")
set(MICROCORE_GENERATED_BOARD_VENDOR_same70_xplained "Microchip/Atmel")
set(MICROCORE_GENERATED_BOARD_MCU_same70_xplained "ATSAME70Q21B")
set(MICROCORE_GENERATED_BOARD_ARCH_same70_xplained "cortex-m7")
set(MICROCORE_GENERATED_BOARD_HEADER_same70_xplained "boards/same70_xplained/board.hpp")
set(MICROCORE_GENERATED_BOARD_LINKER_SCRIPT_same70_xplained "boards/same70_xplained/ATSAME70Q21.ld")
set(MICROCORE_GENERATED_BOARD_STARTUP_SOURCE_same70_xplained "src/hal/vendors/atmel/same70/startup_same70.cpp")
set(MICROCORE_GENERATED_BOARD_TOOLCHAIN_same70_xplained "cmake/toolchains/arm-none-eabi.cmake")
set(MICROCORE_GENERATED_BOARD_SUPPORTED_EXAMPLES_same70_xplained "blink;api_tiers/simple_gpio_blink;api_tiers/simple_gpio_button")
set(MICROCORE_GENERATED_BOARD_SUPPORTS_RTOS_same70_xplained "OFF")
set(MICROCORE_GENERATED_BOARD_FLASH_COMMAND_same70_xplained "openocd")
set(MICROCORE_GENERATED_BOARD_FLASH_ADDRESS_same70_xplained "")
set(MICROCORE_GENERATED_BOARD_OPENOCD_TARGET_same70_xplained "atsamv")
set(MICROCORE_GENERATED_BOARD_OPENOCD_INTERFACE_same70_xplained "cmsis-dap")
