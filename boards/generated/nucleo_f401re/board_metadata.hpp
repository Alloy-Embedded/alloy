#pragma once

/**
 * @file board_metadata.hpp
 * @brief Generated board metadata fragment for nucleo_f401re
 *
 * Auto-generated from boards/*/board.yaml.
 * DO NOT EDIT MANUALLY.
 */

namespace microcore::generated::board_metadata::nucleo_f401re {

inline constexpr char board_id[] = "nucleo_f401re";
inline constexpr char board_name[] = "Nucleo-F401RE";
inline constexpr char board_vendor[] = "STMicroelectronics";
inline constexpr char platform[] = "stm32f4";
inline constexpr char mcu_part_number[] = "STM32F401RET6";
inline constexpr char architecture[] = "cortex-m4";
inline constexpr unsigned int system_clock_hz = 84000000u;

inline constexpr char linker_script[] = "boards/nucleo_f401re/STM32F401RET6.ld";
inline constexpr char startup_source[] = "src/hal/vendors/st/stm32f4/stm32f401/startup.cpp";
inline constexpr char toolchain_file[] = "cmake/toolchains/arm-none-eabi.cmake";

inline constexpr char flash_command[] = "st-flash";
inline constexpr char flash_binary_format[] = "bin";
inline constexpr char flash_load_address[] = "0x08000000";
inline constexpr char openocd_target[] = "stm32f4x";
inline constexpr char openocd_interface[] = "stlink";
inline constexpr char debug_transport[] = "stlink";
inline constexpr char debug_probe[] = "ST-LINK/V2-1";

inline constexpr unsigned int led_count = 1u;
inline constexpr unsigned int button_count = 1u;
inline constexpr unsigned int uart_count = 1u;
inline constexpr unsigned int spi_count = 1u;
inline constexpr unsigned int i2c_count = 1u;

}  // namespace microcore::generated::board_metadata::nucleo_f401re
