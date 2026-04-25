# Espressif ESP32-C3 Platform Configuration
# Architecture: RISC-V RV32IMC
# Toolchain: riscv32-esp-elf-gcc (ESP-IDF or standalone)

message(STATUS "Configuring for ESP32-C3 platform (RISC-V RV32IMC)")

set(ALLOY_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/device)

add_compile_options(-march=rv32imc -mabi=ilp32
                    -Wall -Wextra -ffunction-sections -fdata-sections
                    -fno-exceptions -fno-rtti -fno-threadsafe-statics)
add_link_options(-march=rv32imc -mabi=ilp32 -Wl,--gc-sections)

message(STATUS "ESP32-C3 platform configured")
