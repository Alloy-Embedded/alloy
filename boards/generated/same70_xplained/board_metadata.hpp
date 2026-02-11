#pragma once

/**
 * @file board_metadata.hpp
 * @brief Generated board metadata fragment for same70_xplained
 *
 * Auto-generated from boards/*/board.yaml.
 * DO NOT EDIT MANUALLY.
 */

namespace microcore::generated::board_metadata::same70_xplained {

inline constexpr char board_id[] = "same70_xplained";
inline constexpr char board_name[] = "SAME70 Xplained Ultra";
inline constexpr char board_vendor[] = "Microchip/Atmel";
inline constexpr char platform[] = "same70";
inline constexpr char mcu_part_number[] = "ATSAME70Q21B";
inline constexpr char architecture[] = "cortex-m7";
inline constexpr unsigned int system_clock_hz = 300000000u;

inline constexpr char linker_script[] = "boards/same70_xplained/ATSAME70Q21.ld";
inline constexpr char startup_source[] = "src/hal/vendors/atmel/same70/startup_same70.cpp";
inline constexpr char toolchain_file[] = "cmake/toolchains/arm-none-eabi.cmake";

inline constexpr char flash_command[] = "openocd";
inline constexpr char flash_binary_format[] = "elf";
inline constexpr char flash_load_address[] = "";
inline constexpr char openocd_target[] = "atsamv";
inline constexpr char openocd_interface[] = "cmsis-dap";
inline constexpr char debug_transport[] = "cmsis-dap";
inline constexpr char debug_probe[] = "EDBG";

inline constexpr unsigned int led_count = 1u;
inline constexpr unsigned int button_count = 1u;
inline constexpr unsigned int uart_count = 1u;
inline constexpr unsigned int spi_count = 0u;
inline constexpr unsigned int i2c_count = 0u;

}  // namespace microcore::generated::board_metadata::same70_xplained
